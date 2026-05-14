#include "systick.h"

#include "gd32f30x.h"

static volatile uint32_t g_millis;

void systick_config(void)
{
    if (SysTick_Config(SystemCoreClock / 1000U) != 0U) {
        while (1) {
        }
    }

    NVIC_SetPriority(SysTick_IRQn, 0x05U);
}

void systick_tick(void)
{
    g_millis++;
}

uint32_t systick_millis(void)
{
    return g_millis;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = systick_millis();

    while ((uint32_t)(systick_millis() - start) < ms) {
    }
}

void delay(uint32_t count)
{
    delay_ms(count);
}
