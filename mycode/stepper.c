#include "stepper.h"
#include <ti/driverlib/dl_uart_main.h>
#include <ti/driverlib/m0p/dl_core.h>

/* ====== USART0 ��ʼ�� (SysConfig ������, �˴�����װ) ====== */
void STEPPER_UART_Init(void)
{
    /* SysConfig �� SYSCFG_DL_Stepper_UART_init() ���� SYSCFG_DL_init() �е��� */
    /* ����Ҫ������� */
}

/* ====== 底层发送 (阻塞模式, 确保每字节都发出) ====== */
void STEPPER_SendRaw(const uint8_t *data, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        DL_UART_Main_transmitDataBlocking(Stepper_UART_INST, data[i]);
    }
}

/* ====== �������������� ====== */
static void send_command(uint8_t func, const uint8_t *payload, uint8_t payload_len)
{
    uint8_t buf[32];
    uint8_t idx = 0;

    buf[idx++] = STEPPER_ADDR;      // ��ַ
    buf[idx++] = func;              // ������
    for (uint8_t i = 0; i < payload_len; i++) {
        buf[idx++] = payload[i];
    }
    /* �̶�У��: �ֲ�Ĭ��У�鷽ʽΪ�̶� 0x6B */
    buf[idx++] = 0x6B;

    STEPPER_SendRaw(buf, idx);
}

/* ====== ʹ�ܵ�� ====== */
void STEPPER_Enable(void)
{
    // 01 F3 AB 01 00 6B
    uint8_t payload[] = {0xAB, 0x01, 0x00};
    send_command(0xF3, payload, 3);
}

/* ====== ʧ�ܵ�� ====== */
void STEPPER_Disable(void)
{
    // 01 F3 AB 00 00 6B
    uint8_t payload[] = {0xAB, 0x00, 0x00};
    send_command(0xF3, payload, 3);
}

/* ====== ���λ�� ====== */
void STEPPER_ClearPosition(void)
{
    // 01 0A 6D 6B
    uint8_t payload[] = {0x6D};
    send_command(0x0A, payload, 1);
}

/* ====== ����ֹͣ ====== */
void STEPPER_Stop(void)
{
    // 01 FE 98 00 6B
    uint8_t payload[] = {0x98, 0x00};
    send_command(0xFE, payload, 2);
}

/* ====== FD ���μӼ���λ��ģʽ (�ֲ� 5.3.10) ======
 * 16�ֽ�: 01 FD Dir AccH AccL DecH DecL SpdH SpdL PosB3 B2 B1 B0 Mode Sync 6B
 * �мӼ���, ÿ����������ͣһ��, �ʺϴ�Ƕ�һ�����ƶ� */
static void stepper_fd(uint8_t dir, uint16_t speed, uint32_t position, uint8_t mode)
{
    uint8_t payload[13];

    payload[0]  = dir;
    payload[1]  = 0x03; payload[2] = 0xE8;               /* ���� 1000 RPM/s */
    payload[3]  = 0x03; payload[4] = 0xE8;               /* ���� 1000 RPM/s */
    payload[5]  = (uint8_t)(speed >> 8);
    payload[6]  = (uint8_t)(speed & 0xFF);
    payload[7]  = (uint8_t)(position >> 24);
    payload[8]  = (uint8_t)(position >> 16);
    payload[9]  = (uint8_t)(position >> 8);
    payload[10] = (uint8_t)(position & 0xFF);
    payload[11] = mode;                                  /* 01=���� */
    payload[12] = 0x00;                                  /* ����ִ�� */

    send_command(0xFD, payload, 13);
}

/* FD ����λ�� (current��������) */
void STEPPER_CB_Absolute(uint8_t dir, uint16_t speed, uint32_t position, uint16_t current)
{
    (void)current;
    stepper_fd(dir, speed, position, 0x01);
}

/* FD ���λ�� */
void STEPPER_CB_Relative(uint8_t dir, uint16_t speed, uint32_t position, uint16_t current)
{
    (void)current;
    stepper_fd(dir, speed, position, 0x00);
}

/* ====== CB ֱͨ����λ��ģʽ (�ֲ� 5.3.9) ======
 * 14�ֽ�: 01 CB Dir SpdH SpdL PosB3 B2 B1 B0 Mode Sync CurH CurL 6B
 * �޼Ӽ���, ���ֱ�Ӹ���Ŀ��, �ʺ�����λ�ø��� (��ƽ�� PID) */
static void stepper_cb(uint8_t dir, uint16_t speed, uint32_t position,
                       uint8_t mode, uint16_t current)
{
    uint8_t payload[11];

    payload[0]  = dir;
    payload[1]  = (uint8_t)(speed >> 8);
    payload[2]  = (uint8_t)(speed & 0xFF);
    payload[3]  = (uint8_t)(position >> 24);
    payload[4]  = (uint8_t)(position >> 16);
    payload[5]  = (uint8_t)(position >> 8);
    payload[6]  = (uint8_t)(position & 0xFF);
    payload[7]  = mode;                                  /* 01=���� */
    payload[8]  = 0x00;                                  /* ����ִ�� */
    payload[9]  = (uint8_t)(current >> 8);
    payload[10] = (uint8_t)(current & 0xFF);

    send_command(0xCB, payload, 11);
}

/* CB ֱͨ����λ�� (PID ר��) */
void STEPPER_DirectAbs(uint8_t dir, uint16_t speed, uint32_t position, uint16_t current)
{
    stepper_cb(dir, speed, position, 0x01, current);
}

/* ====== �Ƕȱ�ݺ�����ֱ�Ӵ��Ƕ�(��), �Զ�����Ϊ0.1�㵥λ���ж� ====== */
void STEPPER_GoToAngle(float angle_deg)
{
    uint8_t dir;
    uint32_t pos;

    if (angle_deg >= 0.0f) {
        dir = 0x00;   /* 0=CW 正向 */
        pos = (uint32_t)(angle_deg * 10.0f + 0.5f);
    } else {
        dir = 0x01;   /* 1=CCW 反向 */
        pos = (uint32_t)(-angle_deg * 10.0f + 0.5f);
    }

    stepper_cb(dir, STEPPER_SPEED, pos, 0x01, STEPPER_CURRENT);
}

/* ====== ��ȡ���ʵʱλ�� (�ֲ� 5.5.13) ======
 * ����: 01 36 6B
 * ����: 01 36 ���(00/01) B3 B2 B1 B0 6B (8�ֽ�), λ�õ�λ 0.1��
 * ����ֵ >= 0: �ɹ�, ����λ��(0.1��)
 * ����ֵ < 0:  ��ʱʧ�� */
int32_t STEPPER_ReadPosition(void)
{
    uint8_t i;
    uint8_t resp[7];

    /* ��� RX FIFO (����������) */
    while (!DL_UART_Main_isRXFIFOEmpty(Stepper_UART_INST)) {
        DL_UART_Main_receiveData(Stepper_UART_INST);
    }

    /* ���Ͳ�ѯ����: 01 33 6B */
    uint8_t cmd[] = {0x01, 0x33, 0x6B};
    STEPPER_SendRaw(cmd, 3);

    /* ��ѯ���� 7 �ֽ�, ��ʱ ~10ms */
    for (i = 0; i < 7; i++) {
        uint32_t timeout = 50000;  /* ~50k cycles �� 0.6ms at 80MHz */
        while (DL_UART_Main_isRXFIFOEmpty(Stepper_UART_INST) && --timeout) {}
        if (timeout == 0) return -1;
        resp[i] = DL_UART_Main_receiveData(Stepper_UART_INST);
    }

    /* У��֡ͷ */
    if (resp[0] != 0x01 || resp[1] != 0x33) return -2;

    /* ����λ�� (���, 0.1��) */
    int32_t pos = ((int32_t)resp[2] << 24) | ((int32_t)resp[3] << 16)
                | ((int32_t)resp[4] << 8)  |  (int32_t)resp[5];
    return pos;
}
