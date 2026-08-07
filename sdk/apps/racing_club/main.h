#ifndef RACING_CLUB_MAIN_H
#define RACING_CLUB_MAIN_H
#include "stm32f4xx_hal.h"
extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_tx;
void RacingClub_PlatformInit(void);
void Error_Handler(void);
#endif
