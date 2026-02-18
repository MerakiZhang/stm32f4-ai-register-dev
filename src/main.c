#include "clock/clock.h"
#include "delay/delay.h"

#include "led/led.h"
#include "beep/beep.h"
#include "key/key.h"

int main(void)
{
    /* 1) 时钟树：HSE=8MHz -> HCLK=168MHz */
    if (clock_init_168mhz_hse8() != CLOCK_OK)
    {
        while (1)
        {
            /* 时钟配置失败，停在这里等待调试 */
        }
    }

    /* 2) SysTick 1ms 时基（必须在主频最终确定后配置） */
    delay_init();

    /* 3) 外设初始化 */
    led_init();
    beep_init();
    key_init();

    /* 4) 简单跑灯/蜂鸣器验证 */
    while (1)
    {
        led_toggle(LED0);
        beep_toggle();
        (void)key_scan();
        delay_ms(200);
    }
}
