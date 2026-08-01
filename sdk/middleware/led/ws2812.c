#include "led/ws2812.h"

#define WS2812_LED_COUNT       2U
#define WS2812_BITS_PER_LED   24U
#define WS2812_RESET_SLOTS    48U
#define WS2812_PWM_ZERO       60U
#define WS2812_PWM_ONE       120U

static TIM_HandleTypeDef *ws2812_timer;
static DMA_HandleTypeDef *ws2812_dma;
static uint32_t ws2812_channel;
static uint8_t ws2812_colors[WS2812_LED_COUNT][3];
static uint16_t ws2812_pwm_data[
    WS2812_LED_COUNT * WS2812_BITS_PER_LED + WS2812_RESET_SLOTS
];

static HAL_StatusTypeDef WS2812_Transmit(void)
{
  uint32_t buffer_index = 0;

  if ((ws2812_timer == NULL) || (ws2812_dma == NULL))
  {
    return HAL_ERROR;
  }

  while (HAL_DMA_GetState(ws2812_dma) == HAL_DMA_STATE_BUSY)
  {
  }

  for (uint32_t led = 0; led < WS2812_LED_COUNT; ++led)
  {
    uint32_t color = ((uint32_t)ws2812_colors[led][1] << 16)
                   | ((uint32_t)ws2812_colors[led][0] << 8)
                   |  (uint32_t)ws2812_colors[led][2];

    for (uint32_t bit = 0; bit < WS2812_BITS_PER_LED; ++bit)
    {
      ws2812_pwm_data[buffer_index++] =
          (color & (1UL << (23U - bit))) ? WS2812_PWM_ONE : WS2812_PWM_ZERO;
    }
  }

  while (buffer_index < (sizeof(ws2812_pwm_data) / sizeof(ws2812_pwm_data[0])))
  {
    ws2812_pwm_data[buffer_index++] = 0;
  }

  return HAL_TIM_PWM_Start_DMA(
      ws2812_timer,
      ws2812_channel,
      (uint32_t *)ws2812_pwm_data,
      (uint16_t)(sizeof(ws2812_pwm_data) / sizeof(ws2812_pwm_data[0])));
}

HAL_StatusTypeDef WS2812_Init(TIM_HandleTypeDef *timer,
                              DMA_HandleTypeDef *dma,
                              uint32_t channel)
{
  if ((timer == NULL) || (dma == NULL))
  {
    return HAL_ERROR;
  }

  ws2812_timer = timer;
  ws2812_dma = dma;
  ws2812_channel = channel;
  return HAL_OK;
}

HAL_StatusTypeDef WS2812_SetLed1Color(uint8_t red, uint8_t green, uint8_t blue)
{
  ws2812_colors[0][0] = red;
  ws2812_colors[0][1] = green;
  ws2812_colors[0][2] = blue;
  return WS2812_Transmit();
}

HAL_StatusTypeDef WS2812_SetLed2Color(uint8_t red, uint8_t green, uint8_t blue)
{
  ws2812_colors[1][0] = red;
  ws2812_colors[1][1] = green;
  ws2812_colors[1][2] = blue;
  return WS2812_Transmit();
}
