#include "led.h"

#include "stm32f4xx.h"

#define LED0_PIN_MASK    GPIO_ODR_OD9
#define LED1_PIN_MASK    GPIO_ODR_OD10
#define LED_ALL_PIN_MASK (LED0_PIN_MASK | LED1_PIN_MASK)

static uint32_t led_get_pin_mask(led_id_t led_id)
{
    switch (led_id)
    {
        case LED0:
            return LED0_PIN_MASK;
        case LED1:
            return LED1_PIN_MASK;
        default:
            return 0U;
    }
}

void led_init(void)
{
    /* 1) 使能 GPIOF 时钟 */
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOFEN);
    (void)READ_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOFEN);

    /* 2) 配置 PF9/PF10 为通用输出模式 (01) */
    MODIFY_REG(GPIOF->MODER,
               GPIO_MODER_MODER9 | GPIO_MODER_MODER10,
               GPIO_MODER_MODER9_0 | GPIO_MODER_MODER10_0);

    /* 3) 推挽输出 */
    CLEAR_BIT(GPIOF->OTYPER, GPIO_OTYPER_OT9 | GPIO_OTYPER_OT10);

    /* 4) 无上下拉 */
    CLEAR_BIT(GPIOF->PUPDR, GPIO_PUPDR_PUPDR9 | GPIO_PUPDR_PUPDR10);

    /* 5) 中速输出 (01) */
    MODIFY_REG(GPIOF->OSPEEDR,
               GPIO_OSPEEDER_OSPEEDR9 | GPIO_OSPEEDER_OSPEEDR10,
               GPIO_OSPEEDER_OSPEEDR9_0 | GPIO_OSPEEDER_OSPEEDR10_0);

    /* 6) 默认熄灭（低电平点亮 => 输出高电平） */
    SET_BIT(GPIOF->BSRR, GPIO_BSRR_BS9 | GPIO_BSRR_BS10);
}

void led_on(led_id_t led_id)
{
    uint32_t pin_mask = led_get_pin_mask(led_id);
    if (pin_mask == 0U)
    {
        return;
    }

    /* 低电平点亮：写 BSRR 高 16 位复位引脚 */
    SET_BIT(GPIOF->BSRR, pin_mask << 16U);
}

void led_off(led_id_t led_id)
{
    uint32_t pin_mask = led_get_pin_mask(led_id);
    if (pin_mask == 0U)
    {
        return;
    }

    /* 高电平熄灭：写 BSRR 低 16 位置位引脚 */
    SET_BIT(GPIOF->BSRR, pin_mask);
}

void led_toggle(led_id_t led_id)
{
    uint32_t pin_mask = led_get_pin_mask(led_id);
    if (pin_mask == 0U)
    {
        return;
    }

    if ((READ_BIT(GPIOF->ODR, pin_mask)) != 0U)
    {
        /* 当前为高电平(灭) -> 置低(亮) */
        SET_BIT(GPIOF->BSRR, pin_mask << 16U);
    }
    else
    {
        /* 当前为低电平(亮) -> 置高(灭) */
        SET_BIT(GPIOF->BSRR, pin_mask);
    }
}

