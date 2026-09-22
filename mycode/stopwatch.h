#ifndef __STOPWATCH_H
#define __STOPWATCH_H

#include "ti_msp_dl_config.h"

/**
 * @brief 初始化运行时钟（记录起始时间点）
 */
void Stopwatch_Init(void);

/**
 * @brief 获取从上电开始经过的时间
 * @param ms  输出毫秒 (0-999)
 * @param sec 输出秒   (0-59)
 * @param min 输出分钟 (>=0)
 */
void Stopwatch_GetTime(uint32_t *ms, uint32_t *sec, uint32_t *min);

/**
 * @brief 在 OLED 上显示运行时间
 * @param x   显示起始列
 * @param y   显示起始页
 */
void Stopwatch_Display(uint8_t x, uint8_t y);

#endif /* #ifndef __STOPWATCH_H */
