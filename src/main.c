#include "clock/clock.h"
#include "delay/delay.h"

#include "led/led.h"
#include "beep/beep.h"
#include "key/key.h"

#include "lcd/lcd.h"

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
    lcd_panel_init();

    /* 上电后简单清屏+色块，便于验证窗口/填充 */
    lcd_clear((lcd_color565_t)0x0000); /* BLACK */
    lcd_fill_rect(0, 0, 80, 80, (lcd_color565_t)0xF800);   /* RED */
    lcd_fill_rect(80, 0, 80, 80, (lcd_color565_t)0x07E0);  /* GREEN */
    lcd_fill_rect(160, 0, 80, 80, (lcd_color565_t)0x001F); /* BLUE */

    /* 4) 简单跑灯/蜂鸣器验证 */
    while (1)
    {
        led_toggle(LED0);
        beep_toggle();
        (void)key_scan();

        /* 最小验证：周期性闪烁背光，确认 GPIO 与 FSMC 初始化不导致 HardFault */
        lcd_backlight_set((delay_get_tick() / 500U) % 2U == 0U);
        delay_ms(200);
    }
}
