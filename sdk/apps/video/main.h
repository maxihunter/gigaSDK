#ifndef VIDEO_APP_MAIN_H
#define VIDEO_APP_MAIN_H
#include "stm32f4xx_hal.h"
extern SPI_HandleTypeDef hspi1;
extern SD_HandleTypeDef hsd;
extern DMA_HandleTypeDef hdma_spi1_tx;
extern DMA_HandleTypeDef hdma_spi3_tx;
extern DMA_HandleTypeDef hdma_tim1_ch2;
void VideoApp_PlatformInit(void);
void Error_Handler(void);
#endif
