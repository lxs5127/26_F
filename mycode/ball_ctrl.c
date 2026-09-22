#include "ball_ctrl.h"
#include "stepper.h"
#include "clock.h"              /* tick_ms */
#include "oled_hardware_i2c.h"  /* delay_ms */
#include "vofa.h"

/* 相机最近有效帧时间戳 (camera_uart.c 定义), 用于丢帧保护 */
extern volatile uint32_t g_cam_last_update_ms;

#define CMD_INTERVAL  25u   /* 命令最小间隔 ms (40Hz, 太高电机会抖) */

/* ====== 机械限位: 管子水平=20°, 活动范围0~40° ====== */
#define CENTER_ANGLE  20.0f
#define ANGLE_MIN      0.0f
#define ANGLE_MAX     40.0f

/* 方向极性: 电机越修正球越跑偏时改为 -1 */
#define BALL_POLARITY  -1.0f

/* ====== ȫ�ֱ��� ====== */
// volatile BallPID  ball_pid  = {0.08f, 0.04f, 0.08f, 0, 0, 0, 0, 20.0f, 10.0f};
volatile BallPID  ball_pid  = {0.2f, 0.1f, 0.2f, 0, 0, 0, 0, 2.0f, 10.0f};//0.4     0.15
volatile uint8_t   g_ball_pid_enable = 1;   /* 默认开启 PID; task_3 开环阶段可置 0 */
void BALL_Init(void)
{
    STEPPER_UART_Init();
    delay_ms(200);                  /* �ȵ������������ */
    STEPPER_Enable();
    delay_ms(200);                  /* ȷ��ʹ����Ч */


    ball_pid.integral = 0;
    ball_pid.err_last = 0;
    // STEPPER_GoToAngle(20.0f);
    STEPPER_CB_Absolute(0, STEPPER_SPEED, 200, STEPPER_CURRENT);
    delay_ms(50);

    /* 启动 TIMER_tepper (20ms), 触发 BALL_Update 的定时器中断 */
    DL_TimerG_startCounter(TIMER_tepper_INST);
}

/* ====== 球控制: 相机偏差 → PID → 电机 ======
 *  输入: error_px  球相对画面中心的横向偏差(像素, g_cam_dx)
 *  位置式 PID, 真实dt, 25ms 限频, CB 直通绝对定位
 *  VOFA 50Hz 输出: CH1=误差 CH2=总输出 CH3=目标角度 CH4=积分
 */
void BALL_Update(float error_px)
{
    /* ─── 固定 dt: BALL_Update 仅由 20ms 定时器中断调用 ─── */
    float dt = 0.020f;

    /* ─── 相机丢帧保护: 100ms 无新帧 → 冻结, 防积旧误差 ─── */
    if (tick_ms - g_cam_last_update_ms > 100) {
        ball_pid.integral = 0;
        ball_pid.err_last = 0;
        ball_pid.output   = 0;
        return;
    }

    float err = BALL_POLARITY * error_px;

    /* ─── PID 计算 ─── */
    /* P */
    float p_out = ball_pid.kp * err;

    /* I: 用当前积分值计算 (抗饱和在限幅后处理) */
    float i_out = ball_pid.ki * ball_pid.integral;

    /* D (对误差微分, 防除零) */
    float d_out = (dt > 0.001f)
                ? ball_pid.kd * (err - ball_pid.err_last) / dt : 0.0f;

    /* 总输出 = P + I + D, 限幅 */
    float out = p_out + i_out + d_out;
    if (out >  ball_pid.limit) out =  ball_pid.limit;
    if (out < -ball_pid.limit) out = -ball_pid.limit;

    /* ─── 抗饱和积分: 输出已饱和时不继续朝饱和方向积分 ───
     *   防止积分windup把输出锁死在极限, 球反向时能立刻响应 */
    if (!(out >=  ball_pid.limit && err > 0) &&
        !(out <= -ball_pid.limit && err < 0)) {
        ball_pid.integral += err * dt;
        if (ball_pid.integral >  ball_pid.i_limit) ball_pid.integral =  ball_pid.i_limit;
        if (ball_pid.integral < -ball_pid.i_limit) ball_pid.integral = -ball_pid.i_limit;
    }

    ball_pid.err_last = err;
    ball_pid.err      = err;
    ball_pid.output   = out;

    /* ─── 限频: CB 直通高频发命令会抖 ─── */
    {
        static uint32_t last_cmd = 0;
        if (tick_ms - last_cmd < CMD_INTERVAL) return;
        last_cmd = tick_ms;
    }

    /* ─── 死区: PID输出太小不动 ─── */
    float mag = (out >= 0.0f) ? out : -out;
    if (mag < 0.02f) return;

    /* ─── PID输出 → 绝对角度 (中心=20°, 范围0~40°) ─── */
    float target_angle = CENTER_ANGLE + out;
    if (target_angle > ANGLE_MAX) target_angle = ANGLE_MAX;
    if (target_angle < ANGLE_MIN) target_angle = ANGLE_MIN;

    STEPPER_GoToAngle(target_angle);
}

/* ====== 复位: 电机回20°水平 + PID状态清零 ====== */
void BALL_Reset(void)
{
    ball_pid.integral = 0;
    ball_pid.err_last = 0;
    ball_pid.output   = 0;
    STEPPER_GoToAngle(20.0f);
}

/* ====== VOFA 数据输出 (主循环调用, printf 不抢占串口) ====== */
void BALL_VOFA_Output(void)
{
    static uint32_t last_vofa = 0;
    if (tick_ms - last_vofa < 20) return;   /* 50Hz */
    last_vofa = tick_ms;

    float target = CENTER_ANGLE + ball_pid.output;
    if (target > ANGLE_MAX) target = ANGLE_MAX;
    if (target < ANGLE_MIN) target = ANGLE_MIN;
    printf("BALL err=%.1f out=%.2f tgt=%.2f int=%.2f\r\n",
           (double)ball_pid.err, (double)ball_pid.output,
           (double)target, (double)ball_pid.integral);
}
