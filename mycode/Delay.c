#include "Delay.h"

void Delay(uint16_t ms)
{
    delay_cycles(CPUCLK_FREQ / 1000 * ms);
}


void delay_us(unsigned long __us) 
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 38;

    ticks = __us * (32000000 / 1000000);

    told = SysTick->VAL;

    while (1)
    {
        tnow = SysTick->VAL;

        if (tnow != told)
        {
            if (tnow < told)
                tcnt += told - tnow;
            else
                tcnt += SysTick->LOAD - tnow + told;

            told = tnow;

            if (tcnt >= ticks)
                break;
        }
    }
}
