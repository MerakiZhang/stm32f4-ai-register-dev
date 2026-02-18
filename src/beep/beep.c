#include "beep.h"

#include "stm32f4xx.h"

#define BEEP_PIN_MASK GPIO_ODR_OD8

void beep_init(void)
{
    /* 1) 使能 GPIOF 时钟 */
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOFEN);
    (void)READ_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOFEN);

    /* 2) PF8 配置为通用输出模式 (01) */
    MODIFY_REG(GPIOF->MODER, GPIO_MODER_MODER8, GPIO_MODER_MODER8_0);

    /* 3) 推挽输出 */
    CLEAR_BIT(GPIOF->OTYPER, GPIO_OTYPER_OT8);

    /* 4) 无上下拉 */
    CLEAR_BIT(GPIOF->PUPDR, GPIO_PUPDR_PUPDR8);

    /* 5) 默认关闭蜂鸣器：输出低电平 */
    SET_BIT(GPIOF->BSRR, GPIO_BSRR_BR8);
}

void beep_on(void)
{
    /* 高电平导通三极管，蜂鸣器响 */
    SET_BIT(GPIOF->BSRR, GPIO_BSRR_BS8);
}

void beep_off(void)
{
    /* 低电平截止三极管，蜂鸣器停 */
    SET_BIT(GPIOF->BSRR, GPIO_BSRR_BR8);
}

void beep_toggle(void)
{
    if ((READ_BIT(GPIOF->ODR, BEEP_PIN_MASK)) != 0U)
    {
        beep_off();
    }
    else
    {
        beep_on();
    }
}

