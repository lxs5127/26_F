#include "uart.h"

void UART_send_char(UART_Regs *uart, const uint8_t chr)
{
    DL_UART_transmitDataBlocking(uart, chr);
}

void UART_send_string(UART_Regs *uart, const char *str)
{
    while (*str) {
        UART_send_char(uart, (uint8_t) *str);
        str++;
    }
}

void UART_send_buffer(UART_Regs *uart, const uint8_t *buffer, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        UART_send_char(uart, buffer[i]);
    }
}


