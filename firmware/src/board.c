#include "board.h"

#include "gd32f30x.h"

#define BOARD_LED_PORT GPIOC
#define BOARD_LED_RCU RCU_GPIOC
#define BOARD_LED_RED_PIN GPIO_PIN_0
#define BOARD_LED_GREEN_PIN GPIO_PIN_1
#define BOARD_LED_BLUE_PIN GPIO_PIN_2

#define BOARD_USART USART0
#define BOARD_USART_RCU RCU_USART0
#define BOARD_USART_GPIO_RCU RCU_GPIOA
#define BOARD_USART_GPIO_PORT GPIOA
#define BOARD_USART_TX_PIN GPIO_PIN_9
#define BOARD_USART_RX_PIN GPIO_PIN_10

static uint32_t board_led_pin(board_led_t led)
{
    switch (led) {
    case BOARD_LED_RED:
        return BOARD_LED_RED_PIN;
    case BOARD_LED_GREEN:
        return BOARD_LED_GREEN_PIN;
    case BOARD_LED_BLUE:
        return BOARD_LED_BLUE_PIN;
    default:
        return 0;
    }
}

void board_init(void)
{
    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(BOARD_LED_RCU);
    rcu_periph_clock_enable(BOARD_USART_GPIO_RCU);
    rcu_periph_clock_enable(BOARD_USART_RCU);

    /* Keep SWD active while releasing JTAG pins for GPIO use. */
    gpio_pin_remap_config(GPIO_SWJ_SWDPENABLE_REMAP, ENABLE);

    gpio_init(BOARD_LED_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ,
              BOARD_LED_RED_PIN | BOARD_LED_GREEN_PIN | BOARD_LED_BLUE_PIN);
    board_led_write(BOARD_LED_RED, false);
    board_led_write(BOARD_LED_GREEN, false);
    board_led_write(BOARD_LED_BLUE, false);

    gpio_init(BOARD_USART_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, BOARD_USART_TX_PIN);
    gpio_init(BOARD_USART_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, BOARD_USART_RX_PIN);

    usart_deinit(BOARD_USART);
    usart_baudrate_set(BOARD_USART, 115200U);
    usart_receive_config(BOARD_USART, USART_RECEIVE_ENABLE);
    usart_transmit_config(BOARD_USART, USART_TRANSMIT_ENABLE);
    usart_enable(BOARD_USART);
}

void board_led_write(board_led_t led, bool enabled)
{
    uint32_t pin = board_led_pin(led);

    if (pin == 0U) {
        return;
    }

    gpio_bit_write(BOARD_LED_PORT, pin, enabled ? SET : RESET);
}

void board_led_toggle(board_led_t led)
{
    uint32_t pin = board_led_pin(led);

    if (pin == 0U) {
        return;
    }

    gpio_bit_write(BOARD_LED_PORT, pin, (bit_status)(1U - gpio_input_bit_get(BOARD_LED_PORT, pin)));
}

void board_uart_write(const char *text)
{
    while (*text != '\0') {
        usart_data_transmit(BOARD_USART, (uint8_t)*text);
        while (usart_flag_get(BOARD_USART, USART_FLAG_TBE) == RESET) {
        }
        text++;
    }
}
