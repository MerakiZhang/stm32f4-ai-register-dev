#ifndef TIMER_BASIC_TIMER_H
#define TIMER_BASIC_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief 初始化 TIM6 作为基本定时器延时源。
 *
 * 设计目标：将 TIM6 配置为 1MHz 计数（1 tick = 1us），并使用单次计数模式（OPM）
 * 实现阻塞式 us/ms 延时。
 *
 * @note 依赖 SystemCoreClock 已正确更新（例如在时钟初始化后调用）。
 */
void basic_timer_init(void);

/**
 * @brief 阻塞式 us 级延时（基于 TIM6）。
 * @param us 延时时长（微秒）。支持大于 65535us，内部会自动分段。
 */
void basic_timer_delay_us(uint32_t us);

/**
 * @brief 阻塞式 ms 级延时（基于 TIM6）。
 * @param ms 延时时长（毫秒）。
 */
void basic_timer_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_BASIC_TIMER_H */

