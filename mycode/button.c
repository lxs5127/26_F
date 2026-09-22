#include "button.h"

/* 复位归位标志 (在 main.c 中定义, 用于复位时回到步进电机默认位置) */
extern volatile uint8_t g_home_request;

volatile TaskMode g_task       = TASK_NONE;
volatile uint8_t  g_start_flag = 0;

/* ====== 任务名称表 (OLED 显示用) ====== */
const char* task_name(TaskMode t)
{
    static const char* names[] = {
        [TASK_NONE] = "Idle",
        [TASK_1]    = "Task 1",
        [TASK_2]    = "Task 2",
        [TASK_3]    = "Task 3",
        [TASK_4]    = "Task 4",
        [TASK_5]    = "Task 5",
        [TASK_6]    = "Task 6",
    };
    if (t >= TASK_COUNT) return "???";
    return names[t];
}

/* 串扰免疫去抖: 计数器 + done 锁 */
#define DEBOUNCE_CNT  6   /* 连续 N 次稳定低电平才确认 (与主循环频率相关) */
static uint8_t k1_cnt = 0, k2_cnt = 0, k3_cnt = 0;
static uint8_t k1_done = 0, k2_done = 0, k3_done = 0;

void read_button(void)
{
    uint8_t k1 = !(DL_GPIO_readPins(GPIO_KEY_PIN_KEY1_PORT, GPIO_KEY_PIN_KEY1_PIN));
    uint8_t k2 = !(DL_GPIO_readPins(GPIO_KEY_PIN_KEY2_PORT, GPIO_KEY_PIN_KEY2_PIN));
    uint8_t k3 = !(DL_GPIO_readPins(GPIO_KEY_PIN_KEY3_PORT, GPIO_KEY_PIN_KEY3_PIN));

    /* KEY1/KEY2 互斥: 同时低 → 串扰 → 两边清零 */
    if (k1 && k2) {
        k1_cnt = 0; k2_cnt = 0;
    } else {
        if (k1) { if (k1_cnt < 255) k1_cnt++; } else k1_cnt = 0;
        if (k2) { if (k2_cnt < 255) k2_cnt++; } else k2_cnt = 0;
    }
    if (k3) { if (k3_cnt < 255) k3_cnt++; } else k3_cnt = 0;

    /* KEY1: 复位 */
    if (k1_cnt >= DEBOUNCE_CNT && !k1_done) {
        k1_done = 1;
        g_task = TASK_NONE;
        g_start_flag = 0;
        g_home_request = 1;
    }
    if (!k1 && k1_done) k1_done = 0;

    /* KEY2: 切任务 */
    if (k2_cnt >= DEBOUNCE_CNT && !k2_done) {
        k2_done = 1;
        if (!g_start_flag) g_task = (g_task + 1) % TASK_COUNT;
    }
    if (!k2 && k2_done) k2_done = 0;

    /* KEY3: 启动 */
    if (k3_cnt >= DEBOUNCE_CNT && !k3_done) {
        k3_done = 1;
        if (!g_start_flag && g_task != TASK_NONE) g_start_flag = 1;
    }
    if (!k3 && k3_done) k3_done = 0;
}



