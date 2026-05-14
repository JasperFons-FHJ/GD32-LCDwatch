#include "systick.h"

void SysTick_Handler(void)
{
    systick_tick();
}

void HardFault_Handler(void)
{
    while (1) {
    }
}
