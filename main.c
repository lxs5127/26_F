/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti_msp_dl_config.h"
#include "main.h"
#include "stdio.h"
#include "stepper.h"
#include "ball_ctrl.h"

uint8_t oled_buffer[32];
extern volatile PositionCtrl pos_ctrl;
extern volatile TaskMode g_task;
extern volatile uint8_t g_start_flag;
extern uint8_t val;
extern volatile uint8_t mpu6050_int_flag;  
extern float pitch, roll, yaw;              
uint16_t dist=0;
uint8_t test_done = 0;



/* 摄像头横向偏差 (供 gimbal.c / PID_DEBUG.c 使用) */
volatile int16_t g_cam_dx = 0;

/* 复位归位请求标志 (button.c 中 KEY1 复位时置位, 主循环检测后执行) */
volatile uint8_t g_home_request = 0;

int main(void)
{
    SYSCFG_DL_init();
    /* 启用 UART3 (相机) FIFO, 否则只有 1 字节缓存会丢帧 */
    DL_UART_enableFIFOs(UART_camera_INST);
    SysTick_Init();

    OLED_Init();

    mpu6050_init();

    /* Don't remove this! */
    Interrupt_Init();


    BALL_Init(); 

    /* 初始化秒表 */
    Stopwatch_Init();

    OLED_ShowString(0,7,(uint8_t *)"Init OK",8);
    delay_ms(500);
    OLED_Clear();

    // 最高2m/s
    // set_speed(0, 0);//死区250 180
    // MOTOR_OFF();

    while (1)
    {

        /* 显示当前任务 (%-6s 固定宽度，防止切换残留) */
        sprintf((char *)oled_buffer, "Task: %-6s", task_name(g_task));
        OLED_ShowString(0, 1, oled_buffer, 8);
        sprintf((char *)oled_buffer, "State: %-4s", g_start_flag ? "RUN" : "IDLE");
        OLED_ShowString(0, 2, oled_buffer, 8);

        /* 按键轮询 (读引脚电平, 无中断串扰问题) */
        read_button();

        /* 复位时归位到水平位 + PID清零 */
        if (g_home_request) {
            g_home_request = 0;
            g_cam_dx = 0;
            BALL_Reset();
            /* 清掉第7行的时间显示 */
            OLED_ShowString(0, 7, (uint8_t *)"                ", 8);
        }

        /* 处理灰度传感器 */
        process_line_sensors();

        /* ====== 任务调度 ======
         * KEY2 选任务 → KEY3 启动 → KEY4 复位
         * 在 TaskMode 枚举中添加新任务，在此处添加对应 case */
        if (g_start_flag) {
            switch (g_task) {
            case TASK_1:
                task_1();                        
                break;

            case TASK_2:
                task_2();
                break;

            case TASK_3:
                task_3();
                break;

            case TASK_4:
                task_4();
                break;

            case TASK_5:
                task_5();
                break;

            case TASK_6:
                task_6();
                break;

            default:
                break;
            }
        } else {
            /* 未启动: 清除任务内部状态 */
            set_target_speed(0.0f);
        }



        ParsePIDData();

        /* VOFA 调参数据输出 (主循环, 不与printf冲突) */
        BALL_VOFA_Output();
    }

}
    