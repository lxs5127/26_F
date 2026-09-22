#include "encoder.h"
#include "vofa.h"


volatile float encoder_left=0,encoder_right=0;
volatile float encoder_left_temp=0,encoder_right_temp=0;

void encoder_read(uint32_t pending)
{
    uint32_t pinA, pinB;

    /* ======== 左编码器 - 4倍频正交解码 ======== */
    if (pending & GPIO_encoder_PIN_encoder_left_A_PIN)
    {
        pinA = !!DL_GPIO_readPins(GPIO_encoder_PORT, GPIO_encoder_PIN_encoder_left_A_PIN);
        pinB = !!DL_GPIO_readPins(GPIO_encoder_PORT, GPIO_encoder_PIN_encoder_left_B_PIN);
        if (pinA != pinB)
            encoder_left_temp++;
        else
            encoder_left_temp--;
    }
    if (pending & GPIO_encoder_PIN_encoder_left_B_PIN)
    {
        pinA = !!DL_GPIO_readPins(GPIO_encoder_PORT, GPIO_encoder_PIN_encoder_left_A_PIN);
        pinB = !!DL_GPIO_readPins(GPIO_encoder_PORT, GPIO_encoder_PIN_encoder_left_B_PIN);
        if (pinA == pinB)
            encoder_left_temp++;
        else
            encoder_left_temp--;
    }

    /* ======== 右编码器 - 4倍频正交解码 ======== */
    if (pending & GPIO_encoder_PIN_encoder_right_A_PIN)
    {
        pinA = !!DL_GPIO_readPins(GPIO_encoder_PORT, GPIO_encoder_PIN_encoder_right_A_PIN);
        pinB = !!DL_GPIO_readPins(GPIO_encoder_PORT, GPIO_encoder_PIN_encoder_right_B_PIN);
        if (pinA != pinB)
            encoder_right_temp--;
        else
            encoder_right_temp++;
    }
    if (pending & GPIO_encoder_PIN_encoder_right_B_PIN)
    {
        pinA = !!DL_GPIO_readPins(GPIO_encoder_PORT, GPIO_encoder_PIN_encoder_right_A_PIN);
        pinB = !!DL_GPIO_readPins(GPIO_encoder_PORT, GPIO_encoder_PIN_encoder_right_B_PIN);
        if (pinA == pinB)
            encoder_right_temp--;
        else
            encoder_right_temp++;
    }
}



/* 编码器脉冲/10ms 物理上限 (1m/s ≈ 76, 2倍安全余量) */
#define ENCODER_MAX_10MS   150

void TIMER_read_INST_IRQHandler(void)
{
    float el = encoder_left_temp;
    float er = encoder_right_temp;

    /* 限幅: 超出物理极限的计数值视为噪声, 直接丢弃 */
    if (el >  ENCODER_MAX_10MS) el =  ENCODER_MAX_10MS;
    if (el < -ENCODER_MAX_10MS) el = -ENCODER_MAX_10MS;
    if (er >  ENCODER_MAX_10MS) er =  ENCODER_MAX_10MS;
    if (er < -ENCODER_MAX_10MS) er = -ENCODER_MAX_10MS;

    encoder_left  = el;
    encoder_right = er;

    encoder_right_temp = 0;
    encoder_left_temp  = 0;
}




