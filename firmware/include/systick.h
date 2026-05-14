#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

void systick_config(void);
void systick_tick(void);
uint32_t systick_millis(void);
void delay_ms(uint32_t ms);
void delay(uint32_t count);

#endif /* SYSTICK_H */
