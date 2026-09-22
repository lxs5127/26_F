/**
 * @file    test_stepper.c
 * @brief   ����������Գ��� (�������)
 *
 * ʹ�÷���:
 *   1. main.c �е� main() �ѱ� #if 0 ע��
 *   2. ��������, �򿪴������� 115200
 *   3. ���ͨ�����Ժ�ȡ��ע����һ��
 */

#include "ti_msp_dl_config.h"
#include "main.h"
#include "stdio.h"
#include "clock.h"
#include "interrupt.h"   /* ��Ҫ�����ʹ��UART2�ж�! */
#include "ball_ctrl.h"
#include "stepper.h"

extern uint8_t oled_buffer[32];

#define TEST_SPEED      3000
#define TEST_CURRENT    1500

static void test_delay(uint32_t ms)
{
    uint32_t start = tick_ms;
    while ((tick_ms - start) < ms);
}

static void oled_line(uint8_t line, const char *str)
{
    OLED_ShowString(0, line, (uint8_t *)str, 8);
}

/* ====== ������ (����ʱȡ��ע��, �ճ���main.c�����) ====== */
#if 0
int main(void)
{
    SYSCFG_DL_init();
    SysTick_Init();
    Interrupt_Init();

    OLED_Init();
    oled_line(0, "STEPPER TEST");
    test_delay(500);

    /* 1. ʹ�ܵ�� 01 F3 AB 01 00 6B */
    {
        uint8_t cmd[] = {0x01, 0xF3, 0xAB, 0x01, 0x00, 0x6B};
        STEPPER_SendRaw(cmd, 6);
    }
    printf(">> Enable sent\r\n");
    oled_line(0, "EN");
    test_delay(500);

    /* 2. �ٶ�ģʽ: CW 300RPM, accel=1000RPM/S */
    /*    01 F6 00 03 E8 0B B8 00 6B */
    {
        uint8_t cmd[] = {0x01, 0xF6, 0x00, 0x03, 0xE8, 0x0B, 0xB8, 0x00, 0x6B};
        STEPPER_SendRaw(cmd, 9);
    }
    printf(">> Speed 300RPM CW 3sec\r\n");
    oled_line(0, "SPD 300");
    test_delay(3000);

    /* 3. ֹͣ 01 F6 00 03 E8 00 00 00 6B */
    {
        uint8_t cmd[] = {0x01, 0xF6, 0x00, 0x03, 0xE8, 0x00, 0x00, 0x00, 0x6B};
        STEPPER_SendRaw(cmd, 9);
    }
    printf(">> Stop\r\n");
    test_delay(1000);

    /* 4. ����: CCW 300RPM  01 F6 01 03 E8 0B B8 00 6B */
    {
        uint8_t cmd[] = {0x01, 0xF6, 0x01, 0x03, 0xE8, 0x0B, 0xB8, 0x00, 0x6B};
        STEPPER_SendRaw(cmd, 9);
    }
    printf(">> Speed 300RPM CCW 3sec\r\n");
    oled_line(0, "SPD-300");
    test_delay(3000);

    /* 5. ֹͣ */
    {
        uint8_t cmd[] = {0x01, 0xF6, 0x01, 0x03, 0xE8, 0x00, 0x00, 0x00, 0x6B};
        STEPPER_SendRaw(cmd, 9);
    }
    printf(">> DONE\r\n");
    oled_line(0, "DONE");

    while (1) {
        ParsePIDData();
        test_delay(100);
    }
}
#endif /* #if 0 - test_stepper main */
