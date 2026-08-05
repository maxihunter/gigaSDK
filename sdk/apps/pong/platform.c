#include "main.h"
#include "fatfs.h"
#include "audio/audio.h"

SPI_HandleTypeDef hspi1;
I2S_HandleTypeDef hi2s3;
SD_HandleTypeDef hsd;
DMA_HandleTypeDef hdma_spi1_tx;
DMA_HandleTypeDef hdma_spi3_tx;
DMA_HandleTypeDef hdma_tim1_ch2;

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *spi)
{
    if (spi->Instance != SPI1) return;
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_SPI1_CLK_ENABLE(); __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_5 | GPIO_PIN_7; gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL; gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF5_SPI1; HAL_GPIO_Init(GPIOA, &gpio);
}

void HAL_I2S_MspInit(I2S_HandleTypeDef *i2s)
{
    if (i2s->Instance != SPI3) return;
    RCC_PeriphCLKInitTypeDef clock = {0}; GPIO_InitTypeDef gpio = {0};
    clock.PeriphClockSelection = RCC_PERIPHCLK_I2S;
    clock.PLLI2S.PLLI2SN = 146U; clock.PLLI2S.PLLI2SR = 3U;
    if (HAL_RCCEx_PeriphCLKConfig(&clock) != HAL_OK) Error_Handler();
    __HAL_RCC_SPI3_CLK_ENABLE(); __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio.Mode = GPIO_MODE_AF_PP; gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH; gpio.Alternate = GPIO_AF6_SPI3;
    gpio.Pin = GPIO_PIN_4; HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_3 | GPIO_PIN_5; HAL_GPIO_Init(GPIOB, &gpio);
    hdma_spi3_tx.Instance = DMA1_Stream5; hdma_spi3_tx.Init.Channel = DMA_CHANNEL_0;
    hdma_spi3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi3_tx.Init.PeriphInc = DMA_PINC_DISABLE; hdma_spi3_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_spi3_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_spi3_tx.Init.Mode = DMA_CIRCULAR; hdma_spi3_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_spi3_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_spi3_tx) != HAL_OK) Error_Handler();
    __HAL_LINKDMA(i2s, hdmatx, hdma_spi3_tx);
}

void HAL_SD_MspInit(SD_HandleTypeDef *sd)
{
    if (sd->Instance != SDIO) return;
    GPIO_InitTypeDef gpio = {0}; __HAL_RCC_SDIO_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE(); __HAL_RCC_GPIOD_CLK_ENABLE();
    gpio.Mode = GPIO_MODE_AF_PP; gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF12_SDIO; gpio.Pull = GPIO_PULLUP;
    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
    HAL_GPIO_Init(GPIOC, &gpio); gpio.Pin = GPIO_PIN_2; HAL_GPIO_Init(GPIOD, &gpio);
    gpio.Pin = GPIO_PIN_12; gpio.Pull = GPIO_NOPULL; HAL_GPIO_Init(GPIOC, &gpio);
}

void HAL_SD_MspDeInit(SD_HandleTypeDef *sd)
{
    if (sd->Instance != SDIO) return;
    __HAL_RCC_SDIO_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
                           GPIO_PIN_11 | GPIO_PIN_12);
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);
}

static void clock_init(void)
{
    RCC_OscInitTypeDef osc = {0}; RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE; osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON; osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 4U; osc.PLL.PLLN = 168U;
    osc.PLL.PLLP = RCC_PLLP_DIV2; osc.PLL.PLLQ = 7U;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1; clk.APB1CLKDivider = RCC_HCLK_DIV4;
    clk.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK) Error_Handler();
}

void Pong_PlatformInit(void)
{
    __disable_irq(); HAL_Init(); clock_init();
    __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1; gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL; gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio); gpio.Pin = GPIO_PIN_6; HAL_GPIO_Init(GPIOC, &gpio);
    gpio.Mode = GPIO_MODE_INPUT; gpio.Pull = GPIO_PULLUP;
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7; HAL_GPIO_Init(GPIOE, &gpio);
    gpio.Pin = GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
    HAL_GPIO_Init(GPIOB, &gpio);
    hspi1.Instance = SPI1; hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_1LINE; hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW; hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT; hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB; hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE; hspi1.Init.CRCPolynomial = 10U;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) Error_Handler();
    __HAL_RCC_DMA1_CLK_ENABLE();
    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0U, 0U);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    hsd.Instance = SDIO; hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
    hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
    hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
    hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
    hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
    hsd.Init.ClockDiv = 0U;
    hi2s3.Instance = SPI3; hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
    hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
    hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B_EXTENDED;
    hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
    hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_22K; hi2s3.Init.CPOL = I2S_CPOL_LOW;
    hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
    hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
    if (HAL_I2S_Init(&hi2s3) != HAL_OK) Error_Handler();
    MX_FATFS_Init();
    if (Audio_Init(&hi2s3, NULL) != HAL_OK) Error_Handler();
    __enable_irq();
}

void SysTick_Handler(void) { HAL_IncTick(); }
void DMA1_Stream5_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_spi3_tx); }
void Error_Handler(void) { __disable_irq(); for (;;) {} }
