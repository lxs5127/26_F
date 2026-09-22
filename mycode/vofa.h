#ifndef __VOFA_H
#define __VOFA_H

#include "ti_msp_dl_config.h"
#include <stdint.h>

/* JustFloat 协议通道数量（根据实际需要修改） */
#define VOFA_CH_COUNT  4

/* JustFloat 协议数据帧结构 */
typedef struct {
    float fdata[VOFA_CH_COUNT];
    unsigned char tail[4];
} Vofa_JustFloat_Frame_t;

/**
 * @brief  使用 JustFloat 协议发送多通道 float 数据
 * @param  ch  通道数据数组（长度为 VOFA_CH_COUNT）
 */
void VOFA_JustFloat_Send(float *ch);

/**
 * @brief  使用 JustFloat 协议发送单通道 float 数据
 */
void VOFA_JustFloat_Send1(float data);

/**
 * @brief  使用 JustFloat 协议发送双通道 float 数据
 */
void VOFA_JustFloat_Send2(float d1, float d2);

/**
 * @brief  使用 JustFloat 协议发送三通道 float 数据
 */
void VOFA_JustFloat_Send3(float d1, float d2, float d3);

/**
 * @brief  使用 JustFloat 协议发送四通道 float 数据
 */
void VOFA_JustFloat_Send4(float d1, float d2, float d3, float d4);

#endif
