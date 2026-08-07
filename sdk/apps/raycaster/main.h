#ifndef RAYCASTER_MAIN_H
#define RAYCASTER_MAIN_H

#include "stm32f4xx_hal.h"

extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_tx;

void Raycaster_PlatformInit(void);
void Error_Handler(void);

#endif
