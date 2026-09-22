#include "controll.h"
#include "motor.h"
#include "encoder.h"
#include "vofa.h"
#include "grayscale_sensor.h"
#include "stdio.h"
#define PWM_MAX   4000
#define SPEED_LIMIT_MPS   0.5f   /* 轮子最高速度 (m/s) */

/* 软限幅: 限制轮子目标速度在 ±SPEED_LIMIT_MPS 内 */
static inline float clamp_speed(float spd)
{
    if (spd >  SPEED_LIMIT_MPS) spd =  SPEED_LIMIT_MPS;
    if (spd < -SPEED_LIMIT_MPS) spd = -SPEED_LIMIT_MPS;
    return spd;
}

extern float encoder_left,encoder_right;

/* PID控制器结构体实例化 */
volatile WheelPID pid_L = {35, 5, 0,0,0,0,0};           
volatile WheelPID pid_R = {35, 5, 0,0,0,0,0};

volatile float middle_speed=0, target_speed_L=0, target_speed_R=0;
volatile float bias=0;
volatile float a=0.3;
volatile uint8_t g_stop_flag = 0;  /* 中断内检测到停车标志, 主循环消费 */

/* ====== 定角度转向 ====== */
extern volatile float yaw;               /* MPU6050 偏航角 */
extern float gz_dps;                     /* MPU6050 Z轴角速度 (deg/s) */

/* 陀螺仪阻尼增益：bias = Kp * err - gyro_damp_k * damp_scale * gz_dps
 * 增大 → 转弯阻尼更强, 防蛇形; 减小 → 转弯更灵活
 * 建议范围: 0.0002 ~ 0.0010, 初始值 0.0005 */
volatile float gyro_damp_k = 0.001f;

volatile TurnControl turn_ctrl = {5, 26.0, 0,0,0,0,0,0};//4,30
#define MAX_TURN_SPEED  MS_TO_PULSE(0.10f)
#define MOTOR_DEAD_ZONE 150                /* 电机死区：低于此PWM不转 */

/* ====== 航向保持（走直线纠偏） ====== */
volatile HeadingHold heading = {0.05f, 0.30f, 0, 0, 0, 0};
#define HDG_BIAS_MAX  0.20f               /* 最大纠偏差速 (m/s): bias 直接限幅, 防异常尖峰 */

/* ====== 位置环 ====== */
volatile float pos_left = 0, pos_right = 0;   /* 累积位置 (cm) */
volatile PositionCtrl pos_ctrl = {0.30f, 2.5f, 0,0, 0, 0, 0, 0, 0.5f, 0.15f};


static void speed_controll_Wheel(volatile WheelPID *pid, float encoder_val, float target_mps)
{
    /* 将 m/s 转为脉冲数/10ms */
    float target = MS_TO_PULSE(target_mps);

    /* 一阶低通滤波 */
    pid->filt_speed = a * encoder_val + (1 - a) * pid->filt_last;
    pid->err = target - pid->filt_speed;

    /* ====== 增量式 PI ====== */
    float delta = pid->kpv * (pid->err - pid->err_last)
                + pid->kiv * pid->err;

    /* 抗积分饱和：输出饱和时拒绝同向增量 */
    if ((pid->PWM >=  PWM_MAX && delta > 0) ||
        (pid->PWM <= -PWM_MAX && delta < 0))
    {
        /* 饱和，跳过累加 */
    }
    else
    {
        pid->PWM += delta;
    }

    /* 输出限幅 */
    if (pid->PWM >  PWM_MAX) pid->PWM =  PWM_MAX;
    if (pid->PWM < -PWM_MAX) pid->PWM = -PWM_MAX;

    pid->err_last = pid->err;
    pid->filt_last = pid->filt_speed;
}

void speed_controll_L(void) { speed_controll_Wheel(&pid_L, encoder_left, target_speed_L); }
void speed_controll_R(void) { speed_controll_Wheel(&pid_R, encoder_right, target_speed_R); }


void turn_controll(void)
{
    if (!turn_ctrl.active) return;

    /* 角度误差（处理过零） */
    turn_ctrl.err = turn_ctrl.target - yaw;
    if (turn_ctrl.err > 180)  turn_ctrl.err -= 360;
    if (turn_ctrl.err < -180) turn_ctrl.err += 360;

    /* PD 控制：P=恢复力, D=阻尼 */
    float delta_err = turn_ctrl.err - turn_ctrl.err_last;
    turn_ctrl.output = turn_ctrl.kpv * turn_ctrl.err + turn_ctrl.kdv * delta_err;

    /* 输出限幅 */
    if (turn_ctrl.output >  MAX_TURN_SPEED) turn_ctrl.output =  MAX_TURN_SPEED;
    if (turn_ctrl.output < -MAX_TURN_SPEED) turn_ctrl.output = -MAX_TURN_SPEED;

    /* 原地旋转 */
    target_speed_L =  SPEED_MPS(turn_ctrl.output);
    target_speed_R = -SPEED_MPS(turn_ctrl.output);

    /* 到达判断（连续 10 周期稳定才确认，防过冲误触发） */
    if (turn_ctrl.err > -0.5f && turn_ctrl.err < 0.5f) {
        turn_ctrl.stable_cnt++;
        if (turn_ctrl.stable_cnt >= 10) {
            turn_ctrl.active = 0;
            turn_ctrl.err = 0;
            turn_ctrl.stable_cnt = 0;
            target_speed_L = 0;
            target_speed_R = 0;
            middle_speed = 0;
        }
    } else {
        turn_ctrl.stable_cnt = 0;
    }

    turn_ctrl.err_last = turn_ctrl.err;
}

/* 死区补偿 */
static float deadzone_comp(float pwm)
{
    if (pwm > 0 && pwm < MOTOR_DEAD_ZONE) return MOTOR_DEAD_ZONE;
    if (pwm < 0 && pwm > -MOTOR_DEAD_ZONE) return -MOTOR_DEAD_ZONE;
    if (pwm == 0) return 0;
    return pwm;
}

/* ====== 航向保持（走直线纠偏） ====== */
static void heading_hold(void)
{
    /* 转向中 → 解锁，等转向完成再重新锁 */
    if (turn_ctrl.active) {
        heading.locked = 0;
        return;
    }

    /* 锁参考角 */
    if (!heading.locked) {
        heading.reference = yaw;
        heading.err_last = 0;
        heading.locked = 1;
        return;
    }

    /* 停车时解锁 */
    if (middle_speed > -0.01f && middle_speed < 0.01f) {
        heading.locked = 0;
        bias = 0;
        return;
    }

    /* 偏航误差 */
    float err = heading.reference - yaw;
    if (err > 180)  err -= 360;
    if (err < -180) err += 360;

    /* PD → bias */
    float delta_err = err - heading.err_last;
    bias = heading.kpv * err + heading.kdv * delta_err;

    if (bias >  HDG_BIAS_MAX) bias =  HDG_BIAS_MAX;
    if (bias < -HDG_BIAS_MAX) bias = -HDG_BIAS_MAX;

    heading.err_last = err;
}



/* ====== 巡线控制 ====== */
#define LINE_SENSOR_CNT  8

/* 8个传感器的权重：左→右 (顺时针 左转弯，右侧权重略强) */
static const float line_weight[LINE_SENSOR_CNT] = {3.5f, 2.5f, 1.5f, 0.5f, -0.5f, -1.5f, -2.5f, -3.5f};

volatile LineControl line_ctrl = {0.03f, 0.025f, 0, 0};   /* 快速 */
/* 灰度传感器共享缓存：主循环填充，ISR 读取 */
static uint16_t g_raw[LINE_SENSOR_CNT];

void process_line_sensors(void)
{
    Grayscale_Sensor_Read_All(g_raw);

    if (detect_stop()) g_stop_flag = 1;
}

static uint8_t line_tracking(void)
{
    /* 直接用 g_raw 缓存 */
    float sum_w = 0, sum_v = 0;
    for (int i = 0; i < LINE_SENSOR_CNT; i++) {
        if (g_raw[i]) {
            sum_w += line_weight[i];
            sum_v += 1.0f;
        }
    }

    /* 丢线保持: 保持上一次的bias继续按原方向寻线
     * 连续丢线超过 LOST_TIMEOUT(500ms) 才放弃, 交给上层航向保持兜底 */
#define LOST_TIMEOUT  50      /* 连续丢线周期数 (500ms) */
    if (sum_v < 0.5f) {
        if (line_ctrl.lost_cnt < LOST_TIMEOUT) {
            line_ctrl.lost_cnt++;
            return 1;       /* 保持 bias 不变, 继续按原方向走 */
        }
        line_ctrl.lost_cnt = 0;
        return 0;           /* 长时间丢线 → 交给上层(航向保持) */
    }
    line_ctrl.lost_cnt = 0;

    /* 误差 = 加权中心 */
    float err = sum_w / sum_v;

    /* 中心附近死区，跳过微小抖动 — 仅影响P项 */
    if (err > -0.5f && err < 0.5f) {
        err = 0.0f;
    }

    /* ====== 巡线融合 ======
     * P 项: 传感器偏差 → 拉回中心
     * D 项: 陀螺仪角速度 → 平滑阻尼, 替代传感器微分(噪声大)
     * 负号: 转弯时陀螺仪读出角速度, D 项产生反向阻尼
     *
     * 自适应阻尼: 偏差大(弯道)减小阻尼 → 不跟P项打架
     *             偏差小(直道)全阻尼 → 抑制微抖
     */
    float abs_err = (err > 0) ? err : -err;
    float damp_scale = 1.0f - (abs_err > 1.0f ? 1.0f : abs_err / 1.0f);
    bias = line_ctrl.kpv * err - gyro_damp_k * damp_scale * gz_dps;
    if (bias >  HDG_BIAS_MAX) bias =  HDG_BIAS_MAX;
    if (bias < -HDG_BIAS_MAX) bias = -HDG_BIAS_MAX;

    return 1;
}

/* ====== 停车检测（方形赛道终点） ======
 * 最两边传感器（索引 0,7）未检测到黑线，中间全部检测到 → 判定停车
 * 使用 g_raw 缓存，主循环调用 process_line_sensors() 后自动更新
 */
uint8_t detect_stop(void)
{
    /* 最两边(0,7)低电平，中间(4~6)全部高电平才认 */
    if ( (g_raw[4] && g_raw[5] && g_raw[6])||(g_raw[4] && g_raw[5] && g_raw[6]&& g_raw[7])
        ) return 1;
    return 0;
}//(g_raw[4] && g_raw[5] && g_raw[6])||(g_raw[5] && g_raw[6] && g_raw[7])||





/* ====== 位置环控制 (PD 外环 + PI 内环) ======*/
void position_controll(void)
{
    if (!pos_ctrl.active) return;

    /* 取左右轮平均位置 (cm) */
    float cur_pos = (pos_left + pos_right) * 0.5f;

    /* 位置误差 */
    pos_ctrl.err = pos_ctrl.target_pos - cur_pos;

    /* 误差绝对值 */
    float abs_err = pos_ctrl.err > 0 ? pos_ctrl.err : -pos_ctrl.err;

    /* 误差微分 → 阻尼项 */
    float delta_err = pos_ctrl.err - pos_ctrl.err_last;

    /* PD 输出 → 目标速度 (m/s) */
    pos_ctrl.output = pos_ctrl.kpp * pos_ctrl.err
                    + pos_ctrl.kdp * delta_err;

    /* 输出限幅 */
    if (pos_ctrl.output >  pos_ctrl.speed_limit) pos_ctrl.output =  pos_ctrl.speed_limit;
    if (pos_ctrl.output < -pos_ctrl.speed_limit) pos_ctrl.output = -pos_ctrl.speed_limit;

    middle_speed = pos_ctrl.output;

    /* 到达判定 */
    if (abs_err < pos_ctrl.arrive_threshold) {
        pos_ctrl.stable_cnt++;
        if (pos_ctrl.stable_cnt >= 10) {
            pos_ctrl.active = 0;
            pos_ctrl.stable_cnt = 0;
            middle_speed = 0;
        }
    } else {
        pos_ctrl.stable_cnt = 0;
    }

    pos_ctrl.err_last = pos_ctrl.err;
}





void Controll(void)
{
    /* 灰度传感器已由主循环 process_line_sensors() 读取到 g_raw 缓存中 */

    /* 累积编码器脉冲 → 位置 (cm) (含标定系数) */
    pos_left  += encoder_left  * WHEEL_CIRCUMFERENCE_CM / ENCODER_4X_RESOLUTION * POSITION_SCALE;
    pos_right += encoder_right * WHEEL_CIRCUMFERENCE_CM / ENCODER_4X_RESOLUTION * POSITION_SCALE;

    if (turn_ctrl.active) {
        turn_controll();
        /* 转向由 turn_controll() 直接设置 target_speed_L/R，跳过后续计算 */
    } else {
        /* 位置环激活时，由位置环设定 middle_speed */
        if (pos_ctrl.active) {
            position_controll();
        }
        if (middle_speed > 0.01f || middle_speed < -0.01f) {
            /* 位置环激活：全程航向保持，防止刹车段偏角漂移 */
            if (pos_ctrl.active) {
                heading_hold();
            }
            /* 非位置环：巡线优先, 脱线时切航向保持 */
            else {
                if (!line_tracking()) {
                    heading_hold();
                } else {
                    heading.locked = 0;
                }
            }
        } else {
            bias = 0;
            heading.locked = 0;
        }

        target_speed_L = middle_speed + bias;
        target_speed_R = middle_speed - bias;
    }

    target_speed_L = clamp_speed(target_speed_L);
    target_speed_R = clamp_speed(target_speed_R);

    speed_controll_R();
    speed_controll_L();

    float out_L = deadzone_comp(pid_L.PWM);
    float out_R = deadzone_comp(pid_R.PWM);
    set_speed(out_L, out_R);
}

/* ====== 急停: 复位所有运动控制状态 ======
 * 复位按钮按下时调用, 防止控制中断(10ms)在复位期间继续驱动轮子
 * 清空: 速度/偏置/PID累积/转向/位置环/航向 状态
 */
void stop_motion(void)
{
    middle_speed   = 0;
    target_speed_L = 0;
    target_speed_R = 0;
    bias           = 0;

    pid_L.PWM = 0; pid_L.err = 0; pid_L.err_last = 0;
    pid_L.filt_speed = 0; pid_L.filt_last = 0;
    pid_R.PWM = 0; pid_R.err = 0; pid_R.err_last = 0;
    pid_R.filt_speed = 0; pid_R.filt_last = 0;

    turn_ctrl.active     = 0;
    turn_ctrl.err        = 0;
    turn_ctrl.err_last   = 0;
    turn_ctrl.output     = 0;
    turn_ctrl.stable_cnt = 0;

    pos_ctrl.active      = 0;
    pos_ctrl.err         = 0;
    pos_ctrl.err_last    = 0;
    pos_ctrl.output      = 0;
    pos_ctrl.stable_cnt  = 0;

    heading.locked   = 0;
    heading.err_last = 0;
    heading.active   = 0;

    g_stop_flag = 0;

    /* 立即停轮 */
    set_speed(0, 0);
}

/* ====== 巡线偏置 ====== */
void set_bias(float b)
{
    bias = b;
}

/* ====== 目标速度（单位 m/s） ====== */
void set_target_speed(float speed)
{
    middle_speed = speed;
}

/* ====== PI增益设置 ====== */
void set_pid_gain_L(float kp, float ki)
{
    pid_L.kpv = kp;  pid_L.kiv = ki;
}

void set_pid_gain_R(float kp, float ki)
{
    pid_R.kpv = kp;  pid_R.kiv = ki;
}

/* ====== 定角度转向 ====== */

void turn_angle(float angle_deg)
{
    middle_speed = 0;               /* 转向时停车 */

    turn_ctrl.target = yaw + angle_deg;
    if (turn_ctrl.target > 180)  turn_ctrl.target -= 360;
    if (turn_ctrl.target < -180) turn_ctrl.target += 360;

    turn_ctrl.active = 1;
    turn_ctrl.stable_cnt = 0;
    /* err_last 初始化为当前误差，避免第一周期 D 项爆表 */
    turn_ctrl.err = turn_ctrl.target - yaw;
    if (turn_ctrl.err > 180)  turn_ctrl.err -= 360;
    if (turn_ctrl.err < -180) turn_ctrl.err += 360;
    turn_ctrl.err_last = turn_ctrl.err;
    turn_ctrl.output = 0;
}

void set_turn_gain(float kp, float kd)
{
    turn_ctrl.kpv = kp;
    turn_ctrl.kdv = kd;
}

uint8_t is_turning(void)
{
    return turn_ctrl.active;
}

/* ====== 位置环辅助函数 ====== */

void set_target_position(float pos_cm)
{
    reset_position();                  /* 清零累积位置 */
    pos_ctrl.target_pos = pos_cm;       /* 设定目标位置 */
    pos_ctrl.err = 0;
    pos_ctrl.err_last = 0;
    pos_ctrl.output = 0;
    pos_ctrl.stable_cnt = 0;
    pos_ctrl.active = 1;
}

void set_position_gain(float kp, float kd)
{
    pos_ctrl.kpp = kp;
    pos_ctrl.kdp = kd;
}

void reset_position(void)
{
    pos_left  = 0;
    pos_right = 0;
}

uint8_t is_position_arrive(void)
{
    return !pos_ctrl.active;
}


// 控制中断
void TIMER_controll_INST_IRQHandler(void)
{
    Controll();
}


