# Clock 模块说明

## 1. 模块目标

[`clock` 模块](src/clock/clock.c) 用于将 STM32F407 系统时钟配置为：

- 外部高速晶振 HSE = 8MHz
- `SYSCLK` = 168MHz
- `HCLK` = 168MHz
- `APB1` = 42MHz
- `APB2` = 84MHz

时钟切换完成后，需要由上层再初始化 [`delay` 模块](src/delay/delay.c)，用于生成 1ms SysTick 时基。

---

## 2. 文件结构

- 接口声明：[`src/clock/clock.h`](src/clock/clock.h)
- 具体实现：[`src/clock/clock.c`](src/clock/clock.c)

---

## 3. 对外接口

### 3.1 状态码

定义见 [`clock_status_t`](src/clock/clock.h:10)

- `CLOCK_OK`：配置成功
- `CLOCK_ERR_HSE_TIMEOUT`：等待 HSE 就绪超时
- `CLOCK_ERR_PLL_TIMEOUT`：等待 PLL 锁定超时
- `CLOCK_ERR_SYSCLK_SWITCH_TIMEOUT`：切换系统时钟到 PLL 超时

### 3.2 初始化接口

函数：[`clock_init_168mhz_hse8()`](src/clock/clock.h:18)

行为：

1. 打开 HSE 并等待稳定
2. 使能 PWR 外设并设置电压等级
3. 配置 FLASH 等待周期与缓存
4. 设置总线分频
5. 配置并启动 PLL
6. 切换系统时钟到 PLL
7. 刷新系统时钟变量

说明：

- 本函数不配置 SysTick。
- 若项目需要 [`delay_ms()`](src/delay/delay.c:18)，请在本函数返回 `CLOCK_OK` 后，由上层调用 [`delay_init()`](src/delay/delay.c:7) 生成 1ms 时基。

返回：[`clock_status_t`](src/clock/clock.h:10)

### 3.3 读取当前 HCLK

函数：[`clock_get_hclk_hz()`](src/clock/clock.h:19)

返回：当前 [`SystemCoreClock`](src/clock/clock.c:96) 值，单位 Hz。

---

## 4. 关键参数

在 [`src/clock/clock.c`](src/clock/clock.c) 中采用如下 PLL 参数：

- `PLLM = 8`
- `PLLN = 336`
- `PLLP = 2`
- `PLLQ = 7`

计算关系：

- PLL 输入：`8MHz / 8 = 1MHz`
- VCO：`1MHz * 336 = 336MHz`
- SYSCLK：`336MHz / 2 = 168MHz`

总线分频：

- AHB：`/1` -> `HCLK = 168MHz`
- APB1：`/4` -> `42MHz`
- APB2：`/2` -> `84MHz`

---

## 5. 实现细节与依赖

### 5.1 FLASH 与电源配置

- FLASH 延时配置为 5WS：[`FLASH_ACR_LATENCY_5WS`](src/clock/clock.c:44)
- 同时开启 I-Cache / D-Cache / Prefetch：[`FLASH->ACR`](src/clock/clock.c:45)
- 使能 PWR 并设置 [`PWR_CR_VOS`](src/clock/clock.c:39)

### 5.2 超时保护

实现中使用超时计数，避免硬件异常时死循环：

- HSE 等待：[`RCC_CR_HSERDY`](src/clock/clock.c:32)
- PLL 锁定等待：[`RCC_CR_PLLRDY`](src/clock/clock.c:72)
- SYSCLK 切换等待：[`RCC_CFGR_SWS_PLL`](src/clock/clock.c:79)

### 5.3 与 delay 模块的初始化顺序

本模块 **不依赖** `delay`，也不会在内部调用 [`delay_init()`](src/delay/delay.c:7)。

原因：`clock` 属于核心时钟树配置，应当最先执行；而 [`delay_init()`](src/delay/delay.c:7) 需要依赖最终的 [`SystemCoreClock`](drivers/stm32f4xx/source/system_stm32f4xx.c:137) 计算 1ms 的 SysTick 重装值。

因此正确顺序是：

1. 先调用 [`clock_init_168mhz_hse8()`](src/clock/clock.h:18)
2. 若返回 `CLOCK_OK`，再调用 [`delay_init()`](src/delay/delay.c:7)
3. 之后才能使用 [`delay_ms()`](src/delay/delay.c:18) 以及依赖延时的模块（例如 [`key_scan()`](src/key/key.c:49) 的消抖）

---

## 6. 推荐调用顺序

在系统启动流程中，建议按以下顺序调用：

1. [`clock_init_168mhz_hse8()`](src/clock/clock.h:18)
2. [`delay_init()`](src/delay/delay.c:7)
3. 外设初始化（LED、KEY、BEEP 等）
4. 业务逻辑循环

说明：

- 若 [`clock_init_168mhz_hse8()`](src/clock/clock.h:18) 返回非 `CLOCK_OK`，应进入错误处理分支，避免继续运行在未知时钟状态。

---

## 7. 说明与边界

- 当前模块聚焦系统主时钟路径，不包含 RTC/LSE 业务。
- `LSE 32.768kHz` 可在后续 RTC 模块中独立启用，不影响本模块主频配置。
- 若后续修改主频档位，需同步检查：
  - FLASH 等待周期
  - APB1/APB2 上限
  - SysTick 1ms 配置是否重建

