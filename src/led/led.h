#ifndef LED_LED_H
#define LED_LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
    LED0 = 0,
    LED1 = 1
} led_id_t;

void led_init(void);
void led_on(led_id_t led_id);
void led_off(led_id_t led_id);
void led_toggle(led_id_t led_id);

#ifdef __cplusplus
}
#endif

#endif /* LED_LED_H */

