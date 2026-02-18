#include "delay.h"

#include "stm32f4xx.h"

static volatile uint32_t system_tick_ms = 0U;

void delay_init(void)
{
    /* 以系统内核时钟配置 1ms SysTick 中断 */
    (void)SysTick_Config(SystemCoreClock / 1000U);
}

uint32_t delay_get_tick(void)
{
    return system_tick_ms;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = delay_get_tick();

    while ((delay_get_tick() - start) < ms)
    {
        /* 阻塞等待 */
    }
}

void SysTick_Handler(void)
{
    system_tick_ms++;
}

