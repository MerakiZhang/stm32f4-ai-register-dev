#include "lcd.h"

#include "stm32f4xx.h"

#include "delay/delay.h"

/* 兼容：部分 CMSIS 头文件未提供 DSB/ISB 内联函数时，回退到汇编屏障 */
#ifndef __DSB
#define __DSB() __asm volatile("dsb 0xF" ::: "memory")
#endif
#ifndef __ISB
#define __ISB() __asm volatile("isb 0xF" ::: "memory")
#endif

/* ----------------------------- FSMC 地址映射 ----------------------------- */
/*
 * Bank1 NE4 基地址：0x6C000000
 * RS(D/C) 接 FSMC_A6；16-bit 模式下 A6 对应 MCU 地址 bit7 -> 偏移 0x80
 * 详见: src/lcd/lcd.md
 */
#define LCD_FSMC_NE4_BASE (0x6C000000UL)
#define LCD_FSMC_RS_OFFS  (0x00000080UL)

#define LCD_CMD_REG  (*((volatile uint16_t *)(LCD_FSMC_NE4_BASE + 0U)))
#define LCD_DATA_REG (*((volatile uint16_t *)(LCD_FSMC_NE4_BASE + LCD_FSMC_RS_OFFS)))

/* ----------------------------- 时序可调宏 ----------------------------- */
/*
 * 说明：这些参数是以 HCLK=168MHz 为目标的“保守值”。
 * FSMC_BTRx：
 *   ADDSET[3:0], ADDHLD[3:0], DATAST[7:0], BUSTURN[3:0], ACCMOD[1:0]
 */
#ifndef LCD_FSMC_READ_ADDSET
#define LCD_FSMC_READ_ADDSET (0x0FU)
#endif
#ifndef LCD_FSMC_READ_ADDHLD
#define LCD_FSMC_READ_ADDHLD (0x00U)
#endif
#ifndef LCD_FSMC_READ_DATAST
#define LCD_FSMC_READ_DATAST (0x60U)
#endif
#ifndef LCD_FSMC_READ_BUSTURN
#define LCD_FSMC_READ_BUSTURN (0x00U)
#endif
#ifndef LCD_FSMC_READ_ACCMOD
#define LCD_FSMC_READ_ACCMOD (0x00U) /* Mode A */
#endif

#ifndef LCD_FSMC_WRITE_ADDSET
#define LCD_FSMC_WRITE_ADDSET (0x0FU)
#endif
#ifndef LCD_FSMC_WRITE_ADDHLD
#define LCD_FSMC_WRITE_ADDHLD (0x00U)
#endif
#ifndef LCD_FSMC_WRITE_DATAST
#define LCD_FSMC_WRITE_DATAST (0x10U)
#endif
#ifndef LCD_FSMC_WRITE_BUSTURN
#define LCD_FSMC_WRITE_BUSTURN (0x00U)
#endif
#ifndef LCD_FSMC_WRITE_ACCMOD
#define LCD_FSMC_WRITE_ACCMOD (0x00U) /* Mode A */
#endif

/* ----------------------------- GPIO 工具函数 ----------------------------- */
static void gpio_set_af(GPIO_TypeDef *port, uint8_t pin, uint8_t af)
{
    const uint32_t shift = ((uint32_t)(pin & 0x7U) * 4U);
    const uint32_t mask  = (0xFUL << shift);
    const uint32_t val   = ((uint32_t)(af & 0xFU) << shift);

    if (pin < 8U)
    {
        MODIFY_REG(port->AFR[0], mask, val);
    }
    else
    {
        MODIFY_REG(port->AFR[1], mask, val);
    }
}

static void gpio_config_af_pp_vhigh_nopull(GPIO_TypeDef *port, uint8_t pin, uint8_t af)
{
    /* MODER: Alternate Function(10) */
    MODIFY_REG(port->MODER, (0x3UL << (pin * 2U)), (0x2UL << (pin * 2U)));

    /* OTYPER: Push-Pull(0) */
    CLEAR_BIT(port->OTYPER, (1UL << pin));

    /* OSPEEDR: Very High(11) */
    MODIFY_REG(port->OSPEEDR, (0x3UL << (pin * 2U)), (0x3UL << (pin * 2U)));

    /* PUPDR: NoPull(00) */
    CLEAR_BIT(port->PUPDR, (0x3UL << (pin * 2U)));

    gpio_set_af(port, pin, af);
}

static void gpio_config_output_pp_nopull(GPIO_TypeDef *port, uint8_t pin)
{
    /* MODER: Output(01) */
    MODIFY_REG(port->MODER, (0x3UL << (pin * 2U)), (0x1UL << (pin * 2U)));

    /* OTYPER: Push-Pull(0) */
    CLEAR_BIT(port->OTYPER, (1UL << pin));

    /* OSPEEDR: Medium(01) */
    MODIFY_REG(port->OSPEEDR, (0x3UL << (pin * 2U)), (0x1UL << (pin * 2U)));

    /* PUPDR: NoPull(00) */
    CLEAR_BIT(port->PUPDR, (0x3UL << (pin * 2U)));
}

/* ----------------------------- 底层初始化 ----------------------------- */
static void lcd_gpio_init(void)
{
    /* 使能 GPIO 时钟：D/E/F/G + B */
    SET_BIT(RCC->AHB1ENR,
            RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOBEN);
    (void)READ_BIT(RCC->AHB1ENR,
                   RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                       RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOBEN);

    /* FSMC(AF12) 引脚配置：见 src/lcd/lcd.md */
    /* GPIOD: PD0, PD1, PD4, PD5, PD8, PD9, PD10, PD14, PD15 */
    gpio_config_af_pp_vhigh_nopull(GPIOD, 0U, 12U);
    gpio_config_af_pp_vhigh_nopull(GPIOD, 1U, 12U);
    gpio_config_af_pp_vhigh_nopull(GPIOD, 4U, 12U);
    gpio_config_af_pp_vhigh_nopull(GPIOD, 5U, 12U);
    gpio_config_af_pp_vhigh_nopull(GPIOD, 8U, 12U);
    gpio_config_af_pp_vhigh_nopull(GPIOD, 9U, 12U);
    gpio_config_af_pp_vhigh_nopull(GPIOD, 10U, 12U);
    gpio_config_af_pp_vhigh_nopull(GPIOD, 14U, 12U);
    gpio_config_af_pp_vhigh_nopull(GPIOD, 15U, 12U);

    /* GPIOE: PE7..PE15 */
    for (uint8_t pin = 7U; pin <= 15U; pin++)
    {
        gpio_config_af_pp_vhigh_nopull(GPIOE, pin, 12U);
    }

    /* GPIOF: PF12 (A6) */
    gpio_config_af_pp_vhigh_nopull(GPIOF, 12U, 12U);

    /* GPIOG: PG12 (NE4) */
    gpio_config_af_pp_vhigh_nopull(GPIOG, 12U, 12U);

    /* LCD_RESET: PD3 普通输出 */
    gpio_config_output_pp_nopull(GPIOD, 3U);
    /* 默认释放复位（高电平） */
    SET_BIT(GPIOD->BSRR, (1UL << 3U));

    /* LCD_BL: PB15 普通输出 */
    gpio_config_output_pp_nopull(GPIOB, 15U);
    /* 默认打开背光（高电平） */
    SET_BIT(GPIOB->BSRR, (1UL << 15U));
}

static uint32_t fsmc_build_btr(uint32_t addset,
                               uint32_t addhld,
                               uint32_t datast,
                               uint32_t busturn,
                               uint32_t accmod)
{
    return ((addset & 0x0FUL) << FSMC_BTR1_ADDSET_Pos) |
           ((addhld & 0x0FUL) << FSMC_BTR1_ADDHLD_Pos) |
           ((datast & 0xFFUL) << FSMC_BTR1_DATAST_Pos) |
           ((busturn & 0x0FUL) << FSMC_BTR1_BUSTURN_Pos) |
           ((accmod & 0x03UL) << FSMC_BTR1_ACCMOD_Pos);
}

static void lcd_fsmc_init(void)
{
    /* 使能 FSMC 时钟 */
    SET_BIT(RCC->AHB3ENR, RCC_AHB3ENR_FSMCEN);
    (void)READ_BIT(RCC->AHB3ENR, RCC_AHB3ENR_FSMCEN);

    /* Bank1 NOR/SRAM4: BTCR[6]=BCR4, BTCR[7]=BTR4 */
    /* 先关闭 bank 再配置 */
    CLEAR_BIT(FSMC_Bank1->BTCR[6], FSMC_BCR4_MBKEN);

    const uint32_t btr_read = fsmc_build_btr(LCD_FSMC_READ_ADDSET,
                                             LCD_FSMC_READ_ADDHLD,
                                             LCD_FSMC_READ_DATAST,
                                             LCD_FSMC_READ_BUSTURN,
                                             LCD_FSMC_READ_ACCMOD);
    const uint32_t btr_write = fsmc_build_btr(LCD_FSMC_WRITE_ADDSET,
                                              LCD_FSMC_WRITE_ADDHLD,
                                              LCD_FSMC_WRITE_DATAST,
                                              LCD_FSMC_WRITE_BUSTURN,
                                              LCD_FSMC_WRITE_ACCMOD);

    /* BCR4:
     * - MUXEN=0 (地址/数据不复用)
     * - MTYP=00 (SRAM)
     * - MWID=01 (16-bit)
     * - WREN=1 (允许写)
     * - EXTMOD=1 (分离读写时序，写时序使用 BWTR)
     */
    uint32_t bcr = 0U;
    bcr |= FSMC_BCR4_MWID_0; /* 16-bit */
    bcr |= FSMC_BCR4_WREN;
    bcr |= FSMC_BCR4_EXTMOD;

    FSMC_Bank1->BTCR[6] = bcr; /* MBKEN=0 */
    FSMC_Bank1->BTCR[7] = btr_read;
    FSMC_Bank1E->BWTR[3] = btr_write;

    /* 使能 bank */
    SET_BIT(FSMC_Bank1->BTCR[6], FSMC_BCR4_MBKEN);

    __DSB();
    __ISB();
}

/* ----------------------------- 对外 API ----------------------------- */
void lcd_reset_assert(void)
{
    /* PD3 = 0 */
    SET_BIT(GPIOD->BSRR, (1UL << (3U + 16U)));
}

void lcd_reset_deassert(void)
{
    /* PD3 = 1 */
    SET_BIT(GPIOD->BSRR, (1UL << 3U));
}

void lcd_reset_pulse(void)
{
    lcd_reset_assert();
    delay_ms(20U);
    lcd_reset_deassert();
    delay_ms(50U);
}

void lcd_backlight_on(void)
{
    /* PB15 = 1 */
    SET_BIT(GPIOB->BSRR, (1UL << 15U));
}

void lcd_backlight_off(void)
{
    /* PB15 = 0 */
    SET_BIT(GPIOB->BSRR, (1UL << (15U + 16U)));
}

void lcd_backlight_set(bool on)
{
    if (on)
    {
        lcd_backlight_on();
    }
    else
    {
        lcd_backlight_off();
    }
}

void lcd_write_cmd(uint16_t cmd)
{
    LCD_CMD_REG = cmd;
}

void lcd_write_data(uint16_t data)
{
    LCD_DATA_REG = data;
}

uint16_t lcd_read_data(void)
{
    return LCD_DATA_REG;
}

uint16_t lcd_read_data_dummy(void)
{
    (void)LCD_DATA_REG;
    return LCD_DATA_REG;
}

void lcd_init(void)
{
    lcd_gpio_init();
    lcd_fsmc_init();

    /* 复位 + 背光默认打开 */
    lcd_backlight_on();
    lcd_reset_pulse();
}

/* ============================= 中上层（HX8357D） ============================= */

static inline void lcd_write_u8(uint8_t v)
{
    /* HX8357D 的一些寄存器在参考代码里是按 8-bit 数据写入。
     * 我们走 8080-16bit 总线时，直接把 8-bit 放在低 8 位写即可。
     */
    lcd_write_data((uint16_t)v);
}

static bool lcd_clip_rect_u16(uint16_t *x,
                              uint16_t *y,
                              uint16_t *w,
                              uint16_t *h,
                              uint16_t max_w,
                              uint16_t max_h)
{
    if ((*w == 0U) || (*h == 0U))
    {
        return false;
    }

    uint32_t x0 = *x;
    uint32_t y0 = *y;
    uint32_t x1 = (uint32_t)(*x) + (uint32_t)(*w) - 1U;
    uint32_t y1 = (uint32_t)(*y) + (uint32_t)(*h) - 1U;

    if (x0 >= max_w || y0 >= max_h)
    {
        return false;
    }

    if (x1 >= max_w)
    {
        x1 = (uint32_t)max_w - 1U;
    }
    if (y1 >= max_h)
    {
        y1 = (uint32_t)max_h - 1U;
    }

    *x = (uint16_t)x0;
    *y = (uint16_t)y0;
    *w = (uint16_t)(x1 - x0 + 1U);
    *h = (uint16_t)(y1 - y0 + 1U);
    return true;
}

void lcd_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    if (!lcd_clip_rect_u16(&x, &y, &w, &h, (uint16_t)LCD_PANEL_WIDTH, (uint16_t)LCD_PANEL_HEIGHT))
    {
        return;
    }

    const uint16_t x0 = x;
    const uint16_t y0 = y;
    const uint16_t x1 = (uint16_t)(x + w - 1U);
    const uint16_t y1 = (uint16_t)(y + h - 1U);

    /* Column Address Set (0x2A): x0..x1 */
    lcd_write_cmd(0x2A);
    lcd_write_u8((uint8_t)(x0 >> 8));
    lcd_write_u8((uint8_t)(x0 & 0xFFU));
    lcd_write_u8((uint8_t)(x1 >> 8));
    lcd_write_u8((uint8_t)(x1 & 0xFFU));

    /* Page Address Set (0x2B): y0..y1 */
    lcd_write_cmd(0x2B);
    lcd_write_u8((uint8_t)(y0 >> 8));
    lcd_write_u8((uint8_t)(y0 & 0xFFU));
    lcd_write_u8((uint8_t)(y1 >> 8));
    lcd_write_u8((uint8_t)(y1 & 0xFFU));

    /* Memory Write (0x2C) */
    lcd_write_cmd(0x2C);
}

void lcd_draw_pixel(uint16_t x, uint16_t y, lcd_color565_t color)
{
    uint16_t w = 1U;
    uint16_t h = 1U;
    if (!lcd_clip_rect_u16(&x, &y, &w, &h, (uint16_t)LCD_PANEL_WIDTH, (uint16_t)LCD_PANEL_HEIGHT))
    {
        return;
    }

    lcd_set_window(x, y, 1U, 1U);
    lcd_write_data(color);
}

void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, lcd_color565_t color)
{
    if (!lcd_clip_rect_u16(&x, &y, &w, &h, (uint16_t)LCD_PANEL_WIDTH, (uint16_t)LCD_PANEL_HEIGHT))
    {
        return;
    }

    lcd_set_window(x, y, w, h);

    const uint32_t pixels = (uint32_t)w * (uint32_t)h;
    for (uint32_t i = 0; i < pixels; i++)
    {
        lcd_write_data(color);
    }
}

void lcd_clear(lcd_color565_t color)
{
    lcd_fill_rect(0U, 0U, (uint16_t)LCD_PANEL_WIDTH, (uint16_t)LCD_PANEL_HEIGHT, color);
}

void lcd_panel_init(void)
{
    /* 先做底层初始化 + 硬复位 */
    lcd_init();
    delay_ms(50U);

    /* 初始化序列来源：HX8357D 常用配置（早期参考工程中的 HX8357D 分支）。 */
    lcd_write_cmd(0xE9);
    lcd_write_u8(0x20);

    lcd_write_cmd(0x11); /* Sleep Out */
    delay_ms(120U);

    lcd_write_cmd(0x3A);
    lcd_write_u8(0x55); /* 16-bit/pixel (RGB565) */

    lcd_write_cmd(0xD1);
    lcd_write_u8(0x00);
    lcd_write_u8(0x65);
    lcd_write_u8(0x1F);

    lcd_write_cmd(0xD0);
    lcd_write_u8(0x07);
    lcd_write_u8(0x07);
    lcd_write_u8(0x80);

    lcd_write_cmd(0x36); /* MADCTL */
    lcd_write_u8(0x4C);

    lcd_write_cmd(0xC1);
    lcd_write_u8(0x10);
    lcd_write_u8(0x10);
    lcd_write_u8(0x02);
    lcd_write_u8(0x02);

    lcd_write_cmd(0xC0);
    lcd_write_u8(0x00);
    lcd_write_u8(0x35);
    lcd_write_u8(0x00);
    lcd_write_u8(0x00);
    lcd_write_u8(0x01);
    lcd_write_u8(0x02);

    lcd_write_cmd(0xC4);
    lcd_write_u8(0x03);

    lcd_write_cmd(0xC5);
    lcd_write_u8(0x01);

    lcd_write_cmd(0xD2);
    lcd_write_u8(0x01);
    lcd_write_u8(0x22);

    lcd_write_cmd(0xE7);
    lcd_write_u8(0x38);

    lcd_write_cmd(0xF3);
    lcd_write_u8(0x08);
    lcd_write_u8(0x12);
    lcd_write_u8(0x12);
    lcd_write_u8(0x08);

    lcd_write_cmd(0xC8);
    lcd_write_u8(0x01);
    lcd_write_u8(0x52);
    lcd_write_u8(0x37);
    lcd_write_u8(0x10);
    lcd_write_u8(0x0D);
    lcd_write_u8(0x01);
    lcd_write_u8(0x04);
    lcd_write_u8(0x51);
    lcd_write_u8(0x77);
    lcd_write_u8(0x01);
    lcd_write_u8(0x01);
    lcd_write_u8(0x0D);
    lcd_write_u8(0x08);
    lcd_write_u8(0x80);
    lcd_write_u8(0x00);

    lcd_write_cmd(0x29); /* Display ON */
    delay_ms(20U);
}

