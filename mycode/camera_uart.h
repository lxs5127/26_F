/**
 * @file    camera_uart.h
 * @brief   相机 UART3 数据帧接收 (MaixCam → 单片机)
 * @note    帧格式: d{偏差值}\n  (例如 "d12\n" 或 "d-5\n")
 *          偏差 = 目标中心X - 画面中心X
 *          收到有效帧后自动更新 g_cam_dx
 */

#ifndef __CAMERA_UART_H
#define __CAMERA_UART_H

#include "ti_msp_dl_config.h"
#include <stdint.h>

/**
 * @brief   UART3 接收中断处理函数
 */
void UART3_IRQHandler(void);

/**
 * @brief   轮询 UART3 RX (安全兜底)
 */
void CameraUART_Poll(void);

/**
 * @brief   向相机发送命令帧 (Q3 / Q4 / Q5 / Q6)
 * @param   cmd  命令编号: 3 ~ 6
 */
void CameraUART_SendCmd(uint8_t cmd);

/* 调试计数器 */
extern volatile uint16_t g_cam_rx_byte_cnt;
extern volatile uint16_t g_cam_frame_cnt;
extern volatile uint16_t g_cam_lost_frames;   /* 丢帧计数 (超时/溢出) */

#endif /* __CAMERA_UART_H */
