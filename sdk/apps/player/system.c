#include <stdint.h>
#include "stm32f407xx.h"

uint32_t SystemCoreClock = 16000000U;
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                  1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};
void SystemInit(void)
{
    SCB->CPACR |= (3UL << 20U) | (3UL << 22U);
    SCB->VTOR = 0x08020000UL;
}
void SystemCoreClockUpdate(void) {}
void _init(void) {}
void _fini(void) {}
