#ifndef KEY_KEY_H
#define KEY_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
    KEY_NONE = 0,
    KEY_WKUP,
    KEY0,
    KEY1,
    KEY2
} key_id_t;

void key_init(void);
key_id_t key_scan(void);

#ifdef __cplusplus
}
#endif

#endif /* KEY_KEY_H */

