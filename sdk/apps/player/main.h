#ifndef PLAYER_MAIN_H
#define PLAYER_MAIN_H

#include "stm32f4xx_hal.h"

extern SPI_HandleTypeDef hspi1;
extern I2S_HandleTypeDef hi2s3;
extern SD_HandleTypeDef hsd;
extern DMA_HandleTypeDef hdma_spi1_tx;
extern DMA_HandleTypeDef hdma_spi3_tx;
extern DMA_HandleTypeDef hdma_tim1_ch2;

void Player_PlatformInit(void);
void Error_Handler(void);

#endif
