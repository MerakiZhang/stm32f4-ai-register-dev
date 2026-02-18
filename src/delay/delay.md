# delay_ms 函数实现说明

## 1. 目标

实现一个毫秒级阻塞延时函数：
delay_ms(ms)

用于提供简单稳定的时间延迟，例如：

- 按键消抖
- LED 闪烁
- 简单时间控制

---

## 2. 实现原理

使用 MCU 的 **SysTick 定时器** 作为系统 1ms 时间基准。

实现思路：

1. 配置 SysTick 定时器，使其每 1ms 产生一次中断
2. 在 SysTick 中断服务函数中，维护一个全局毫秒计数器，例如：

system_tick_ms

3. `delay_ms(ms)` 调用时：

   - 记录当前时间：
     ```
     start = system_tick_ms
     ```
   - 循环等待直到：
     ```
     (system_tick_ms - start) >= ms
     ```
   - 条件满足后返回

---

## 3. SysTick 配置要求

- 时基：1ms
- 必须持续递增毫秒计数
- 中断函数只做一件事：
  - `system_tick_ms++`

不应在中断中执行复杂逻辑。

---

## 4. 函数行为

`delay_ms(ms)`：

- 输入：延时毫秒数 `ms`
- 行为：阻塞 CPU 直到时间达到
- 返回：延时结束后返回

特点：

- 延时精度由 SysTick 决定
- 不依赖 CPU 空循环
- 主频变化不会影响精度

---




