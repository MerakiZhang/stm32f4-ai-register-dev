#ifndef CLOCK_CLOCK_H
#define CLOCK_CLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
    CLOCK_OK = 0,
    CLOCK_ERR_HSE_TIMEOUT,
    CLOCK_ERR_PLL_TIMEOUT,
    CLOCK_ERR_SYSCLK_SWITCH_TIMEOUT
} clock_status_t;

clock_status_t clock_init_168mhz_hse8(void);
uint32_t clock_get_hclk_hz(void);

#ifdef __cplusplus
}
#endif

#endif /* CLOCK_CLOCK_H */

