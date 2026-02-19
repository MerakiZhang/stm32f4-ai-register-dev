# STM32F407 + FSMC(NE4) 驱动 HX8357D(3.5" TFT, 8080-16bit)  
> 目的：整理 **LCD ↔ MCU 引脚映射**，并明确 **GPIO 应如何配置**（AF / 输出 / 输入）

---

## 1. 总线与地址映射结论（必须先确认）

- LCD 接口：**8080 并口，16-bit 数据总线**
- FSMC 片选：**NE4**（STM32F407 FSMC Bank1 Region4）
- RS(D/C) 选择：接 **FSMC_A6**

### 1.1 命令/数据地址映射（Memory-mapped）
- Bank1 NE4 基地址：`0x6C000000`
- RS 接 A6，且 16-bit 模式下 A6 对应 MCU 地址 bit7  
  → 命令/数据地址偏移：`0x80`

因此：
- **LCD_CMD 地址**：`0x6C000000`（RS=0，写命令）
- **LCD_DATA 地址**：`0x6C000080`（RS=1，写数据/GRAM）

---

## 2. LCD ↔ STM32F407 引脚映射表

### 2.1 FSMC 控制/地址线
| LCD 信号 | FSMC 信号 | STM32F407 引脚 |
|---|---|---|
| LCD_CS | FSMC_NE4 | PG12 |
| LCD_WR | FSMC_NWE | PD5 |
| LCD_RD | FSMC_NOE | PD4 |
| LCD_RS / D-C | FSMC_A6 | PF12 |

### 2.2 FSMC 数据线（16-bit）
| 数据线 | STM32 引脚 | 数据线 | STM32 引脚 |
|---|---|---|---|
| D0  | PD14 | D8  | PE11 |
| D1  | PD15 | D9  | PE12 |
| D2  | PD0  | D10 | PE13 |
| D3  | PD1  | D11 | PE14 |
| D4  | PE7  | D12 | PE15 |
| D5  | PE8  | D13 | PD8  |
| D6  | PE9  | D14 | PD9  |
| D7  | PE10 | D15 | PD10 |

### 2.3 LCD 额外控制
| 信号 | 功能 | STM32 引脚 |
|---|---|---|
| LCD_RESET | 硬件复位 | PD3 |
| LCD_BL | 背光开关 | PB15 |

---

## 3. 触摸（电阻触摸 SPI）引脚映射（可选）
> 触摸与 LCD 显示分开：LCD 走 FSMC，并不影响触摸 SPI。

| 触摸信号 | STM32 引脚 | 建议用途 |
|---|---|---|
| T_SCK  | PB0  | SPI SCK 或模拟 SCK |
| T_MISO | PB2  | SPI MISO |
| T_MOSI | PF11 | SPI MOSI |
| T_CS   | PC13 | SPI CS（GPIO 输出） |
| T_PEN  | PB1  | 触摸中断输入（EXTI） |

---

## 4. GPIO 配置要求（按功能分组）

## 4.1 FSMC 引脚（必须配置为 Alternate Function）
> 下列引脚全部属于 FSMC 信号线：需要配置为 **AF 模式**，复用到 **FSMC(AF12)**

### 4.1.1 需要配置为 AF12(FSMC) 的引脚清单
- **GPIOD**：PD0, PD1, PD4, PD5, PD8, PD9, PD10, PD14, PD15
- **GPIOE**：PE7, PE8, PE9, PE10, PE11, PE12, PE13, PE14, PE15
- **GPIOF**：PF12
- **GPIOG**：PG12

### 4.1.2 推荐 GPIO 参数（FSMC 信号线通用）
- 模式（MODER）：**Alternate Function (AF)**
- 复用（AFR）：**AF12**
- 输出类型（OTYPER）：**Push-Pull**
- 速度（OSPEEDR）：**Very High**
- 上下拉（PUPDR）：**NoPull**（若你的板子有外部上拉/下拉需求，再按硬件修改）

---

## 4.2 LCD_RESET（普通 GPIO 输出）
- 引脚：**PD3**
- 模式：**Output**
- 输出类型：Push-Pull
- 速度：Low/Medium 均可
- 上下拉：NoPull（一般不需要）
- 典型默认电平：上电后先拉高；需要复位时拉低一段时间再拉高

---

## 4.3 LCD_BL 背光（普通 GPIO 输出）
- 引脚：**PB15**
- 模式：**Output**
- 输出类型：Push-Pull
- 速度：Low/Medium 均可
- 上下拉：NoPull
- 备注：如果后续要调亮度，可改接定时器 PWM；当前先用 GPIO 开关即可

---

## 4.4 触摸 SPI（可选配置）
> 如果你用硬件 SPI：SCK/MOSI/MISO 配 AF；CS 用 GPIO 输出；PEN 用输入/EXTI。

- **PB0 (T_SCK)**：SPI SCK（AF，具体 AF 取决于你选的 SPI 外设）
- **PB2 (T_MISO)**：SPI MISO（AF）
- **PF11 (T_MOSI)**：SPI MOSI（AF）
- **PC13 (T_CS)**：GPIO Output（片选）
- **PB1 (T_PEN)**：GPIO Input（可配置 EXTI，中断触发方式通常为下降沿）

---

## 5. 端口时钟（初始化前必须打开）
> 下列端口只要你用到了就要开 RCC 时钟：

- FSMC 数据/控制：**GPIOD、GPIOE、GPIOF、GPIOG**
- Reset/BL：**GPIOD、GPIOB**
- 触摸（可选）：**GPIOB、GPIOC、GPIOF**

---

## 6. 快速自检清单（避免最常见错误）
- [ ] NE4 是否确实接到 PG12，并在 FSMC 配置中选 Bank1_4
- [ ] PF12 是否设置为 FSMC_A6（AF12），否则命令/数据会混
- [ ] 数据线 D0~D15 是否全部为 AF12（少一根就会花屏/颜色不对）
- [ ] PD3 复位脚是否为普通输出且能拉低/拉高
- [ ] PB15 背光是否打开（否则屏可能“工作但不亮”）
- [ ] FSMC 写时序先保守（DataSetup 给大一点），稳定后再加速

---

## 7. 本工程底层驱动实现说明（CMSIS）

本仓库已提供 **底层 LCD 驱动**（仅初始化 + Cmd/Data 读写 + Reset/背光控制），文件：

- 头文件：[`src/lcd/lcd.h`](src/lcd/lcd.h:1)
- 源文件：[`src/lcd/lcd.c`](src/lcd/lcd.c:1)

### 7.1 API（最小集合）

- 初始化：[`lcd_init()`](src/lcd/lcd.h:18)
- 复位控制：[`lcd_reset_assert()`](src/lcd/lcd.h:21)、[`lcd_reset_deassert()`](src/lcd/lcd.h:22)、[`lcd_reset_pulse()`](src/lcd/lcd.h:23)
- 背光控制：[`lcd_backlight_on()`](src/lcd/lcd.h:26)、[`lcd_backlight_off()`](src/lcd/lcd.h:27)、[`lcd_backlight_set()`](src/lcd/lcd.h:28)
- 总线读写：[`lcd_write_cmd()`](src/lcd/lcd.h:31)、[`lcd_write_data()`](src/lcd/lcd.h:34)、[`lcd_read_data()`](src/lcd/lcd.h:41)、[`lcd_read_data_dummy()`](src/lcd/lcd.h:44)

### 7.5 中上层（面板层：HX8357D）API 约定

本工程的中上层绘图 API 直接追加在同一份头文件中：[`src/lcd/lcd.h`](src/lcd/lcd.h:1)

- 初始化面板：[`lcd_panel_init()`](src/lcd/lcd.h:1)
- 设置窗口：[`lcd_set_window(x,y,w,h)`](src/lcd/lcd.h:1)
- 清屏/填充/画点：[`lcd_clear()`](src/lcd/lcd.h:1)、[`lcd_fill_rect()`](src/lcd/lcd.h:1)、[`lcd_draw_pixel()`](src/lcd/lcd.h:1)

参数约定：
- 坐标系：左上角(0,0)，x 向右，y 向下
- 矩形使用 **起点+宽高**：`(x,y,w,h)`
- 默认 **自动裁剪** 到屏幕范围（越界调用安全无副作用）

### 7.2 FSMC 地址映射（NE4 + A6）

驱动内部使用 Memory-mapped 方式访问：

- CMD：`0x6C000000`
- DATA：`0x6C000080`

与本文 1.1 的结论一致。

### 7.3 FSMC 时序参数调整

[`src/lcd/lcd.c`](src/lcd/lcd.c:1) 中提供了可覆盖的宏，用于调整读/写时序：

- `LCD_FSMC_READ_ADDSET / ADDHLD / DATAST / BUSTURN / ACCMOD`
- `LCD_FSMC_WRITE_ADDSET / ADDHLD / DATAST / BUSTURN / ACCMOD`

默认采用保守值以保证先点亮/稳定，再逐步加速。

### 7.4 构建注意事项

历史参考代码目录 `src/lcd/ref/` 已从仓库移除；本工程仅保留 CMSIS(寄存器) 实现的 LCD 驱动。

---
