#ifndef __BALL_CTRL_H
#define __BALL_CTRL_H

#include "ti_msp_dl_config.h"
#include <stdint.h>

/* ====== ����λ��PID�ṹ�� (λ��ʽ, ����controll.c���) ====== */
typedef struct {
    float kp, ki, kd;           /* PID���� */
    float err, err_last;        /* ����ƫ��(cm) / �ϴ�ƫ�� */
    float integral;             /* �����ۻ� */
    float output;               /* PID��� = �ڸ˽Ƕ�(��) */
    float limit;                /* ����޷� */
    float i_limit;              /* �����޷� */
} BallPID;

/* ====== �������ģʽ ====== */
typedef enum {
    BALL_IDLE = 0,              /* ���� */
    BALL_TASK3,                 /* ��3��: K230 �Զ� O��+5��-5 */
    BALL_TASK45,                /* ��4/5��: ��ʻ�б��� */
    BALL_TASK6                  /* ��6��: ��ʻ�б�������λ�� */
} BallMode;

extern volatile BallPID  ball_pid;
extern volatile BallMode ball_mode;
extern volatile float    ball_dt;     /* ���֡ dt (s) */
extern volatile uint8_t  g_ball_pid_enable;  /* 1=PID 运行 (开环阶段可置 0 关闭) */

void BALL_Init(void);                           /* ��ʼ�� (ʹ�ܵ��+����+��е����) */
void BALL_Update(float error_px);             /* ͼ����ƫ��, PID�������Ƕ� */
void BALL_Reset(void);                          /* 复位: 电机回20度, PID清零 */
void BALL_SetMode(BallMode mode);               /* 切模式 */
void BALL_SetPID(float kp, float ki, float kd); /* ���ߵ��� */

#endif /* __BALL_CTRL_H */

