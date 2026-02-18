#ifndef BEEP_BEEP_H
#define BEEP_BEEP_H

#ifdef __cplusplus
extern "C" {
#endif

void beep_init(void);
void beep_on(void);
void beep_off(void);
void beep_toggle(void);

#ifdef __cplusplus
}
#endif

#endif /* BEEP_BEEP_H */

