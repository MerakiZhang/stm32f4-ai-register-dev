#include "timer/basic_timer.h"

#include "stm32f4xx.h"

/* TIM6 为 16-bit 基本定时器，ARR 最大 0xFFFF。
 * 将计数频率配置为 1MHz（1us/tick），单次计数模式（OPM）下：
 * - 置位 CEN 后开始从 0 计数到 ARR
 * - 产生更新事件并置 UIF
 */

static uint32_t basic_timer_get_apb_prescaler(uint32_t ppre_bits)
{
    /* PPRE[2:0] 编码：
     * 0xx: /1
     * 100: /2
     * 101: /4
     * 110: /8
     * 111: /16
     */
    if (ppre_bits < 4U)
    {
        return 1U;
    }

    switch (ppre_bits)
    {
    case 4U: return 2U;
    case 5U: return 4U;
    case 6U: return 8U;
    default: return 16U;
    }
}

static uint32_t basic_timer_tim6_clk_hz(void)
{
    /* TIM6 位于 APB1：
     * - PCLK1 = HCLK / APB1_prescaler
     * - 若 APB1_prescaler != 1，则 TIMxCLK = 2 * PCLK1
     */
    uint32_t cfgr = RCC->CFGR;
    uint32_t ppre1_bits = (cfgr & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
    uint32_t apb1_div = basic_timer_get_apb_prescaler(ppre1_bits);

    /* SystemCoreClock 在 CMSIS 语义中代表 HCLK（AHB 时钟）。 */
    uint32_t hclk_hz = SystemCoreClock;
    uint32_t pclk1_hz = hclk_hz / apb1_div;

    if (apb1_div == 1U)
    {
        return pclk1_hz;
    }
    return 2U * pclk1_hz;
}

static void basic_timer_delay_us_chunk(uint16_t us)
{
    if (us == 0U)
    {
        return;
    }

    /* 停止并清标志 */
    CLEAR_BIT(TIM6->CR1, TIM_CR1_CEN);
    WRITE_REG(TIM6->SR, 0U);

    /* 单次计数：计数 us 个 tick（1 tick = 1us） */
    WRITE_REG(TIM6->CNT, 0U);
    WRITE_REG(TIM6->ARR, (uint32_t)us - 1U);

    /* 触发更新事件，将 PSC/ARR 立即装载 */
    WRITE_REG(TIM6->EGR, TIM_EGR_UG);

    /* UG 会触发一次更新事件并可能置位 UIF，需要清除后再开始计数 */
    WRITE_REG(TIM6->SR, 0U);

    /* 启动计数 */
    SET_BIT(TIM6->CR1, TIM_CR1_CEN);

    /* 阻塞等待更新完成 */
    while ((READ_BIT(TIM6->SR, TIM_SR_UIF)) == 0U)
    {
        /* busy wait */
    }

    /* 清除 UIF，避免下次误判 */
    WRITE_REG(TIM6->SR, 0U);
}

void basic_timer_init(void)
{
    /* 1) 使能 TIM6 时钟 */
    SET_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM6EN);
    (void)READ_BIT(RCC->APB1ENR, RCC_APB1ENR_TIM6EN);

    /* 2) 复位 TIM6，得到干净状态 */
    SET_BIT(RCC->APB1RSTR, RCC_APB1RSTR_TIM6RST);
    CLEAR_BIT(RCC->APB1RSTR, RCC_APB1RSTR_TIM6RST);

    /* 3) 配置为 1MHz 计数（1us/tick） */
    uint32_t tim6_clk = basic_timer_tim6_clk_hz();
    uint32_t psc_div = tim6_clk / 1000000U;
    if (psc_div == 0U)
    {
        psc_div = 1U;
    }

    /* PSC 寄存器写入的是 (div-1) */
    WRITE_REG(TIM6->PSC, psc_div - 1U);

    /* 4) 单次计数模式（OPM=1），向上计数（DIR=0，默认） */
    MODIFY_REG(TIM6->CR1,
               TIM_CR1_OPM,
               TIM_CR1_OPM);

    /* 5) 默认 ARR 给一个最大值；实际延时会按需改写 */
    WRITE_REG(TIM6->ARR, 0xFFFFU);
    WRITE_REG(TIM6->CNT, 0U);

    /* 6) 产生一次更新事件装载预分频 */
    WRITE_REG(TIM6->EGR, TIM_EGR_UG);
    WRITE_REG(TIM6->SR, 0U);
}

void basic_timer_delay_us(uint32_t us)
{
    /* TIM6 ARR 为 16-bit，分段处理 */
    while (us != 0U)
    {
        uint16_t chunk = (us > 0xFFFFU) ? 0xFFFFU : (uint16_t)us;
        basic_timer_delay_us_chunk(chunk);
        us -= (uint32_t)chunk;
    }
}

void basic_timer_delay_ms(uint32_t ms)
{
    /* 使用 64-bit 防止 ms*1000 溢出 */
    uint64_t total_us = (uint64_t)ms * 1000ULL;

    while (total_us != 0ULL)
    {
        uint16_t chunk = (total_us > 0xFFFFULL) ? 0xFFFFU : (uint16_t)total_us;
        basic_timer_delay_us_chunk(chunk);
        total_us -= (uint64_t)chunk;
    }
}

