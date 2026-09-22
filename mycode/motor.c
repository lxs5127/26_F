#include "motor.h"


void MOTOR_OFF(void)
{
    DL_TimerA_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C0_IDX);
    DL_TimerA_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C1_IDX);

    DL_GPIO_clearPins(GPIO_motor_PORT, GPIO_motor_PIN_AIN1_PIN);
    DL_GPIO_clearPins(GPIO_motor_PORT, GPIO_motor_PIN_AIN2_PIN);
    DL_GPIO_clearPins(GPIO_motor_PORT, GPIO_motor_PIN_BIN1_PIN);
    DL_GPIO_clearPins(GPIO_motor_PORT, GPIO_motor_PIN_BIN2_PIN);
}

void set_speed(float PWM_left, float PWM_right)
{
    uint32_t compValue;

    /* ====== 左轮 (先方向, 后 PWM, 避免 H 桥不定态) ====== */
    if (PWM_left < 0)
    {
        DL_GPIO_clearPins(GPIO_motor_PORT, GPIO_motor_PIN_AIN1_PIN);
        DL_GPIO_setPins(GPIO_motor_PORT, GPIO_motor_PIN_AIN2_PIN);
        compValue = (uint32_t)(-PWM_left);
        DL_TimerA_setCaptureCompareValue(PWM_0_INST, compValue, GPIO_PWM_0_C1_IDX);
    }
    else if (PWM_left > 0)
    {
        DL_GPIO_setPins(GPIO_motor_PORT, GPIO_motor_PIN_AIN1_PIN);
        DL_GPIO_clearPins(GPIO_motor_PORT, GPIO_motor_PIN_AIN2_PIN);
        compValue = (uint32_t)(PWM_left);
        DL_TimerA_setCaptureCompareValue(PWM_0_INST, compValue, GPIO_PWM_0_C1_IDX);
    }
    else 
    {
        DL_GPIO_setPins(GPIO_motor_PORT, GPIO_motor_PIN_AIN1_PIN);
        DL_GPIO_setPins(GPIO_motor_PORT, GPIO_motor_PIN_AIN2_PIN);
        DL_TimerA_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C1_IDX);
    }

    /* ====== 右轮 ====== */
    if (PWM_right < 0)
    {
        DL_GPIO_clearPins(GPIO_motor_PORT, GPIO_motor_PIN_BIN1_PIN);
        DL_GPIO_setPins(GPIO_motor_PORT, GPIO_motor_PIN_BIN2_PIN);
        compValue = (uint32_t)(-PWM_right);
        DL_TimerA_setCaptureCompareValue(PWM_0_INST, compValue, GPIO_PWM_0_C0_IDX);
    }
    else if (PWM_right > 0)
    {
        DL_GPIO_setPins(GPIO_motor_PORT, GPIO_motor_PIN_BIN1_PIN);
        DL_GPIO_clearPins(GPIO_motor_PORT, GPIO_motor_PIN_BIN2_PIN);
        compValue = (uint32_t)(PWM_right);
        DL_TimerA_setCaptureCompareValue(PWM_0_INST, compValue, GPIO_PWM_0_C0_IDX);
    }
    else 
    {
        DL_GPIO_setPins(GPIO_motor_PORT, GPIO_motor_PIN_BIN1_PIN);
        DL_GPIO_setPins(GPIO_motor_PORT, GPIO_motor_PIN_BIN2_PIN);
        DL_TimerA_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C0_IDX);
    }
}


