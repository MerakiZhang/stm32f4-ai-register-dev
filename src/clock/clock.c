#include "clock.h"

#include "stm32f4xx.h"

#define CLOCK_WAIT_TIMEOUT  (0x4FFFFFU)

#define CLOCK_PLL_M         (8U)
#define CLOCK_PLL_N         (336U)
#define CLOCK_PLL_P_BITS    (0U) /* PLLP = 2 => 编码 00 */
#define CLOCK_PLL_Q         (7U)

static int clock_wait_flag_set(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t timeout = CLOCK_WAIT_TIMEOUT;

    while (((*reg) & mask) == 0U)
    {
        if (timeout-- == 0U)
        {
            return 0;
        }
    }

    return 1;
}

clock_status_t clock_init_168mhz_hse8(void)
{
    /* 1) 使能 HSE */
    SET_BIT(RCC->CR, RCC_CR_HSEON);
    if (!clock_wait_flag_set(&RCC->CR, RCC_CR_HSERDY))
    {
        return CLOCK_ERR_HSE_TIMEOUT;
    }

    /* 2) 使能 PWR 时钟并设置电压缩放（Scale 1） */
    SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);
    SET_BIT(PWR->CR, PWR_CR_VOS);

    /* 3) 配置 FLASH 等待周期与缓存（168MHz 需要 5WS） */
    MODIFY_REG(FLASH->ACR,
               FLASH_ACR_LATENCY,
               FLASH_ACR_LATENCY_5WS);
    SET_BIT(FLASH->ACR, FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN);

    /* 4) 先配置总线分频：AHB=1, APB1=4, APB2=2 */
    MODIFY_REG(RCC->CFGR,
               RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2,
               RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2);

    /* 5) 若 PLL 正在运行，先关闭 */
    if ((READ_BIT(RCC->CR, RCC_CR_PLLON)) != 0U)
    {
        CLEAR_BIT(RCC->CR, RCC_CR_PLLON);
        while ((READ_BIT(RCC->CR, RCC_CR_PLLRDY)) != 0U)
        {
            /* wait PLL unlock */
        }
    }

    /* 6) PLL 配置：HSE=8MHz, M=8, N=336, P=2, Q=7 -> SYSCLK=168MHz */
    WRITE_REG(RCC->PLLCFGR,
              (CLOCK_PLL_M << RCC_PLLCFGR_PLLM_Pos)
              | (CLOCK_PLL_N << RCC_PLLCFGR_PLLN_Pos)
              | (CLOCK_PLL_P_BITS << RCC_PLLCFGR_PLLP_Pos)
              | RCC_PLLCFGR_PLLSRC_HSE
              | (CLOCK_PLL_Q << RCC_PLLCFGR_PLLQ_Pos));

    /* 7) 使能 PLL 并等待锁定 */
    SET_BIT(RCC->CR, RCC_CR_PLLON);
    if (!clock_wait_flag_set(&RCC->CR, RCC_CR_PLLRDY))
    {
        return CLOCK_ERR_PLL_TIMEOUT;
    }

    /* 8) 切换系统时钟到 PLL */
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL);
    if (!clock_wait_flag_set(&RCC->CFGR, RCC_CFGR_SWS_PLL))
    {
        return CLOCK_ERR_SYSCLK_SWITCH_TIMEOUT;
    }

    /* 9) 刷新系统频率变量，并固定为本模块目标值（避免 HSE_VALUE 未覆写导致误差） */
    SystemCoreClockUpdate();
    SystemCoreClock = 168000000U;

    return CLOCK_OK;
}

uint32_t clock_get_hclk_hz(void)
{
    return SystemCoreClock;
}

