#ifndef DELAY_DELAY_H
#define DELAY_DELAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void delay_init(void);
void delay_ms(uint32_t ms);
uint32_t delay_get_tick(void);

#ifdef __cplusplus
}
#endif

#endif /* DELAY_DELAY_H */

