#include "main.h"

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

void HAL_MspInit(void) { __HAL_RCC_SYSCFG_CLK_ENABLE(); __HAL_RCC_PWR_CLK_ENABLE(); }

void HAL_SPI_MspInit(SPI_HandleTypeDef *spi)
{
    if (spi->Instance != SPI1) return;
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_SPI1_CLK_ENABLE(); __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_5 | GPIO_PIN_7; gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL; gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF5_SPI1; HAL_GPIO_Init(GPIOA, &gpio);
    __HAL_RCC_DMA2_CLK_ENABLE();
    hdma_spi1_tx.Instance = DMA2_Stream3; hdma_spi1_tx.Init.Channel = DMA_CHANNEL_3;
    hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE; hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi1_tx.Init.Mode = DMA_NORMAL; hdma_spi1_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    hdma_spi1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK) Error_Handler();
    __HAL_LINKDMA(spi, hdmatx, hdma_spi1_tx);
}

static void clock_init(void)
{
    RCC_OscInitTypeDef osc = {0}; RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE(); __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE; osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON; osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 4U; osc.PLL.PLLN = 168U; osc.PLL.PLLP = RCC_PLLP_DIV2; osc.PLL.PLLQ = 7U;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV4; clk.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK) Error_Handler();
}

void RacingClub_PlatformInit(void)
{
    __disable_irq(); HAL_Init(); clock_init();
    __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE(); __HAL_RCC_GPIOE_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1; gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL; gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH; HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_6; HAL_GPIO_Init(GPIOC, &gpio);
    gpio.Mode = GPIO_MODE_INPUT; gpio.Pull = GPIO_PULLUP;
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7; HAL_GPIO_Init(GPIOE, &gpio);
    gpio.Pin = GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9; HAL_GPIO_Init(GPIOB, &gpio);
    hspi1.Instance = SPI1; hspi1.Init.Mode = SPI_MODE_MASTER; hspi1.Init.Direction = SPI_DIRECTION_1LINE;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT; hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE; hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2; hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE; hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10U;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) Error_Handler();
    HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 1U, 0U); HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
    __enable_irq();
}
void SysTick_Handler(void) { HAL_IncTick(); }
void DMA2_Stream3_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_spi1_tx); }
void Error_Handler(void) { __disable_irq(); for (;;) {} }
