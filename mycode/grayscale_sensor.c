#include "grayscale_sensor.h"
uint16_t g_sensor_data[GRAYSCALE_SENSOR_CHANNELS];

//读取所有8个通道的灰度值（八路数字灰度，每个通道独立引脚直接读取）
void Grayscale_Sensor_Read_All(uint16_t* sensor_values)
{
    sensor_values[0] = SENSOR_OUT1_READ();
    sensor_values[1] = SENSOR_OUT2_READ();
    sensor_values[2] = SENSOR_OUT3_READ();
    sensor_values[3] = SENSOR_OUT4_READ();
    sensor_values[4] = SENSOR_OUT5_READ();
    sensor_values[5] = SENSOR_OUT6_READ();
    sensor_values[6] = SENSOR_OUT7_READ();
    sensor_values[7] = SENSOR_OUT8_READ();
}

//读取单个指定通道的灰度值
uint16_t Grayscale_Sensor_Read_Single(uint8_t channel)
{
    switch (channel)
    {
        case 0: return SENSOR_OUT1_READ();
        case 1: return SENSOR_OUT2_READ();
        case 2: return SENSOR_OUT3_READ();
        case 3: return SENSOR_OUT4_READ();
        case 4: return SENSOR_OUT5_READ();
        case 5: return SENSOR_OUT6_READ();
        case 6: return SENSOR_OUT7_READ();
        case 7: return SENSOR_OUT8_READ();
        default: return 0;
    }
}

