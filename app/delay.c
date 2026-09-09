//#include <stdint.h>
#include "stm32f10x.h"


void delay_us(uint32_t us)
{
    /* 使能SysTick，时钟源为AHB时钟，即CPU时钟，计数值为us * (CPU频率 / 1000000) - 1 */
    SysTick->LOAD = us * (SystemCoreClock / 1000000) - 1;
    SysTick->VAL = 0;   /* 清空计数值 */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; /* 使能SysTick，启动计数 */
    while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0); /* 等待计数完成 */
    SysTick->CTRL = 0;  /* 关闭SysTick */
}

void delay_ms(uint32_t ms)
{
    /* 循环调用微妙级延时函数，延时ms毫秒 */
    while (ms--)
    {
        delay_us(1000);
    }
}
