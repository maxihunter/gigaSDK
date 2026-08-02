/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * @file    bsp_driver_sd.c for F4 (based on stm324x9i_eval_sd.c)
 * @brief   This file includes a generic uSD card driver.
 *          To be completed by the user according to the board used for the project.
 * @note    Some functions generated as weak: they can be overridden by
 *          - code in user files
 *          - or BSP code from the FW pack files
 *          if such files are added to the generated project (by the user).
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

#ifdef OLD_API
/* kept to avoid issue when migrating old projects. */
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
#else
/* USER CODE BEGIN FirstSection */
/* can be used to modify / undefine following code or add new definitions */
/* USER CODE END FirstSection */
/* Includes ------------------------------------------------------------------*/
#include "bsp_driver_sd.h"

#ifdef SDIO_DIAGNOSTICS
#include <stdio.h>
#define SD_LOG(...) printf("[SD] " __VA_ARGS__)
#else
#define SD_LOG(...) ((void)0)
#endif

/* Extern variables ---------------------------------------------------------*/

extern SD_HandleTypeDef hsd;

#define SD_SWITCH_CHECK_HIGH_SPEED       0x00FFFFF1U
#define SD_SWITCH_ENABLE_HIGH_SPEED      0x80FFFFF1U
#define SD_SWITCH_STATUS_SIZE            64U
#define SD_SWITCH_HIGH_SPEED_SUPPORTED   0x02U
#define SD_SWITCH_HIGH_SPEED_SELECTED    0x01U
#define SD_SWITCH_TIMEOUT_MS             100U

static uint8_t sd_high_speed_enabled;
static uint32_t sd_speed_test_block[512U / sizeof(uint32_t)];

static void BSP_SD_SetDefaultSpeed(void)
{
  hsd.Instance->DCTRL = 0U;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockDiv = 0U;
  (void)SDIO_Init(hsd.Instance, hsd.Init);
  __HAL_SD_CLEAR_FLAG(&hsd, SDIO_STATIC_FLAGS);
  hsd.ErrorCode = HAL_SD_ERROR_NONE;
  hsd.State = HAL_SD_STATE_READY;
  SD_LOG("clock fallback: CLKCR=%08lx (24 MHz)\r\n",
         (unsigned long)hsd.Instance->CLKCR);
}

static uint8_t BSP_SD_ReadSwitchStatus(uint32_t argument,
                                       uint8_t status[SD_SWITCH_STATUS_SIZE])
{
  SDIO_DataInitTypeDef config;
  uint32_t tickstart = HAL_GetTick();
  uint32_t word_count = 0U;
  uint32_t errorstate;

  __HAL_SD_CLEAR_FLAG(&hsd, SDIO_STATIC_FLAGS);

  config.DataTimeOut = SDMMC_DATATIMEOUT;
  config.DataLength = SD_SWITCH_STATUS_SIZE;
  config.DataBlockSize = SDIO_DATABLOCK_SIZE_64B;
  config.TransferDir = SDIO_TRANSFER_DIR_TO_SDIO;
  config.TransferMode = SDIO_TRANSFER_MODE_BLOCK;
  config.DPSM = SDIO_DPSM_ENABLE;
  (void)SDIO_ConfigData(hsd.Instance, &config);

  errorstate = SDMMC_CmdSwitch(hsd.Instance, argument);
  if (errorstate != HAL_SD_ERROR_NONE)
  {
    SD_LOG("CMD6 arg=%08lx failed: err=%08lx STA=%08lx RESP1=%08lx\r\n",
           (unsigned long)argument, (unsigned long)errorstate,
           (unsigned long)hsd.Instance->STA,
           (unsigned long)SDIO_GetResponse(hsd.Instance, SDIO_RESP1));
    __HAL_SD_CLEAR_FLAG(&hsd, SDIO_STATIC_FLAGS);
    return MSD_ERROR;
  }

  while (!__HAL_SD_GET_FLAG(&hsd, SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL |
                                   SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DBCKEND))
  {
    if (__HAL_SD_GET_FLAG(&hsd, SDIO_FLAG_RXDAVL))
    {
      uint32_t word = SDIO_ReadFIFO(hsd.Instance);

      if (word_count < (SD_SWITCH_STATUS_SIZE / sizeof(uint32_t)))
      {
        /* SDIO FIFO words contain the first wire byte in bits 7:0. */
        status[word_count * 4U + 0U] = (uint8_t)(word >> 0U);
        status[word_count * 4U + 1U] = (uint8_t)(word >> 8U);
        status[word_count * 4U + 2U] = (uint8_t)(word >> 16U);
        status[word_count * 4U + 3U] = (uint8_t)(word >> 24U);
        word_count++;
      }
    }

    if ((HAL_GetTick() - tickstart) >= SD_SWITCH_TIMEOUT_MS)
    {
      SD_LOG("CMD6 arg=%08lx data timeout: words=%lu STA=%08lx DCTRL=%08lx\r\n",
             (unsigned long)argument, (unsigned long)word_count,
             (unsigned long)hsd.Instance->STA,
             (unsigned long)hsd.Instance->DCTRL);
      __HAL_SD_CLEAR_FLAG(&hsd, SDIO_STATIC_FLAGS);
      return MSD_ERROR;
    }
  }

  while (__HAL_SD_GET_FLAG(&hsd, SDIO_FLAG_RXDAVL))
  {
    uint32_t word = SDIO_ReadFIFO(hsd.Instance);

    if (word_count < (SD_SWITCH_STATUS_SIZE / sizeof(uint32_t)))
    {
      status[word_count * 4U + 0U] = (uint8_t)(word >> 0U);
      status[word_count * 4U + 1U] = (uint8_t)(word >> 8U);
      status[word_count * 4U + 2U] = (uint8_t)(word >> 16U);
      status[word_count * 4U + 3U] = (uint8_t)(word >> 24U);
      word_count++;
    }
  }

  if (__HAL_SD_GET_FLAG(&hsd, SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL |
                              SDIO_FLAG_DTIMEOUT) ||
      (word_count != (SD_SWITCH_STATUS_SIZE / sizeof(uint32_t))))
  {
    SD_LOG("CMD6 arg=%08lx data failed: words=%lu STA=%08lx\r\n",
           (unsigned long)argument, (unsigned long)word_count,
           (unsigned long)hsd.Instance->STA);
    __HAL_SD_CLEAR_FLAG(&hsd, SDIO_STATIC_FLAGS);
    return MSD_ERROR;
  }

  SD_LOG("CMD6 arg=%08lx ok: support=%02x selected=%02x\r\n",
         (unsigned long)argument, status[13], status[16]);
  __HAL_SD_CLEAR_FLAG(&hsd, SDIO_STATIC_DATA_FLAGS);
  return MSD_OK;
}

static uint8_t BSP_SD_EnableHighSpeed(void)
{
  uint8_t status[SD_SWITCH_STATUS_SIZE];

  /* CMD6 check mode: group 1 bit 1 announces High-Speed support. */
  if (BSP_SD_ReadSwitchStatus(SD_SWITCH_CHECK_HIGH_SPEED, status) != MSD_OK)
  {
    SD_LOG("High-Speed check command failed\r\n");
    return MSD_ERROR;
  }
  if ((status[13] & SD_SWITCH_HIGH_SPEED_SUPPORTED) == 0U)
  {
    SD_LOG("High-Speed not supported: status[13]=%02x\r\n", status[13]);
    return MSD_ERROR;
  }

  /* CMD6 switch mode, then verify the selected function in group 1. */
  if (BSP_SD_ReadSwitchStatus(SD_SWITCH_ENABLE_HIGH_SPEED, status) != MSD_OK)
  {
    SD_LOG("High-Speed switch command failed\r\n");
    return MSD_ERROR;
  }
  if ((status[16] & 0x0FU) != SD_SWITCH_HIGH_SPEED_SELECTED)
  {
    SD_LOG("High-Speed not selected: status[16]=%02x\r\n", status[16]);
    return MSD_ERROR;
  }

  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_ENABLE;
  hsd.Init.ClockDiv = 0U;
  (void)SDIO_Init(hsd.Instance, hsd.Init);
  SD_LOG("High-Speed clock enabled: CLKCR=%08lx (48 MHz)\r\n",
         (unsigned long)hsd.Instance->CLKCR);
  return MSD_OK;
}

/* USER CODE BEGIN BeforeInitSection */
/* can be used to modify / undefine following code or add code */
/* USER CODE END BeforeInitSection */
/**
  * @brief  Initializes the SD card device.
  * @retval SD status
  */
__weak uint8_t BSP_SD_Init(void)
{
  uint8_t sd_state = MSD_OK;
  uint8_t detected = BSP_SD_IsDetected();

  SD_LOG("init begin: detect=%u CLKCR=%08lx\r\n", detected,
         (unsigned long)hsd.Instance->CLKCR);
  /* Check if the SD card is plugged in the slot */
  if (detected != SD_PRESENT)
  {
    SD_LOG("card detect reports not present\r\n");
    return MSD_ERROR;
  }
  /* HAL SD initialization */
  sd_state = HAL_SD_Init(&hsd);
  SD_LOG("HAL_SD_Init=%u error=%08lx state=%u card_type=%lu version=%lu blocks=%lu CLKCR=%08lx\r\n",
         sd_state, (unsigned long)hsd.ErrorCode, (unsigned int)hsd.State,
         (unsigned long)hsd.SdCard.CardType,
         (unsigned long)hsd.SdCard.CardVersion,
         (unsigned long)hsd.SdCard.LogBlockNbr,
         (unsigned long)hsd.Instance->CLKCR);
  if ((sd_state == MSD_OK) &&
      (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B) != HAL_OK))
  {
    SD_LOG("4-bit switch failed: error=%08lx STA=%08lx\r\n",
           (unsigned long)hsd.ErrorCode, (unsigned long)hsd.Instance->STA);
    sd_state = MSD_ERROR;
  }
  else if (sd_state == MSD_OK)
  {
    /* This HAL version configures CLKCR but does not update the handle. */
    hsd.Init.BusWide = SDIO_BUS_WIDE_4B;
    SD_LOG("4-bit enabled: CLKCR=%08lx\r\n",
           (unsigned long)hsd.Instance->CLKCR);
  }

  sd_high_speed_enabled = 0U;
  if ((sd_state == MSD_OK) && (BSP_SD_EnableHighSpeed() == MSD_OK))
  {
    /* Do not expose an unusable 48 MHz bus to FatFs.  Some cards accept CMD6,
       while the board wiring or adapter cannot transfer reliably at 48 MHz. */
    HAL_StatusTypeDef read_status = HAL_SD_ReadBlocks(
        &hsd, (uint8_t *)sd_speed_test_block, 0U, 1U,
        SD_SWITCH_TIMEOUT_MS);
    HAL_SD_CardStateTypeDef card_state = HAL_SD_GetCardState(&hsd);

    if ((read_status == HAL_OK) && (card_state == HAL_SD_CARD_TRANSFER))
    {
      sd_high_speed_enabled = 1U;
      SD_LOG("48 MHz verification ok: sector=ok CMD13_state=%u\r\n",
             (unsigned int)card_state);
    }
    else
    {
      SD_LOG("48 MHz verification failed: read=%u CMD13_state=%u error=%08lx STA=%08lx\r\n",
             (unsigned int)read_status, (unsigned int)card_state,
             (unsigned long)hsd.ErrorCode,
             (unsigned long)hsd.Instance->STA);
      BSP_SD_SetDefaultSpeed();
      card_state = HAL_SD_GetCardState(&hsd);
      SD_LOG("24 MHz CMD13 retry: state=%u error=%08lx STA=%08lx\r\n",
             (unsigned int)card_state, (unsigned long)hsd.ErrorCode,
             (unsigned long)hsd.Instance->STA);
    }
  }
  else if (sd_state == MSD_OK)
  {
    /* Also reset the data path after an unsupported/failed CMD6 check. */
    BSP_SD_SetDefaultSpeed();
  }

  SD_LOG("init end: result=%u high_speed=%u error=%08lx state=%u CLKCR=%08lx\r\n",
         sd_state, sd_high_speed_enabled, (unsigned long)hsd.ErrorCode,
         (unsigned int)hsd.State, (unsigned long)hsd.Instance->CLKCR);
  return sd_state;
}

uint8_t BSP_SD_IsHighSpeed(void)
{
  return sd_high_speed_enabled;
}
/* USER CODE BEGIN AfterInitSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END AfterInitSection */

/* USER CODE BEGIN InterruptMode */
/**
  * @brief  Configures Interrupt mode for SD detection pin.
  * @retval Returns 0
  */
__weak uint8_t BSP_SD_ITConfig(void)
{
  /* Code to be updated by the user or replaced by one from the FW pack (in a stmxxxx_sd.c file) */

  return (uint8_t)0;
}

/** @brief  SD detect IT treatment
  */
__weak void BSP_SD_DetectIT(void)
{
  /* Code to be updated by the user or replaced by one from the FW pack (in a stmxxxx_sd.c file) */
}
/* USER CODE END InterruptMode */

/* USER CODE BEGIN BeforeReadBlocksSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeReadBlocksSection */
/**
  * @brief  Reads block(s) from a specified address in an SD card, in polling mode.
  * @param  pData: Pointer to the buffer that will contain the data to transmit
  * @param  ReadAddr: Address from where data is to be read
  * @param  NumOfBlocks: Number of SD blocks to read
  * @param  Timeout: Timeout for read operation
  * @retval SD status
  */
__weak uint8_t BSP_SD_ReadBlocks(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks, uint32_t Timeout)
{
  uint8_t sd_state = MSD_OK;

  if (HAL_SD_ReadBlocks(&hsd, (uint8_t *)pData, ReadAddr, NumOfBlocks, Timeout) != HAL_OK)
  {
    sd_state = MSD_ERROR;
  }

  return sd_state;
}

/* USER CODE BEGIN BeforeWriteBlocksSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeWriteBlocksSection */
/**
  * @brief  Writes block(s) to a specified address in an SD card, in polling mode.
  * @param  pData: Pointer to the buffer that will contain the data to transmit
  * @param  WriteAddr: Address from where data is to be written
  * @param  NumOfBlocks: Number of SD blocks to write
  * @param  Timeout: Timeout for write operation
  * @retval SD status
  */
__weak uint8_t BSP_SD_WriteBlocks(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks, uint32_t Timeout)
{
  uint8_t sd_state = MSD_OK;

  if (HAL_SD_WriteBlocks(&hsd, (uint8_t *)pData, WriteAddr, NumOfBlocks, Timeout) != HAL_OK)
  {
    sd_state = MSD_ERROR;
  }

  return sd_state;
}

/* USER CODE BEGIN BeforeReadDMABlocksSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeReadDMABlocksSection */
/**
  * @brief  Reads block(s) from a specified address in an SD card, in DMA mode.
  * @param  pData: Pointer to the buffer that will contain the data to transmit
  * @param  ReadAddr: Address from where data is to be read
  * @param  NumOfBlocks: Number of SD blocks to read
  * @retval SD status
  */
__weak uint8_t BSP_SD_ReadBlocks_DMA(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks)
{
  uint8_t sd_state = MSD_OK;

  /* Read block(s) in DMA transfer mode */
  if (HAL_SD_ReadBlocks_DMA(&hsd, (uint8_t *)pData, ReadAddr, NumOfBlocks) != HAL_OK)
  {
    sd_state = MSD_ERROR;
  }

  return sd_state;
}

/* USER CODE BEGIN BeforeWriteDMABlocksSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeWriteDMABlocksSection */
/**
  * @brief  Writes block(s) to a specified address in an SD card, in DMA mode.
  * @param  pData: Pointer to the buffer that will contain the data to transmit
  * @param  WriteAddr: Address from where data is to be written
  * @param  NumOfBlocks: Number of SD blocks to write
  * @retval SD status
  */
__weak uint8_t BSP_SD_WriteBlocks_DMA(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks)
{
  uint8_t sd_state = MSD_OK;

  /* Write block(s) in DMA transfer mode */
  if (HAL_SD_WriteBlocks_DMA(&hsd, (uint8_t *)pData, WriteAddr, NumOfBlocks) != HAL_OK)
  {
    sd_state = MSD_ERROR;
  }

  return sd_state;
}

/* USER CODE BEGIN BeforeEraseSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeEraseSection */
/**
  * @brief  Erases the specified memory area of the given SD card.
  * @param  StartAddr: Start byte address
  * @param  EndAddr: End byte address
  * @retval SD status
  */
__weak uint8_t BSP_SD_Erase(uint32_t StartAddr, uint32_t EndAddr)
{
  uint8_t sd_state = MSD_OK;

  if (HAL_SD_Erase(&hsd, StartAddr, EndAddr) != HAL_OK)
  {
    sd_state = MSD_ERROR;
  }

  return sd_state;
}

/**
  * @brief  Gets the current SD card data status.
  * @param  None
  * @retval Data transfer state.
  *          This value can be one of the following values:
  *            @arg  SD_TRANSFER_OK: No data transfer is acting
  *            @arg  SD_TRANSFER_BUSY: Data transfer is acting
  */
__weak uint8_t BSP_SD_GetCardState(void)
{
  HAL_SD_CardStateTypeDef card_state = HAL_SD_GetCardState(&hsd);

  SD_LOG("GetCardState: state=%u error=%08lx STA=%08lx CLKCR=%08lx\r\n",
         (unsigned int)card_state, (unsigned long)hsd.ErrorCode,
         (unsigned long)hsd.Instance->STA,
         (unsigned long)hsd.Instance->CLKCR);
  return ((card_state == HAL_SD_CARD_TRANSFER) ?
          SD_TRANSFER_OK : SD_TRANSFER_BUSY);
}

/**
  * @brief  Get SD information about specific SD card.
  * @param  CardInfo: Pointer to HAL_SD_CardInfoTypedef structure
  * @retval None
  */
__weak void BSP_SD_GetCardInfo(HAL_SD_CardInfoTypeDef *CardInfo)
{
  /* Get SD card Information */
  HAL_SD_GetCardInfo(&hsd, CardInfo);
}

/* USER CODE BEGIN BeforeCallBacksSection */
/* can be used to modify previous code / undefine following code / add code */
/* USER CODE END BeforeCallBacksSection */
/**
  * @brief SD Abort callbacks
  * @param hsd: SD handle
  * @retval None
  */
void HAL_SD_AbortCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_AbortCallback();
}

/**
  * @brief Tx Transfer completed callback
  * @param hsd: SD handle
  * @retval None
  */
void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_WriteCpltCallback();
}

/**
  * @brief Rx Transfer completed callback
  * @param hsd: SD handle
  * @retval None
  */
void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_ReadCpltCallback();
}

/* USER CODE BEGIN CallBacksSection_C */
/**
  * @brief BSP SD Abort callback
  * @retval None
  * @note empty (up to the user to fill it in or to remove it if useless)
  */
__weak void BSP_SD_AbortCallback(void)
{

}

/**
  * @brief BSP Tx Transfer completed callback
  * @retval None
  * @note empty (up to the user to fill it in or to remove it if useless)
  */
__weak void BSP_SD_WriteCpltCallback(void)
{

}

/**
  * @brief BSP Rx Transfer completed callback
  * @retval None
  * @note empty (up to the user to fill it in or to remove it if useless)
  */
__weak void BSP_SD_ReadCpltCallback(void)
{

}
/* USER CODE END CallBacksSection_C */
#endif

/**
 * @brief  Detects if SD card is correctly plugged in the memory slot or not.
 * @param  None
 * @retval Returns if SD is detected or not
 */
__weak uint8_t BSP_SD_IsDetected(void)
{
  __IO uint8_t status = SD_PRESENT;

  if (BSP_PlatformIsDetected() == 0x0)
  {
    status = SD_NOT_PRESENT;
  }

  return status;
}

/* USER CODE BEGIN AdditionalCode */
/* user code can be inserted here */
/* USER CODE END AdditionalCode */
