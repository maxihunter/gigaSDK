/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ili9341/ILI9341_STM32_Driver.h"
#include "ili9341/ILI9341_GFX.h"
#include "keyboard.h"
#include "menu.h"
#include "bootup.h"
#include "string.h"
#include <stdio.h>
#include "test.h"
#include "test16.h"
#include "minirle.h"
#include "rtc/rtc_clock.h"
#include "led/ws2812.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2S_HandleTypeDef hi2s3;

SD_HandleTypeDef hsd;

SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi2_tx;

TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
unsigned int sd_error = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2S3_Init(void);
/* USER CODE BEGIN PFP */
static void ILI9341_Draw_Splash(void);
static void ILI9341_FPS_Test(void);
static HAL_StatusTypeDef PCM5102A_TestBeep(void);
static void BIOS_LaunchApplication(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write(int file, char *ptr, int len)
{
    HAL_StatusTypeDef hstatus;

    if (file == 1 || file == 2) {
        hstatus = HAL_UART_Transmit(&huart1, (uint8_t*) ptr, len, HAL_MAX_DELAY);
        if (hstatus == HAL_OK)
            return len;
        else
            return -1;
    }
    return -1;
}
/*int __io_putchar(int ch)
{
    ITM_SendChar(ch);
    return (ch);
}*/
/*void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim) {

    if (htim->Instance == TIM1) {
        printf("Call DMA HALF FINISHED!\n\r");
        ws2812_update_buffer(&ws_leds, &ws_leds.dma_buffer[0]);
    }

}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {

    if (htim->Instance == TIM1) {
        printf("Call DMA ALL FINISHED!\n\r");
        ws2812_update_buffer(&ws_leds, &ws_leds.dma_buffer[BUFFER_SIZE]);
    }

}*/

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SDIO_SD_Init();
  MX_SPI2_Init();
  MX_FATFS_Init();
  MX_TIM1_Init();
  if (WS2812_Init(&htim1, &hdma_tim1_ch2, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  MX_USART1_UART_Init();
  MX_I2S3_Init();
  if (RTC_Clock_Init() != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN 2 */
  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
  //HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
  ILI9341_Init();
  ILI9341_Draw_Splash();
  
  WS2812_SetLed1Color(200, 200, 200);
  WS2812_SetLed2Color(200, 200, 200);

  FATFS fs;
  FRESULT res;
  res = f_mount(&fs, SDPath, 1);
  if (res != FR_OK) {
    ILI9341_Draw_Text("SD Card not found", 60, 220, RED, 2, BLACK);
    sd_error = 1;
	HAL_Delay(2000);
  }
  HAL_Delay(1000);
  uint16_t dec_data[3500] = {0};

  printf("===========================================================\n\r");
  RTC_ClockDateTime current_time;
  if (RTC_Clock_Get(&current_time) == HAL_OK)
  {
    printf("RTC: %04u-%02u-%02u %02u:%02u:%02u\n\r",
           current_time.year, current_time.month, current_time.day,
           current_time.hours, current_time.minutes, current_time.seconds);
  }
  minirle_decompress16(test_file_16, 568, dec_data );
  printf("MiniRLE16 test data decompressed\n\r");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  mainMenu_Init(BIOS_LaunchApplication);
  mainMenu_Handler();
  menuHeader_Handler(&current_time, 4);
  if (PCM5102A_TestBeep() != HAL_OK)
  {
    printf("PCM5102A test beep failed\n\r");
  }
  int port_state;
  uint32_t previous_keymap = 0U;
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
    uint32_t keymap = getKeyState();
    uint32_t pressed_keys = keymap & ~previous_keymap;
    previous_keymap = keymap;

    if (pressed_keys) {
        //printf("KEYDOWN=%lx\n\r", (unsigned long)pressed_keys);
        if (pressed_keys & KBRD_BTN_1) {
            mainMenu_TriggerSelect();
        } else
        if ((pressed_keys & KBRD_BTN_2) || (pressed_keys & KBRD_BTN_MENU)) {
            mainMenu_TriggerBack();
        } else
        if (pressed_keys & KBRD_BTN_UP) {
            mainMenu_TriggerUp();
        } else
        if (pressed_keys & KBRD_BTN_DOWN) {
            mainMenu_TriggerDown();
        } else
        if (pressed_keys & KBRD_BTN_LEFT) {
            mainMenu_TriggerLeft();
        } else
        if (pressed_keys & KBRD_BTN_RIGHT) {
            mainMenu_TriggerRight();
        }
        mainMenu_Handler();
    }
		port_state = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_6);
    HAL_Delay(50);
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2S3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S3_Init(void)
{

  /* USER CODE BEGIN I2S3_Init 0 */

  /* USER CODE END I2S3_Init 0 */

  /* USER CODE BEGIN I2S3_Init 1 */

  /* USER CODE END I2S3_Init 1 */
  hi2s3.Instance = SPI3;
  hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_22K;
  hi2s3.Init.CPOL = I2S_CPOL_LOW;
  hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S3_Init 2 */

  /* USER CODE END I2S3_Init 2 */

}

/**
  * @brief SDIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDIO_SD_Init(void)
{

  /* USER CODE BEGIN SDIO_Init 0 */

  /* USER CODE END SDIO_Init 0 */

  /* USER CODE BEGIN SDIO_Init 1 */

  /* USER CODE END SDIO_Init 1 */
  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd.Init.ClockDiv = 0;
  /* USER CODE BEGIN SDIO_Init 2 */

  /* USER CODE END SDIO_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 105;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);

  /*Configure GPIO pin : PE6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PC6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PD1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PB4 PB5 PB6 PB7
                           PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

static HAL_StatusTypeDef PCM5102A_TestBeep(void)
{
  enum {
    block_frames = 128
  };
  static const uint16_t tone_frequencies[] = {500, 1000, 2000};
  static const int16_t sine_table[32] = {
       0,  1171,  2296,  3333,  4243,  4989,  5543,  5885,
    6000,  5885,  5543,  4989,  4243,  3333,  2296,  1171,
       0, -1171, -2296, -3333, -4243, -4989, -5543, -5885,
   -6000, -5885, -5543, -4989, -4243, -3333, -2296, -1171
  };
  uint16_t audio_buffer[block_frames * 2];
  uint32_t i2s_clock = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_I2S);
  uint32_t prescaler = 2U * (SPI3->I2SPR & SPI_I2SPR_I2SDIV);

  if ((SPI3->I2SPR & SPI_I2SPR_ODD) != 0U) {
    prescaler++;
  }
  if ((i2s_clock == 0U) || (prescaler == 0U)) {
    return HAL_ERROR;
  }

  uint32_t sample_rate = i2s_clock / (256U * prescaler);
  uint32_t duration_frames = sample_rate / 4U;
  uint32_t gap_frames = sample_rate / 10U;
  uint32_t fade_frames = sample_rate / 200U;

  printf("I2S: clock=%lu Hz, sample_rate=%lu Hz, prescaler=%lu\n\r",
         (unsigned long)i2s_clock, (unsigned long)sample_rate,
         (unsigned long)prescaler);

  for (uint32_t tone = 0;
       tone < (sizeof(tone_frequencies) / sizeof(tone_frequencies[0]));
       tone++)
  {
    uint32_t phase = 0;
    uint32_t phase_step =
        (uint32_t)(((uint64_t)tone_frequencies[tone] << 32) / sample_rate);

    for (uint32_t frame = 0; frame < duration_frames; frame += block_frames)
    {
      uint32_t frames_in_block = duration_frames - frame;
      if (frames_in_block > block_frames) {
        frames_in_block = block_frames;
      }

      for (uint32_t i = 0; i < frames_in_block; i++)
      {
        uint32_t current_frame = frame + i;
        uint32_t gain = fade_frames;

        if (current_frame < fade_frames) {
          gain = current_frame;
        } else if ((duration_frames - current_frame) <= fade_frames) {
          gain = duration_frames - current_frame - 1;
        }

        int32_t sample = sine_table[phase >> 27] * 4;
        sample = (sample * (int32_t)gain) / fade_frames;
        phase += phase_step;

        audio_buffer[i * 2] = (uint16_t)(int16_t)sample;
        audio_buffer[(i * 2) + 1] = (uint16_t)(int16_t)sample;
      }

      HAL_StatusTypeDef status = HAL_I2S_Transmit(
          &hi2s3, audio_buffer, (uint16_t)(frames_in_block * 2), HAL_MAX_DELAY);
      if (status != HAL_OK) {
        return status;
      }
    }

    memset(audio_buffer, 0, sizeof(audio_buffer));
    for (uint32_t frame = 0; frame < gap_frames; frame += block_frames)
    {
      uint32_t frames_in_block = gap_frames - frame;
      if (frames_in_block > block_frames) {
        frames_in_block = block_frames;
      }

      HAL_StatusTypeDef status = HAL_I2S_Transmit(
          &hi2s3, audio_buffer, (uint16_t)(frames_in_block * 2), HAL_MAX_DELAY);
      if (status != HAL_OK) {
        return status;
      }
    }
  }

  return HAL_OK;
}

/*
 * Application menu callbacks own the device after launch and must not return.
 * Replace this placeholder loop with the application entry point.
 */
static void BIOS_LaunchApplication(void)
{
  ILI9341_Fill_Screen(BLACK);
  ILI9341_Draw_Text("Application started", 50, 110, WHITE, 2, BLACK);

  while (1)
  {
    /* Application main loop. */
  }
}

static void ILI9341_Draw_Splash(void) {
  ILI9341_Fill_Screen(WHITE);

  ILI9341_Draw_SmallImage(bootup_logo, 20, 70, 304, 114);
  char buff[20] = {0};
  snprintf(buff, 20, "Bios version: %s", BIOS_VERSION);
  ILI9341_Draw_Text(buff, 118, 210, BLACK, 1, WHITE);   // 17 * 5 = 85 ; 160 - 42
}

static void SD_Card_Print_info(void) {
    if (sd_error == 0) {
       /* snprintf(buff, 64, "BS:%lu", hsd.SdCard.BlockSize);
        ILI9341_Draw_Text(buff, 0, 15, WHITE, 2, BLACK);
        snprintf(buff, 64, "Bnbr:%lu", hsd.SdCard.BlockNbr);
        ILI9341_Draw_Text(buff, 0, 30, WHITE, 2, BLACK);
        snprintf(buff, 64, "CS:%lu", hsd.SdCard.BlockSize * hsd.SdCard.BlockNbr / 1000);
        ILI9341_Draw_Text(buff, 0, 45, WHITE, 2, BLACK);
        snprintf(buff, 64, "VER:%lu", hsd.SdCard.CardVersion);
        ILI9341_Draw_Text(buff, 0, 60, WHITE, 2, BLACK);
        unsigned int delta = 0;
        f_opendir(&dir, "/");
        do {
            f_readdir(&dir, &fno);
            if (fno.fname[0] != 0) {
                snprintf(buff, 64, "%d: %s", delta+1, fno.fname);
                ILI9341_Draw_Text(buff, 10, 75+(delta*15), WHITE, 2, BLACK);
                delta++;
            }
        } while (fno.fname[0] != 0);
        f_closedir(&dir);*/
    } else {
        ILI9341_Draw_Text("SD CARD ERROR", 0, 220, WHITE, 2, BLACK);
    }
}

static void ILI9341_FPS_Test(void) {
  char buff[20] = {0};
  uint32_t tickstart = HAL_GetTick();
  ILI9341_Fill_Screen(BLUE);
  ILI9341_Fill_Screen(GREEN);
  ILI9341_Fill_Screen(PINK);
  ILI9341_Fill_Screen(OLIVE);
  ILI9341_Fill_Screen(NAVY);
  ILI9341_Fill_Screen(PURPLE);
  ILI9341_Fill_Screen(MAROON);
  ILI9341_Fill_Screen(LIGHTGREY);
  ILI9341_Fill_Screen(CYAN);
  ILI9341_Fill_Screen(MAGENTA);
  ILI9341_Fill_Screen(YELLOW);
  ILI9341_Fill_Screen(ORANGE);
  ILI9341_Fill_Screen(GREENYELLOW);
  ILI9341_Fill_Screen(WHITE);
  ILI9341_Fill_Screen(BLUE);
  ILI9341_Fill_Screen(GREEN);
  ILI9341_Fill_Screen(PINK);
  ILI9341_Fill_Screen(OLIVE);
  ILI9341_Fill_Screen(NAVY);
  ILI9341_Fill_Screen(PURPLE);
  ILI9341_Fill_Screen(MAROON);
  ILI9341_Fill_Screen(LIGHTGREY);
  ILI9341_Fill_Screen(CYAN);
  ILI9341_Fill_Screen(MAGENTA);
  ILI9341_Fill_Screen(YELLOW);
  ILI9341_Fill_Screen(ORANGE);
  uint32_t secs = (HAL_GetTick() - tickstart);
  if (secs <= 0) secs = 1;
  uint32_t frate = 25000 / (secs);
  ILI9341_Fill_Screen(BLACK);
  snprintf(buff, 24, "FPS:%lu(%lu)", frate,secs);
  ILI9341_Draw_Text(buff, 150, 0, WHITE, 1, BLACK);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
    HAL_Delay(300);
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
