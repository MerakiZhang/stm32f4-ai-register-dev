# LED 驱动接口说明

## 一、GPIO 配置要求

- 引脚模式：GPIO 输出模式  
- 输出类型：推挽输出（Push-Pull）  
- 上下拉：无上下拉  
- 输出速度：中等  

### 电平逻辑

本电路为 **低电平点亮**：

- GPIO = 0 → LED 亮  
- GPIO = 1 → LED 灭  

---

## 二、LED 引脚映射（示例）

- LED0 → PF9  
- LED1 → PF10  

---

## 三、需要实现的函数接口

### 1. 点亮 LED
led_on(led_id)
行为：
- 将对应 GPIO 输出置为 **低电平**

---

### 2. 熄灭 LED
led_off(led_id)
行为：
- 将对应 GPIO 输出置为 **高电平**

---

### 3. 翻转 LED 状态
led_toggle(led_id)
行为：
- 若当前 GPIO 为高电平 → 置低（LED 亮）
- 若当前 GPIO 为低电平 → 置高（LED 灭）

---
