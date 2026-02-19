#ifndef LCD_LCD_H
#define LCD_LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief LCD(8080-16bit) over FSMC(NE4) 底层驱动（仅初始化 + Cmd/Data 读写 + Reset/背光控制）。
 *
 * 地址映射与引脚见: src/lcd/lcd.md
 */

/* ============================= 面板参数（HX8357D） ============================= */
/**
 * @brief HX8357D 典型 3.5" TFT 分辨率（竖屏/Portrait）。
 *
 * 说明：若后续支持旋转（MADCTL），可在中上层维护当前 width/height。
 */
#define LCD_PANEL_WIDTH  (320U)
#define LCD_PANEL_HEIGHT (480U)

typedef uint16_t lcd_color565_t;

/* ============================= 底层（总线层） ============================= */

/** 初始化 FSMC + GPIO，并对 LCD 进行硬件复位，默认打开背光。 */
void lcd_init(void);

/** @brief LCD 硬件复位脚控制（PD3）。active-low */
void lcd_reset_assert(void);
void lcd_reset_deassert(void);
void lcd_reset_pulse(void);

/** @brief 背光控制（PB15）。active-high */
void lcd_backlight_on(void);
void lcd_backlight_off(void);
void lcd_backlight_set(bool on);

/** @brief FSMC Memory-mapped 写命令（RS=0）。 */
void lcd_write_cmd(uint16_t cmd);

/** @brief FSMC Memory-mapped 写数据（RS=1）。 */
void lcd_write_data(uint16_t data);

/** @brief 读数据（从 LCD_DATA 地址读取一次 16-bit）。
 *  
 * 说明：不同 LCD 控制器对读取可能要求 dummy read。
 * 如需 dummy read，请使用 lcd_read_data_dummy().
 */
uint16_t lcd_read_data(void);

/** @brief 读数据（先 dummy read 一次，再返回第二次读到的数据）。 */
uint16_t lcd_read_data_dummy(void);

/* ============================= 中上层（面板层：HX8357D） ============================= */

/**
 * @brief 初始化 LCD 面板（包含底层 lcd_init()），并发送 HX8357D 初始化序列。
 *
 * 说明：本函数完成：
 * - GPIO+FSMC 初始化
 * - 硬件复位
 * - HX8357D 寄存器初始化（Sleep Out / Pixel Format / MADCTL / Gamma ...）
 */
void lcd_panel_init(void);

/**
 * @brief 设置写入窗口并进入 GRAM 写状态（会发送 0x2C）。
 *
 * 坐标系：左上角(0,0)，x 向右，y 向下。
 * 参数：x,y 为左上角；w,h 为宽高。
 * 默认裁剪到 [0..LCD_PANEL_WIDTH)×[0..LCD_PANEL_HEIGHT)。
 */
void lcd_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/** 清屏（填充全屏为指定 RGB565 颜色）。 */
void lcd_clear(lcd_color565_t color);

/** 填充矩形（RGB565），默认裁剪。 */
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, lcd_color565_t color);

/** 画一个像素点（RGB565），默认裁剪。 */
void lcd_draw_pixel(uint16_t x, uint16_t y, lcd_color565_t color);

#ifdef __cplusplus
}
#endif

#endif /* LCD_LCD_H */

