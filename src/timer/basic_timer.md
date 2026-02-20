# basic_timer（TIM6 基本定时器延时）使用说明

## 1. 模块目标

提供基于 TIM6 的 **us / ms 级阻塞延时**接口：

- [`basic_timer_init()`](src/timer/basic_timer.c:89)
- [`basic_timer_delay_us()`](src/timer/basic_timer.c:124)
- [`basic_timer_delay_ms()`](src/timer/basic_timer.c:135)

模块通过把 TIM6 配置为 1MHz 计数（1 tick = 1us），并使用 OPM 单次计数模式实现稳定延时。

## 2. 文件与接口

- 头文件：[`src/timer/basic_timer.h`](src/timer/basic_timer.h:1)
- 源文件：[`src/timer/basic_timer.c`](src/timer/basic_timer.c:1)

包含方式：

```c
#include "timer/basic_timer.h"
```

## 3. 初始化与调用顺序

### 3.1 初始化要求

必须在系统时钟配置完成后调用 [`basic_timer_init()`](src/timer/basic_timer.c:89)。

原因：本模块依赖 `SystemCoreClock` 来计算 TIM6 的输入时钟并设置预分频（PSC）。

在本工程中，推荐顺序示例：

1. 时钟树配置：[`clock_init_168mhz_hse8()`](src/clock/clock.c:27)
2. SysTick 1ms（可选，与本模块互不替代）：[`delay_init()`](src/delay/delay.c:7)
3. TIM6 基本定时器：[`basic_timer_init()`](src/timer/basic_timer.c:89)

工程中已在 [`main()`](src/main.c:10) 的初始化阶段插入了 [`basic_timer_init()`](src/main.c:26) 的调用用于编译验证。

### 3.2 典型用法

```c
#include "timer/basic_timer.h"

int main(void)
{
    /* 先完成系统时钟配置... */
    basic_timer_init();

    while (1)
    {
        basic_timer_delay_us(10);
        basic_timer_delay_ms(1);
    }
}
```

## 4. 实现原理（简述）

### 4.1 1MHz 计数

[`basic_timer_init()`](src/timer/basic_timer.c:89) 内部：

1. 使能 TIM6 外设时钟（APB1）
2. 计算 TIM6 输入时钟（APB1 定时器时钟规则）
3. 设置 `PSC`，使 TIM6 计数频率为 1MHz
4. 使能 OPM（One-Pulse Mode）单次计数

### 4.2 单次计数阻塞等待

单次延时由 [`basic_timer_delay_us_chunk()`](src/timer/basic_timer.c:55) 完成：

1. 停止计数并清状态位（UIF）
2. `CNT = 0`，设置 `ARR = us - 1`
3. 写 `EGR.UG` 触发更新，将 PSC/ARR 装载
4. 清 UIF（UG 可能导致 UIF 置位）
5. 置位 `CR1.CEN` 开始计数
6. 轮询 `SR.UIF` 直到更新事件发生

由于 TIM6 是 16-bit 计数器，`ARR` 最大为 `0xFFFF`，因此：

- [`basic_timer_delay_us()`](src/timer/basic_timer.c:124) 会把超过 `0xFFFF` 的延时分段执行
- [`basic_timer_delay_ms()`](src/timer/basic_timer.c:135) 会将 `ms` 换算为 `us`（使用 64-bit 防溢出）并分段执行

## 5. 注意事项与限制

1. **阻塞式延时**：调用期间 CPU 忙等，不适合长时间延时或功耗敏感场景。
2. **TIM6 资源占用**：该模块独占 TIM6。
   - 若工程后续需要使用 TIM6 触发 DAC 或其它用途，需要改用其他定时器或调整方案。
3. **依赖时钟稳定**：若运行中动态修改 APB1 分频或 `SystemCoreClock` 未同步更新，会导致延时不准；修改时钟后需重新调用 [`basic_timer_init()`](src/timer/basic_timer.c:89)。

