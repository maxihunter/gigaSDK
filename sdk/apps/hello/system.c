#include <stdint.h>

#include "stm32f407xx.h"

uint32_t SystemCoreClock = 16000000U;

void SystemInit(void)
{
    SCB->CPACR |= (3UL << 20U) | (3UL << 22U);
    SCB->VTOR = 0x08020000UL;
}

void SystemCoreClockUpdate(void)
{
}

void _init(void)
{
}

void _fini(void)
{
}
