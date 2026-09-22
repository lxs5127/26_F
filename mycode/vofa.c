#include "vofa.h"
#include "stdio.h"
#include "string.h"



// 重定向fputc函数
int fputc(int ch, FILE *f)
{
    DL_UART_transmitDataBlocking(UART_vofa_INST, ch);
     return (ch);
}
// 重定向fputs函数
int fputs(const char* restrict s, FILE* restrict stream) 
{
    uint16_t i,len;
    len = strlen(s);
    for(i=0;i<len;i++)
    {
    DL_UART_transmitDataBlocking(UART_vofa_INST, s[i]);
    }
    return len;
}
// 重定向puts函数
int puts(const char* _ptr)
{
    int count = fputs(_ptr,stdout);
    count += fputs("\n",stdout);
    return count;
}

/* ==================== JustFloat 协议实现 ==================== */

/**
 * JustFloat 协议帧格式:
 *   fdata[0] | fdata[1] | ... | fdata[N-1] | tail[4]
 * 其中 tail = {0x00, 0x00, 0x80, 0x7f} 为帧结束标志
 * (即 IEEE 754 浮点 +inf 的小端编码)
 */
static const unsigned char vofa_tail[4] = {0x00, 0x00, 0x80, 0x7f};

static void vofa_write_buf(const uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        DL_UART_transmitDataBlocking(UART_vofa_INST, buf[i]);
    }
}

void VOFA_JustFloat_Send(float *ch)
{
    vofa_write_buf((uint8_t *)ch, sizeof(float) * VOFA_CH_COUNT);
    vofa_write_buf(vofa_tail, 4);
}

void VOFA_JustFloat_Send1(float d1)
{
    float ch[1] = {d1};
    vofa_write_buf((uint8_t *)ch, sizeof(float) * 1);
    vofa_write_buf(vofa_tail, 4);
}

void VOFA_JustFloat_Send2(float d1, float d2)
{
    float ch[2] = {d1, d2};
    vofa_write_buf((uint8_t *)ch, sizeof(float) * 2);
    vofa_write_buf(vofa_tail, 4);
}

void VOFA_JustFloat_Send3(float d1, float d2, float d3)
{
    float ch[3] = {d1, d2, d3};
    vofa_write_buf((uint8_t *)ch, sizeof(float) * 3);
    vofa_write_buf(vofa_tail, 4);
}

void VOFA_JustFloat_Send4(float d1, float d2, float d3, float d4)
{
    float ch[4] = {d1, d2, d3, d4};
    vofa_write_buf((uint8_t *)ch, sizeof(float) * 4);
    vofa_write_buf(vofa_tail, 4);
}


