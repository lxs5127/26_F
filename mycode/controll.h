#ifndef __CONTROLL_H
#define __CONTROLL_H

#include "ti_msp_dl_config.h"

/* 单轮PI控制器结构体 */
typedef struct {
    float kpv, kiv;             /* PI增益 */
    float err, err_last;        /* 当前误差 / 上次误差 */
    float PWM;                  /* 输出PWM值 */
    float filt_speed, filt_last; /* 滤波后速度 / 上次滤波值 */
} WheelPID;

/* 左右轮PID控制器实例 */
extern volatile WheelPID pid_L;
extern volatile WheelPID pid_R;

/* 共享控制变量 */
extern volatile float middle_speed;
extern volatile float bias;
extern volatile float a;            /* 一阶低通滤波系数 */

/* 调试用目标速度 */
extern volatile float target_speed_L;
extern volatile float target_speed_R;

/* 定角度转向控制结构体 */
typedef struct {
    float kpv, kdv;             /* PD增益 */
    float err, err_last;        /* 当前角度误差 / 上次误差 */
    float output;               /* 转向输出速度 */
    float target;               /* 目标角度 */
    uint8_t active;             /* 转向标志 */
    uint8_t stable_cnt;         /* 连续稳定计数 */
} TurnControl;

extern volatile TurnControl turn_ctrl;

/* 航向保持控制结构体 */
typedef struct {
    float kpv, kdv;             /* PD增益 */
    float err_last;             /* 上次偏航误差 */
    float reference;            /* 参考航向角 */
    uint8_t active;             /* 使能标志 */
    uint8_t locked;             /* 参考角已锁定 */
} HeadingHold;

extern volatile HeadingHold heading;

/* 巡线控制结构体 */
typedef struct {
    float kpv, kdv;             /* PD增益 */
    float err_last;             /* 上次误差 */
    uint16_t lost_cnt;          /* 丢线持续计数 (丢线保持用) */
} LineControl;

extern volatile LineControl line_ctrl;

void process_line_sensors(void);   /* 主循环调用：读灰度传感器 → 停车检测 + 巡线数据准备 */
uint8_t detect_stop(void);

/* ====== 位置环控制结构体 (PD外环) ======
 * 外环 PD + 内环 PI(速度环)，无需 I 项 */
typedef struct {
    float kpp, kdp;              /* PD增益 */
    float err, err_last;         /* 位置误差 / 上次误差 */
    float target_pos;            /* 目标位置 (cm) */
    float output;                /* 输出速度 (m/s) */
    uint8_t active;              /* 使能标志 */
    uint8_t stable_cnt;          /* 到达稳定计数 */
    float arrive_threshold;      /* 到达判定阈值 (cm) */
    float speed_limit;           /* 位置环最大输出速度 (m/s) */
} PositionCtrl;

extern volatile PositionCtrl pos_ctrl;
extern volatile float pos_left, pos_right;  /* 左右轮累积位置 (cm) */

extern volatile uint8_t g_corner_flag;      /* 直角检测标志 (中断内刷新) */

extern float gz_dps;                        /* MPU6050 Z轴角速度 (deg/s) */
extern volatile float gyro_damp_k;          /* 陀螺仪阻尼增益 0.0005~0.0030 */

void speed_controll_L(void);
void speed_controll_R(void);
void Controll(void);
void position_controll(void);
void set_target_position(float pos_cm);
void set_position_gain(float kp, float kd);
void reset_position(void);
uint8_t is_position_arrive(void);

void set_bias(float b);
void set_target_speed(float speed);
void stop_motion(void);     /* 急停: 清零全部运动状态, 控制中断立即输出0 */
void set_pid_gain_L(float kp, float ki);
void set_pid_gain_R(float kp, float ki);

/* 定角度转向 (使用 MPU6050 yaw) */
void turn_controll(void);
void turn_angle(float angle_deg);
void set_turn_gain(float kp, float kd);
uint8_t is_turning(void);

#endif
