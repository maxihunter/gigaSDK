#ifndef WS2812_H
#define WS2812_H

#include "stm32f4xx_hal.h"

HAL_StatusTypeDef WS2812_Init(TIM_HandleTypeDef *timer,
                              DMA_HandleTypeDef *dma,
                              uint32_t channel);
HAL_StatusTypeDef WS2812_SetLed1Color(uint8_t red, uint8_t green, uint8_t blue);
HAL_StatusTypeDef WS2812_SetLed2Color(uint8_t red, uint8_t green, uint8_t blue);

#endif
