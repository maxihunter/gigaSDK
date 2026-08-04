#include <stdint.h>

#include "stm32f407xx.h"

int main(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;
    GPIOA->MODER = (GPIOA->MODER & ~(3UL << (6U * 2U))) |
                   (1UL << (6U * 2U));

    for (;;) {
        GPIOA->ODR ^= (1UL << 6U);
        for (volatile uint32_t delay = 0U; delay < 500000U; ++delay) {
            __NOP();
        }
    }
}
