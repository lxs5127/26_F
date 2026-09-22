#include "stopwatch.h"
#include "clock.h"       // 用于 tick_ms
#include "oled_software_i2c.h"  // OLED 显示
#include <stdio.h>

static volatile uint32_t stopwatch_base = 0;  // 上电时的 tick_ms 基准值

/* 显示缓冲区 */
static char sw_display_buf[16];

void Stopwatch_Init(void)
{
    /* 记录起始时刻，之后一直用 tick_ms - base 计算经过时间 */
    stopwatch_base = tick_ms;
}

void Stopwatch_GetTime(uint32_t *ms, uint32_t *sec, uint32_t *min)
{
    uint32_t total_ms = tick_ms - stopwatch_base;

    if (ms)  *ms  = total_ms % 1000;
    if (sec) *sec = (total_ms / 1000) % 60;
    if (min) *min = total_ms / 60000;
}

void Stopwatch_Display(uint8_t x, uint8_t y)
{
    uint32_t ms, sec, min;

    Stopwatch_GetTime(&ms, &sec, &min);

    /* 显示运行时间：MM:SS.mmm */
    sprintf(sw_display_buf, "%02lu:%02lu.%03lu", (unsigned long)min,
            (unsigned long)sec, (unsigned long)ms);
    OLED_ShowString(x, y, (uint8_t *)sw_display_buf, 8);
}
