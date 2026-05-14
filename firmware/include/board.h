#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BOARD_LED_RED = 0,
    BOARD_LED_GREEN,
    BOARD_LED_BLUE,
} board_led_t;

void board_init(void);
void board_led_write(board_led_t led, bool enabled);
void board_led_toggle(board_led_t led);
void board_uart_write(const char *text);

#endif /* BOARD_H */
