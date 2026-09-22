/**
 * @file    gimbal_task.c
 * @brief   云台控制任务实现
 * @note    TIMER_tepper (TIMG6) 中断中周期性调用 Gimbal_Run()
 *          周期 20ms (50Hz)
 */

#include "gimbal_task.h"
#include "ti_msp_dl_config.h"
#include "button.h"

#include "camera_uart.h"

/* 摄像头横向偏差 (在 main.c 中定义) */
extern volatile int16_t g_cam_dx;

/* 任务状态 (在 button.c 中定义) */
extern volatile uint8_t  g_start_flag;
extern volatile TaskMode g_task;

void TIMER_tepper_INST_IRQHandler(void)
{
        BALL_Update((float)g_cam_dx);
}
