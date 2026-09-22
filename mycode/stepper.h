#ifndef __STEPPER_X42S_H
#define __STEPPER_X42S_H

#include "ti_msp_dl_config.h"
#include <stdbool.h>
#include <stdint.h>

/* ====== ��е������ģ���޹� PID ����Ҫ, �������ã� ====== */
#define BEAM_LEN_MM         250.0f
#define SCREW_PITCH_MM      2.0f
#define PIXELS_PER_CM       33.0f
#define CENTER_PIXEL        160.0f
#define DEG_PER_BEAM_DEG    (BEAM_LEN_MM * 6.283185f / SCREW_PITCH_MM)

/* ====== ���ò��� ====== */
#define STEPPER_ADDR        0x01        /* �����ַ */
#define STEPPER_SPEED       500         /* �ٶ� ��0.1RPM (500=50RPM) */
#define STEPPER_CURRENT     2000        /* CB���� mA (FDģʽ���Դ��ֶ�) */

/* ====== �������� ====== */

void STEPPER_UART_Init(void);
void STEPPER_Enable(void);
void STEPPER_Disable(void);
void STEPPER_ClearPosition(void);
void STEPPER_Stop(void);

/**
 * @brief FD ����λ��ģʽ (�ֲ� 5.3.10) �� �ʺϴ�Ƕ�һ�����ƶ�
 * @param dir      0=CW / 1=CCW
 * @param speed    �ٶ� ��0.1RPM
 * @param position λ�� ��0.1�� (3600=360��)
 * @param current  ���� mA (FD����, ��������)
 */
void STEPPER_CB_Absolute(uint8_t dir, uint16_t speed, uint32_t position, uint16_t current);
void STEPPER_CB_Relative(uint8_t dir, uint16_t speed, uint32_t position, uint16_t current);

/**
 * @brief CB ֱͨλ��ģʽ (�ֲ� 5.3.9) �� �޼Ӽ���, ƽ������, �ʺ� PID
 * @param dir      0=CW / 1=CCW
 * @param speed    �ٶ� ��0.1RPM
 * @param position λ�� ��0.1��
 * @param current  ���� mA (CB��Ч)
 */
void STEPPER_DirectAbs(uint8_t dir, uint16_t speed, uint32_t position, uint16_t current);

/**
 * @brief �Ƕȱ�ݺ��� �� ֱ�Ӵ��Ƕ�(��), �ڲ��Զ����㲢�з�
 * @param angle_deg  Ŀ��Ƕ�(��), �����=����, �����=����
 * @note  ʹ��Ĭ���ٶ� STEPPER_SPEED �͵��� STEPPER_CURRENT, CBֱͨģʽ
 */
void STEPPER_GoToAngle(float angle_deg);

void STEPPER_SendRaw(const uint8_t *data, uint8_t len);
int32_t STEPPER_ReadPosition(void);     /* ��ȡʵʱλ��(0.1��), ��ʱ <0 */

#endif /* __STEPPER_X42S_H */
