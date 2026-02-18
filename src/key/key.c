#include "key.h"
#include "../delay/delay.h"

#include "stm32f4xx.h"

static key_id_t key_read_now(void)
{
    /* 优先级：KEY_WKUP > KEY0 > KEY1 > KEY2 */
    if (READ_BIT(GPIOA->IDR, GPIO_IDR_ID0) != 0U)
    {
        return KEY_WKUP;
    }

    if (READ_BIT(GPIOE->IDR, GPIO_IDR_ID4) == 0U)
    {
        return KEY0;
    }

    if (READ_BIT(GPIOE->IDR, GPIO_IDR_ID3) == 0U)
    {
        return KEY1;
    }

    if (READ_BIT(GPIOE->IDR, GPIO_IDR_ID2) == 0U)
    {
        return KEY2;
    }

    return KEY_NONE;
}

void key_init(void)
{
    /* 1) 使能 GPIOA/GPIOE 时钟 */
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOEEN);
    (void)READ_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOEEN);

    /* 2) PA0: 输入模式 + 下拉 */
    CLEAR_BIT(GPIOA->MODER, GPIO_MODER_MODER0);
    MODIFY_REG(GPIOA->PUPDR, GPIO_PUPDR_PUPDR0, GPIO_PUPDR_PUPDR0_1);

    /* 3) PE2/PE3/PE4: 输入模式 + 上拉 */
    CLEAR_BIT(GPIOE->MODER, GPIO_MODER_MODER2 | GPIO_MODER_MODER3 | GPIO_MODER_MODER4);
    MODIFY_REG(GPIOE->PUPDR,
               GPIO_PUPDR_PUPDR2 | GPIO_PUPDR_PUPDR3 | GPIO_PUPDR_PUPDR4,
               GPIO_PUPDR_PUPDR2_0 | GPIO_PUPDR_PUPDR3_0 | GPIO_PUPDR_PUPDR4_0);
}

key_id_t key_scan(void)
{
    key_id_t key = key_read_now();

    if (key == KEY_NONE)
    {
        return KEY_NONE;
    }

    /* 软件消抖：检测到按下后等待 >=20ms 再确认 */
    delay_ms(20U);

    if (key_read_now() == key)
    {
        return key;
    }

    return KEY_NONE;
}

