#ifndef __ENCODER_H
#define __ENCODER_H

#include "ti_msp_dl_config.h"

/* 编码器参数 (MG513P30, 减速比1:30, 13线霍尔编码器) */
#define ENCODER_LINES_PER_REV      390.0f     // 减速后编码器线数 = 13 * 30
#define ENCODER_4X_RESOLUTION      (ENCODER_LINES_PER_REV * 4.0f)  // 4倍频分辨率 = 1560

/* 轮子参数 (65mm直径) */
#define WHEEL_DIAMETER_CM          6.5f       // 轮子直径(cm)
#define WHEEL_CIRCUMFERENCE_CM     (3.14159265f * WHEEL_DIAMETER_CM)  // 轮子周长(cm)


/* 位置累积标定系数
 * 跑 30cm 实测 35cm → POSITION_SCALE = 30/35 ≈ 0.857
 */
#define POSITION_SCALE             1.01f

/* 速度换算宏
   encoder_left  = 每 10ms 的脉冲数
   用法: speed_cmps = SPEED_CMPS(encoder_left);   →  cm/s
         speed_mps  = SPEED_MPS(encoder_left);     →  m/s
         target = MS_TO_PULSE(0.5);                →  m/s → 脉冲/10ms
*/
#define SPEED_CMPS(encoder_val)    ((encoder_val) * 100.0f * WHEEL_CIRCUMFERENCE_CM / ENCODER_4X_RESOLUTION)
#define SPEED_MPS(encoder_val)     (SPEED_CMPS(encoder_val) / 100.0f)
#define MS_TO_PULSE(ms)            ((ms) * ENCODER_4X_RESOLUTION / WHEEL_CIRCUMFERENCE_CM)

void encoder_read(uint32_t pending);

#endif
