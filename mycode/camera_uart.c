/**
 * @file    camera_uart.c
 * @brief   相机 UART3 数据帧接收 — 收到 d{偏差}\n → 更新 g_cam_dx
 */

#include "camera_uart.h"

/* ==================== 外部变量 ==================== */

extern volatile int16_t g_cam_dx;
extern volatile uint32_t tick_ms;

/* 帧计数器 (调试用) */
volatile uint16_t g_cam_rx_byte_cnt = 0;
volatile uint16_t g_cam_frame_cnt   = 0;
/* 最近一次有效偏差帧的时间戳 (ms), 用于 Gimbal 相机断连保护 */
volatile uint32_t g_cam_last_update_ms = 0;

/* dx 跳变过滤: 相邻帧突变超过阈值 → 限幅, 拒绝鬼数据 */
#define MAX_DX_JUMP     30
static int16_t g_cam_dx_filtered = 0;

/* ==================== 帧解析状态机 ==================== */
#define CAM_BUF_SIZE    8
#define CAM_TIMEOUT_MS  100     /* 帧超时: 收到 'd' 后 100ms 内必须完整 */

static struct {
    uint8_t  state;
    char     buf[CAM_BUF_SIZE];
    uint8_t  idx;
} g_cam_uart = {0, {0}, 0};

static uint32_t g_cam_state_enter_ms = 0;   /* 进入 state 1 的时间戳 */
volatile uint16_t g_cam_lost_frames  = 0;   /* 丢帧计数 (超时/溢出) */

/* ==================== UART3 中断接收 ==================== */
void UART3_IRQHandler(void)
{
    while (!DL_UART_isRXFIFOEmpty(UART_camera_INST))
    {
        uint8_t ch = DL_UART_receiveData(UART_camera_INST);
        g_cam_rx_byte_cnt++;

        switch (g_cam_uart.state)
        {
        case 0:
            if (ch == 'd') {
                g_cam_uart.idx = 0;
                g_cam_uart.state = 1;
                g_cam_state_enter_ms = tick_ms;     /* 记录帧头时间 */
            }
            break;
        case 1:
            if (ch == '\n') {
                g_cam_uart.buf[g_cam_uart.idx] = '\0';
                if (g_cam_uart.idx > 0) {
                    int16_t dx = 0; uint8_t neg = 0, i = 0;
                    if (g_cam_uart.buf[0] == '-') { neg = 1; i = 1; }
                    for (; i < g_cam_uart.idx; i++) {
                        char c = g_cam_uart.buf[i];
                        if (c >= '0' && c <= '9') dx = dx * 10 + (c - '0');
                        else { dx = 0; break; }
                    }
                    if (neg) dx = -dx;
                    /* 跳变过滤: 相邻帧突变 >30px → 限幅, 拒绝鬼数据 */
                    {
                        int16_t diff = dx - g_cam_dx_filtered;
                        if      (diff >  MAX_DX_JUMP) dx = g_cam_dx_filtered + MAX_DX_JUMP;
                        else if (diff < -MAX_DX_JUMP) dx = g_cam_dx_filtered - MAX_DX_JUMP;
                        g_cam_dx_filtered = dx;
                    }
                    g_cam_dx = dx;          /* ← 核心: 更新偏差, Gimbal_Run 自动使用 */
                    g_cam_last_update_ms = tick_ms;   /* 记录有效帧时间戳 */
                    g_cam_frame_cnt++;
                }
                g_cam_uart.state = 0;
            }
            else if ((ch >= '0' && ch <= '9') || (ch == '-' && g_cam_uart.idx == 0)) {
                if (g_cam_uart.idx < CAM_BUF_SIZE - 1)
                    g_cam_uart.buf[g_cam_uart.idx++] = (char)ch;
                else {
                    g_cam_uart.state = 0;
                    g_cam_lost_frames++;            /* 缓冲区溢出 */
                }
            }
            else { g_cam_uart.state = 0; }
            break;
        default: g_cam_uart.state = 0; break;
        }
    }
}

/* ==================== 轮询安全兜底 ==================== */
void CameraUART_Poll(void)
{
    /* 帧超时保护: 收到 'd' 后 100ms 内未完成 → 丢弃残帧 */
    if (g_cam_uart.state == 1 &&
        (tick_ms - g_cam_state_enter_ms > CAM_TIMEOUT_MS)) {
        g_cam_uart.state = 0;
        g_cam_lost_frames++;
    }

    while (!DL_UART_isRXFIFOEmpty(UART_camera_INST))
        DL_UART_receiveData(UART_camera_INST);
}

/* ==================== 发送命令帧 ==================== */
/**
 * @brief   向相机发送命令帧 (Q3 / Q4 / Q5 / Q6)
 * @param   cmd  命令编号: 3 ~ 6
 * @note    发送格式: "Q3", "Q4", "Q5", "Q6"
 */
void CameraUART_SendCmd(uint8_t cmd)
{
    if (cmd < 3 || cmd > 6) return;

    DL_UART_transmitDataBlocking(UART_camera_INST, 'Q');
    DL_UART_transmitDataBlocking(UART_camera_INST, '0' + cmd);
    DL_UART_transmitDataBlocking(UART_camera_INST, '\n');
}
