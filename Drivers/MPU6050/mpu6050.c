#include "ti_msp_dl_config.h"

#include "inv_mpu.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mpu6050.h"
#include "mspm0_clock.h"

/* ===== 采样率 & 灵敏度配置 ===== */
#define DEFAULT_MPU_HZ   (200)  /* 200Hz 采样 -> dt=5ms */

/* ===== 全局变量 ===== */
volatile uint8_t mpu6050_int_flag = 0;

float pitch, roll, yaw;

/* 加速度全局变量 (g), 供 gimbal.c 前馈使用 */
float g_mpu_accel_ax = 0.0f;
float g_mpu_accel_ay = 0.0f;
float g_mpu_accel_az = 0.0f;

/* 卡尔曼滤波器实例 */
KalmanFilter_t kf_pitch;
KalmanFilter_t kf_roll;

/* 原始传感器数据 */
short gyro[3], accel[3];
unsigned long sensor_timestamp;
unsigned long last_timestamp;  /* 上一次的时间戳 (ms) */

/* 灵敏度 */
static float gyro_sens;        /* LSB per deg/s */
static float accel_sens;       /* LSB per g */

/* 陀螺仪零偏校准值 (deg/s), 上电静止时采样平均 */
static float gyro_bias[3] = {0.0f, 0.0f, 0.0f};

/* 加速度计零偏校准值 (g), 上电静止时采样平均
 * Z轴减去 1g 重力, 使水平静止时三轴均 ≈ 0 */
static float accel_bias[3] = {0.0f, 0.0f, 0.0f};

/* 温度补偿 — 系数由校准时自学习得到 */
static float  gyro_temp_coeff;       /* 自学习的温漂系数 (°/s/°C) */
static long   calib_temp_raw;        /* 校准时参考温度 (Q16) */
static float  temp_bias_comp;        /* 当前温度补偿值 */
static uint16_t temp_sample_cnt;     /* 温度读取计数器 */

/* 加速度计初始角度偏置: 上电时记录的零位 (deg) */
static float accel_home_pitch = 0.0f;
static float accel_home_roll  = 0.0f;

/* yaw 只用陀螺仪积分 (无磁力计, 会漂移) */
static float yaw_integrated;

/* 陀螺仪 Z 轴角速度 (deg/s), 供巡线融合使用 */
float gz_dps;

/* 静止检测计数器 (用于运行时零偏微调) */
static uint16_t stationary_cnt = 0;

/* ===== 初始化 ===== */
void mpu6050_init(void)
{
    int result;
    unsigned short gyro_rate, gyro_fsr;
    unsigned char accel_fsr;

    result = mpu_init();
    if (result)
        DL_SYSCTL_resetDevice(DL_SYSCTL_RESET_POR);

    /* 配置 MPU6050 INT 引脚 (PB.1)：上升沿中断, 数据就绪时触发 */
    DL_GPIO_setLowerPinsPolarity(GPIOB, DL_GPIO_PIN_1_EDGE_RISE);
    DL_GPIO_clearInterruptStatus(GPIOB, GPIO_MPU6050_PIN_INT_PIN);
    DL_GPIO_enableInterrupt(GPIOB, GPIO_MPU6050_PIN_INT_PIN);

    /* 启动陀螺仪 + 加速度计 */
    mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    /* 配置 FIFO (同时会启用 INT 引脚的数据就绪中断) */
    mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    mpu_set_sample_rate(DEFAULT_MPU_HZ);

    /* 读回配置 */
    mpu_get_sample_rate(&gyro_rate);
    mpu_get_gyro_fsr(&gyro_fsr);
    mpu_get_accel_fsr(&accel_fsr);

    /* 获取灵敏度, 用于原始值转换 */
    mpu_get_gyro_sens(&gyro_sens);
    {
        unsigned short as;
        mpu_get_accel_sens(&as);
        accel_sens = (float)as;
    }

    /* ===== 快速稳定: MPU6050 PLL 锁定 & 内部电路稳定 ===== */
    mspm0_delay_ms(100);

    /* ===== 陀螺仪零偏校准 (峰峰值法抖动抑制) =====
     *
     * 策略:
     *   采样 400 次 × 5ms = 2 秒 (原来 10 秒)
     *   跟踪采样窗口内的 min/max:
     *     若任一轴的峰峰值 (max-min) > 阈值 → 判定传感器在振动
     *     等待 300ms 后重新校准, 最多重试 5 次
     *
     *   峰峰值法 vs 帧差值法:
     *     帧差值只看相邻帧跳变 — 漏掉缓慢但持续的振动
     *     峰峰值看整个窗口的振幅 — 任何形式的振动都会被捕获
     *
     * 阈值: 18 LSB ≈ 1.1 °/s @ ±2000dps
     *       静止噪声峰峰值 < 6 LSB, 留足余量
     */
    {
        #define CALIB_SAMPLES        400   /* 400 × 5ms = 2 秒 */
        #define CALIB_DELAY_MS       5
        #define JITTER_RANGE_LSB     18    /* 峰峰值阈值 (LSB), ~1.1°/s @ ±2000dps */
        #define MAX_RETRIES          5

        float sum[3];
        short raw[3];
        short min_raw[3], max_raw[3];
        unsigned long ts;
        int retry;

        for (retry = 0; retry < MAX_RETRIES; retry++)
        {
            if (retry > 0)
                mspm0_delay_ms(300);       /* 抖动后等待机械稳定 */

            /* 第一个样本初始化 min/max/sum */
            mpu_get_gyro_reg(raw, &ts);
            min_raw[0] = max_raw[0] = raw[0];
            min_raw[1] = max_raw[1] = raw[1];
            min_raw[2] = max_raw[2] = raw[2];
            sum[0] = (float)raw[0];
            sum[1] = (float)raw[1];
            sum[2] = (float)raw[2];

            int jitter = 0;

            for (int i = 1; i < CALIB_SAMPLES && !jitter; i++)
            {
                mspm0_delay_ms(CALIB_DELAY_MS);
                mpu_get_gyro_reg(raw, &ts);

                /* 更新 min/max */
                if (raw[0] < min_raw[0]) min_raw[0] = raw[0];
                if (raw[0] > max_raw[0]) max_raw[0] = raw[0];
                if (raw[1] < min_raw[1]) min_raw[1] = raw[1];
                if (raw[1] > max_raw[1]) max_raw[1] = raw[1];
                if (raw[2] < min_raw[2]) min_raw[2] = raw[2];
                if (raw[2] > max_raw[2]) max_raw[2] = raw[2];

                sum[0] += (float)raw[0];
                sum[1] += (float)raw[1];
                sum[2] += (float)raw[2];

                /* 每 32 个样本 (~160ms) 检查一次范围, 提前退出 */
                if ((i & 0x1F) == 0x1F)
                {
                    int r0 = max_raw[0] - min_raw[0];
                    int r1 = max_raw[1] - min_raw[1];
                    int r2 = max_raw[2] - min_raw[2];
                    if (r0 > JITTER_RANGE_LSB
                     || r1 > JITTER_RANGE_LSB
                     || r2 > JITTER_RANGE_LSB)
                        jitter = 1;        /* 窗口内有振动 */
                }
            }

            if (!jitter)
                break;                     /* 校准成功 */
            /* jitter → 丢弃本轮, 重试 */
        }

        if (retry < MAX_RETRIES)
        {
            gyro_bias[0] = sum[0] / (float)CALIB_SAMPLES / gyro_sens;
            gyro_bias[1] = sum[1] / (float)CALIB_SAMPLES / gyro_sens;
            gyro_bias[2] = sum[2] / (float)CALIB_SAMPLES / gyro_sens;
        }
        /* else: 5 次均失败 → 保持默认零偏 0 */

        /* 温度补偿: 记录参考温度, 温补系数留待后期离线标定 */
        mpu_get_temperature(&calib_temp_raw, &ts);
        gyro_temp_coeff = 0.0f;
        temp_bias_comp  = 0.0f;

        #undef CALIB_SAMPLES
        #undef CALIB_DELAY_MS
        #undef JITTER_RANGE_LSB
        #undef MAX_RETRIES
    }

    /* 初始化卡尔曼滤波器 */
    /* 参数:
     *   Q_angle=0.01:   角度过程噪声
     *   Q_bias=0.015:   零偏过程噪声 (0.003→0.015, 5x 加速偏置收敛)
     *                   运动后停下 ~2s 内消除 0.01°/s 残余偏置
     *   R_measure=0.1:  测量噪声
     *
     * 若振动大 → 增大 R_measure (如 1.0)
     * 若仍延迟 → 继续增大 Q_angle (如 0.05)
     */
    KalmanFilter_Init(&kf_pitch, 0.01f, 0.015f, 0.1f);
    KalmanFilter_Init(&kf_roll,  0.01f, 0.015f, 0.1f);

    /* 记录初始姿态作为零位偏置 (短平均, 抗抖动) */
    {
        #define HOME_SAMPLES  20          /* 20 次 × 5ms = 100ms 平均 */
        short a_raw[3];
        unsigned long ts;
        float sum_ax = 0.0f, sum_ay = 0.0f, sum_az = 0.0f;

        for (int i = 0; i < HOME_SAMPLES; i++)
        {
            mpu_get_accel_reg(a_raw, &ts);
            sum_ax += (float)a_raw[0];
            sum_ay += (float)a_raw[1];
            sum_az += (float)a_raw[2];
            mspm0_delay_ms(5);
        }
        float ax = sum_ax / (float)HOME_SAMPLES / accel_sens;
        float ay = sum_ay / (float)HOME_SAMPLES / accel_sens;
        float az = sum_az / (float)HOME_SAMPLES / accel_sens;

        /* 加速度计零偏: XY 应为 0, Z 应为 1g */
        accel_bias[0] = ax;
        accel_bias[1] = ay;
        accel_bias[2] = az - 1.0f;

        accel_home_pitch = atan2f(-ax, sqrtf(ay*ay + az*az)) * 57.29578f;
        accel_home_roll  = atan2f( ay, az) * 57.29578f;

        #undef HOME_SAMPLES
    }

    /* 初始化时间戳和 yaw */
    mspm0_get_clock_ms(&last_timestamp);
    yaw_integrated = 0.0f;

    /* 初始角度设为 0 */
    pitch = 0.0f;
    roll  = 0.0f;
    yaw   = 0.0f;
}

/* ===== 读取传感器并卡尔曼解算 ===== */
int Read_MPU6050(void)
{
    unsigned long now;
    float dt;
    float accel_pitch, accel_roll;
    float gx_dps, gy_dps;
    float ax_g, ay_g, az_g;

    /* 1. 读取原始数据 */
    if (mpu_get_gyro_reg(gyro, &sensor_timestamp) != 0)
        return -1;
    if (mpu_get_accel_reg(accel, &sensor_timestamp) != 0)
        return -1;

    /* 2. 计算时间间隔 */
    mspm0_get_clock_ms(&now);
    dt = (float)(now - last_timestamp) / 1000.0f;
    last_timestamp = now;

    /* 防止 dt 异常 (首次或溢出) */
    if (dt <= 0.0f || dt > 0.5f)
        dt = 1.0f / (float)DEFAULT_MPU_HZ;

    /* 3. 原始值 → 物理量 (减去零偏 + 温度补偿) */
    gx_dps = (float)gyro[0] / gyro_sens - gyro_bias[0];
    gy_dps = (float)gyro[1] / gyro_sens - gyro_bias[1];
    gz_dps = (float)gyro[2] / gyro_sens - gyro_bias[2] - temp_bias_comp;

    ax_g = (float)accel[0] / accel_sens;     /* g */
    ay_g = (float)accel[1] / accel_sens;
    az_g = (float)accel[2] / accel_sens;

    /* 更新全局加速度 (供 gimbal.c 前馈使用, 已去零偏)
     * XY 去偏后静止 ≈ 0, Z 去偏后静止 ≈ 1g */
    g_mpu_accel_ax = ax_g - accel_bias[0];
    g_mpu_accel_ay = ay_g - accel_bias[1];
    g_mpu_accel_az = az_g - accel_bias[2];

    /* 每 ~0.5 秒读一次温度, 更新补偿 */
    if ((temp_sample_cnt++ & 0x7F) == 0) {   /* 每 128 次 ≈ 0.64s */
        long temp_raw;
        if (mpu_get_temperature(&temp_raw, &sensor_timestamp) == 0) {
            float temp_delta = (float)(temp_raw - calib_temp_raw) / 65536.0f;
            temp_bias_comp = temp_delta * gyro_temp_coeff;
        }
    }

    /* ===== 双层零偏微调 =====
     *
     * Tier 1 (三轴, τ≈10s): 严格静止时快速修正全部三轴
     *   gyro_mag < 0.3°/s 且持续 0.4s → alpha=0.0005
     *
     * Tier 2 (仅Z轴, τ≈250s): 直线行驶时极慢修正Z轴零偏
     *   |gz_dps| < 1.0°/s → alpha=0.00002
     *   正常转弯 (|gz| > 1°/s) 时完全不影响
     *   运动中电机发热导致的Z轴零偏漂移会被缓慢吸收
     */
    {
        float gyro_mag = fabsf(gx_dps) + fabsf(gy_dps) + fabsf(gz_dps);

        /* Tier 1: 严格静止 */
        if (gyro_mag < 0.3f)
        {
            if (++stationary_cnt > 80)     /* ~0.4s 连续静止后才开始 */
            {
                gyro_bias[0] += 0.0005f * gx_dps;
                gyro_bias[1] += 0.0005f * gy_dps;
                gyro_bias[2] += 0.0005f * gz_dps;
            }
        }
        else
        {
            stationary_cnt = 0;
        }

        /* Tier 2: Z轴直线行驶微调 (运动中可用, 不影响转弯) */
        if (fabsf(gz_dps) < 1.0f)
        {
            gyro_bias[2] += 0.00002f * gz_dps;
        }
    }

    /* 4. 从加速度计推算角度 (作为卡尔曼滤波的观测值) */
    /*
     * Pitch: 绕 Y 轴旋转, 用 atan2(-ax, sqrt(ay^2+az^2))
     * Roll:  绕 X 轴旋转, 用 atan2(ay, az)
     *
     * 注意: 符号取决于 IMU 安装方向, 可能需要调整
     *
     * 减去 accel_home_xxx: 使上电时的初始姿态为 0°
     */
    accel_pitch = atan2f(-ax_g, sqrtf(ay_g * ay_g + az_g * az_g)) * 57.29578f
                  - accel_home_pitch;
    accel_roll  = atan2f( ay_g, az_g) * 57.29578f
                  - accel_home_roll;

    /* 5. 卡尔曼滤波融合 (带加速度幅值门控) */
    /*
     * Pitch: 对应 gyro[1] (Y轴陀螺仪), 但注意安装矩阵中 Y 轴反转
     *        原来的 gyro_orientation = {-1,0,0, 0,-1,0, 0,0,1}
     *        所以 gyro_y 需取反
     *
     * 加速度幅值门控:
     *   |accel| 偏离 1g 超过 0.15g → 说明有线性加减速
     *   → 此时加速度计角度不可信, 只用陀螺仪预测
     *   → 通过传入"预测值自身"作为观测值, 使卡尔曼 innovation=0
     *     协方差仍正常增长 (不确定性随时间累积)
     */
    {
        float accel_mag = sqrtf(ax_g * ax_g + ay_g * ay_g + az_g * az_g);
        int accel_ok = (fabsf(accel_mag - 1.0f) < 0.15f);

        if (accel_ok)
        {
            /* 正常: 加速度 ≈ 1g, 用加速度计角度修正 */
            pitch = KalmanFilter_Update(&kf_pitch, accel_pitch, -gy_dps, dt);
            roll  = KalmanFilter_Update(&kf_roll,  accel_roll,   gx_dps, dt);
        }
        else
        {
            /* 加减速中: 加速度计不可信, 只做陀螺预测 (innovation=0 技巧) */
            float pred_p = kf_pitch.angle + (-gy_dps - kf_pitch.bias) * dt;
            float pred_r = kf_roll.angle  + ( gx_dps - kf_roll.bias)  * dt;
            pitch = KalmanFilter_Update(&kf_pitch, pred_p, -gy_dps, dt);
            roll  = KalmanFilter_Update(&kf_roll,  pred_r,  gx_dps, dt);
        }
    }

    /* 6. Yaw (偏航): 仅用陀螺仪 Z 轴积分, 无磁力计无法修正漂移 */
    yaw_integrated += gz_dps * dt;
    yaw = yaw_integrated;

    mpu6050_int_flag = 1;

    return 0;
}
