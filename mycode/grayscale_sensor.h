#ifndef __GRAYSCALE_SENSOR_H
#define __GRAYSCALE_SENSOR_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

//=====================================================================================
//  引脚配置接口 (Pin Configuration) — 对应 SysConfig GPIO_trace 组
//  八路数字灰度模块：8 路独立数字输出 (PIN_OUT1~PIN_OUT8)
//=====================================================================================
#define GRAY_OUT1_PORT          GPIO_trace_PIN_OUT1_PORT        // GPIOB
#define GRAY_OUT1_PIN           GPIO_trace_PIN_OUT1_PIN         // DL_GPIO_PIN_19
#define GRAY_OUT2_PORT          GPIO_trace_PIN_OUT2_PORT        // GPIOB
#define GRAY_OUT2_PIN           GPIO_trace_PIN_OUT2_PIN         // DL_GPIO_PIN_17
#define GRAY_OUT3_PORT          GPIO_trace_PIN_OUT3_PORT        // GPIOA
#define GRAY_OUT3_PIN           GPIO_trace_PIN_OUT3_PIN         // DL_GPIO_PIN_16
#define GRAY_OUT4_PORT          GPIO_trace_PIN_OUT4_PORT        // GPIOA
#define GRAY_OUT4_PIN           GPIO_trace_PIN_OUT4_PIN         // DL_GPIO_PIN_14
#define GRAY_OUT5_PORT          GPIO_trace_PIN_OUT5_PORT        // GPIOB
#define GRAY_OUT5_PIN           GPIO_trace_PIN_OUT5_PIN         // DL_GPIO_PIN_20
#define GRAY_OUT6_PORT          GPIO_trace_PIN_OUT6_PORT        // GPIOB
#define GRAY_OUT6_PIN           GPIO_trace_PIN_OUT6_PIN         // DL_GPIO_PIN_25
#define GRAY_OUT7_PORT          GPIO_trace_PIN_OUT7_PORT        // GPIOA
#define GRAY_OUT7_PIN           GPIO_trace_PIN_OUT7_PIN         // DL_GPIO_PIN_25
#define GRAY_OUT8_PORT          GPIO_trace_PIN_OUT8_PORT        // GPIOA
#define GRAY_OUT8_PIN           GPIO_trace_PIN_OUT8_PIN         // DL_GPIO_PIN_27

//=====================================================================================
//  数字读取宏 (Digital Read Macros) — 每个通道独立读取
//=====================================================================================
#define SENSOR_OUT1_READ()      (!!(DL_GPIO_readPins(GRAY_OUT1_PORT, GRAY_OUT1_PIN)))
#define SENSOR_OUT2_READ()      (!!(DL_GPIO_readPins(GRAY_OUT2_PORT, GRAY_OUT2_PIN)))
#define SENSOR_OUT3_READ()      (!!(DL_GPIO_readPins(GRAY_OUT3_PORT, GRAY_OUT3_PIN)))
#define SENSOR_OUT4_READ()      (!!(DL_GPIO_readPins(GRAY_OUT4_PORT, GRAY_OUT4_PIN)))
#define SENSOR_OUT5_READ()      (!!(DL_GPIO_readPins(GRAY_OUT5_PORT, GRAY_OUT5_PIN)))
#define SENSOR_OUT6_READ()      (!!(DL_GPIO_readPins(GRAY_OUT6_PORT, GRAY_OUT6_PIN)))
#define SENSOR_OUT7_READ()      (!!(DL_GPIO_readPins(GRAY_OUT7_PORT, GRAY_OUT7_PIN)))
#define SENSOR_OUT8_READ()      (!!(DL_GPIO_readPins(GRAY_OUT8_PORT, GRAY_OUT8_PIN)))

//=====================================================================================
//  驱动函数接口 (Driver API)
//=====================================================================================

#define GRAYSCALE_SENSOR_CHANNELS   8   // 传感器通道总数 Number of sensor channels

extern uint16_t g_sensor_data[GRAYSCALE_SENSOR_CHANNELS];

void Grayscale_Sensor_Read_All(uint16_t* sensor_values);
uint16_t Grayscale_Sensor_Read_Single(uint8_t channel);

#endif // __GRAYSCALE_SENSOR_H
