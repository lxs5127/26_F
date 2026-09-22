#include "PID_DEBUG.h"
#include "string.h"
#include "stdlib.h"
#include <stdio.h>
#include "controll.h"
#include "encoder.h"
#include "uart.h"
#include "ball_ctrl.h"

/* 摄像头坐标偏差 (在 main.c 中定义, 用于模拟测试) */
extern volatile int16_t g_cam_dx;

#define RX_BUFF_SIZE 64
static uint8_t  rxBuff[RX_BUFF_SIZE];
static uint8_t  rx_Data;
static uint16_t rx_cnt = 0;
static uint8_t  rx_complete_flag = 0;
static uint16_t rx_timeout = 0;      /* 超时计数器 */

/**
 * 串口调参协议 (简化版)
 *
 *  格式:  #XX=值     ← 直接回车发送, 不用校验和
 *  终止:  回车(\r\n) 或 '!' 均可
 *  响应:  OK\r\n / ERR:01\r\n(格式) / ERR:03\r\n(未知)
 *
 *  全部命令:
 *  ───────────────────────────────────────
 *  速度环:  Lp Li Rp Ri Vp Vi Ms Aa
 *  航向:    Hy Hp Hd      转向: Ta Tp Td
 *  巡线:    Fp Fd Fg        位置: Sp Sd St Sr Sa Ss
 *  球平衡:  Bp Bi Bd Bl BL   模拟: Dx
 *
 *  球平衡参数说明:
 *    Bp=Kp   Bi=Ki   Bd=Kd      PID增益
 *    Bl=limit  输出限幅(°)   BL=i_limit  积分限幅
 *
 *  示例:
 *    #St=50 回车  → OK    走 50cm
 *    #Ms=0.2 回车 → OK    设速度 0.2m/s
 *    #Sp=0.3 回车 → OK    设位置环 P
 *    #Bp=0.1 回车 → OK    设球平衡 P
 *    #Bl=15  回车 → OK    设输出限幅 15°
 *    #Ta=90!      → OK    左转 90° (旧格式兼容)
 */

void ParsePIDData(void)
{
    if (!rx_complete_flag) {
        if (rx_cnt > 0) {
            /* 超时未收完: 若像有效命令(#XX=)则照样处理, 否则丢弃 */
            if (++rx_timeout > 500) {
                if (rx_cnt >= 4 && rxBuff[0] == '#') {
                    rxBuff[rx_cnt] = '\0';   /* 手动补结束符 */
                    rx_complete_flag = 1;
                } else {
                    rx_cnt = 0;
                }
                rx_timeout = 0;
            }
        }
        if (!rx_complete_flag) return;
    }

    rx_complete_flag = 0;

    /* 过滤空帧 */
    if (rx_cnt < 4) goto done;

    if (rxBuff[0] != '#') goto done;

    {
        char *equal_pos = strchr((const char *)rxBuff, '=');
        if (!equal_pos || (equal_pos - (char *)rxBuff) != 3) {
            printf("ERR:01\r\n"); /* 格式错误 */
            goto done;
        }

        /* ── 截断值串: '\r' '\n' '!' '*' 都是终止符 ── */
        char *end_ptr = equal_pos + 1;
        while (*end_ptr && *end_ptr != '\r' && *end_ptr != '\n'
               && *end_ptr != '!' && *end_ptr != '*') {
            end_ptr++;
        }
        *end_ptr = '\0';

        float data = atof((const char *)(equal_pos + 1));

        char c1 = rxBuff[1];
        char c2 = rxBuff[2];
        uint8_t ok = 1;

             if (c1 == 'L' && c2 == 'p') { pid_L.kpv = data; }
        else if (c1 == 'L' && c2 == 'i') { pid_L.kiv = data; }
        else if (c1 == 'R' && c2 == 'p') { pid_R.kpv = data; }
        else if (c1 == 'R' && c2 == 'i') { pid_R.kiv = data; }
        else if (c1 == 'V' && c2 == 'p') { pid_L.kpv = data; pid_R.kpv = data; }
        else if (c1 == 'V' && c2 == 'i') { pid_L.kiv = data; pid_R.kiv = data; }
        else if (c1 == 'M' && c2 == 's') { middle_speed = data; }
        else if (c1 == 'T' && c2 == 'a') { turn_angle(data); }
        else if (c1 == 'T' && c2 == 'p') { turn_ctrl.kpv = data; }
        else if (c1 == 'T' && c2 == 'd') { turn_ctrl.kdv = data; }
        else if (c1 == 'A' && c2 == 'a') { a = data; }
        else if (c1 == 'H' && c2 == 'y') { heading.active = (uint8_t)data; }
        else if (c1 == 'H' && c2 == 'p') { heading.kpv = data; }
        else if (c1 == 'H' && c2 == 'd') { heading.kdv = data; }
        else if (c1 == 'F' && c2 == 'p') { line_ctrl.kpv = data; }
        else if (c1 == 'F' && c2 == 'd') { line_ctrl.kdv = data; }
        else if (c1 == 'F' && c2 == 'g') { gyro_damp_k = data; }
        /* ── 模拟摄像头偏差 ── */
        else if (c1 == 'D' && c2 == 'x') {
            g_cam_dx = (int16_t)data;
            printf("Cam dx=%d\r\n", g_cam_dx);
        }
        /* ── 位置环 ── */
        else if (c1 == 'S' && c2 == 'p') { pos_ctrl.kpp = data; }
        else if (c1 == 'S' && c2 == 'd') { pos_ctrl.kdp = data; }
        else if (c1 == 'S' && c2 == 't') { set_target_position(data); }
        else if (c1 == 'S' && c2 == 'r') { if ((uint8_t)data == 1) reset_position(); }
        else if (c1 == 'S' && c2 == 'a') { pos_ctrl.arrive_threshold = data; }
        else if (c1 == 'S' && c2 == 's') { pos_ctrl.speed_limit = data; }
        /* ── 球平衡 PID (BallPID) ── */
        else if (c1 == 'B' && c2 == 'p') { ball_pid.kp = data; }
        else if (c1 == 'B' && c2 == 'i') { ball_pid.ki = data; }
        else if (c1 == 'B' && c2 == 'd') { ball_pid.kd = data; }
        else if (c1 == 'B' && c2 == 'l') { ball_pid.limit = data; }
        else if (c1 == 'B' && c2 == 'L') { ball_pid.i_limit = data; }
        else {
            ok = 0;
        }

        if (ok) {
            printf("OK\r\n");
        } else {
            printf("ERR:03 (%.2s)\r\n", &rxBuff[1]);
        }
    }

done:
    rx_cnt = 0;
    rx_complete_flag = 0;
}


/* ── UART2 (VOFA/调参) 中断接收 ──
 *   摄像头在 UART3 (camera_uart.c), 不经过此中断,
 *   这里只处理 VOFA 调参命令 */
void UART_vofa_INST_IRQHandler(void)
{
    while (!DL_UART_isRXFIFOEmpty(UART_vofa_INST))
    {
        rx_Data = DL_UART_receiveData(UART_vofa_INST);
        rx_timeout = 0;

        /* ════════════════════════════════════════════════
         *  PID 调参协议 (新: #XX=值*CK\r\n, 旧: #XX=值!)
         * ════════════════════════════════════════════════ */
        /* 过滤控制字符, 保留 '!'(旧兼容) '#' '\r' '\n' */
        if (rx_Data < ' ' && rx_Data != '\r' && rx_Data != '\n')
            continue;

        /* '#' 强制重新开始帧 (但上一帧未处理完则不覆盖) */
        if (rx_Data == '#') {
            if (!rx_complete_flag) {
                rx_cnt = 0;
            }
            /* else: 丢弃新来的 '#', 等主循环处理完当前帧 */
        }

        /* 缓冲区溢出保护 — 丢弃整帧 */
        if (rx_cnt >= RX_BUFF_SIZE - 1) {
            rx_cnt = 0;
            rx_complete_flag = 0;
            continue;
        }

        rxBuff[rx_cnt++] = rx_Data;

        /* 帧结束检测: '\n' 或 '!' */
        if (rx_Data == '\n' || rx_Data == '!') {
            /* 过滤末尾的 \r (来自 \r\n) */
            if (rx_cnt >= 2 && rxBuff[rx_cnt - 2] == '\r') {
                rxBuff[rx_cnt - 2] = '\0'; /* 去掉 \r */
                rx_cnt--;                   /* cnt 回退 */
            }
            rxBuff[rx_cnt] = '\0';
            rx_complete_flag = 1;
        }
    }
}












