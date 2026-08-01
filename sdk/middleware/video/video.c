#include "video/video.h"

#include "ff.h"
#include "ili9341/ILI9341_STM32_Driver.h"

#include <string.h>

#define VIDEO_PIXEL_FORMAT_RGB565_BE 1U
#define VIDEO_IO_BUFFER_SIZE         2048U

static SPI_HandleTypeDef *video_spi;
static uint8_t video_io_buffer[VIDEO_IO_BUFFER_SIZE] __attribute__((aligned(4)));

static uint16_t Video_ReadU16(const uint8_t *value)
{
  return (uint16_t)value[0] | ((uint16_t)value[1] << 8);
}

static uint32_t Video_ReadU32(const uint8_t *value)
{
  return (uint32_t)value[0] |
         ((uint32_t)value[1] << 8) |
         ((uint32_t)value[2] << 16) |
         ((uint32_t)value[3] << 24);
}

static HAL_StatusTypeDef Video_ParseHeader(const uint8_t *header,
                                           Video_Info *info)
{
  if ((header == NULL) || (info == NULL) ||
      (memcmp(header, "GVID", 4U) != 0) ||
      (header[4] != VIDEO_GVID_VERSION) ||
      (header[5] != VIDEO_PIXEL_FORMAT_RGB565_BE) ||
      (Video_ReadU16(&header[6]) != VIDEO_GVID_HEADER_SIZE))
  {
    return HAL_ERROR;
  }

  info->width = Video_ReadU16(&header[8]);
  info->height = Video_ReadU16(&header[10]);
  info->fps_milli = Video_ReadU32(&header[12]);
  info->frame_count = Video_ReadU32(&header[16]);

  if ((info->width == 0U) || (info->height == 0U) ||
      (info->width > ILI9341_SCREEN_WIDTH) ||
      (info->height > ILI9341_SCREEN_HEIGHT) ||
      (info->fps_milli == 0U) || (info->frame_count == 0U))
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

static HAL_StatusTypeDef Video_BeginPixels(uint16_t x, uint16_t y,
                                           uint16_t width, uint16_t height)
{
  if ((video_spi == NULL) || (width == 0U) || (height == 0U))
  {
    return HAL_ERROR;
  }

  ILI9341_Set_Address(x, y, x + width - 1U, y + height - 1U);
  HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
  return HAL_OK;
}

static void Video_EndPixels(void)
{
  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

static HAL_StatusTypeDef Video_ClearBlack(Video_ServiceHandler service)
{
  memset(video_io_buffer, 0, sizeof(video_io_buffer));
  if (Video_BeginPixels(0U, 0U, ILI9341_SCREEN_WIDTH,
                        ILI9341_SCREEN_HEIGHT) != HAL_OK)
  {
    return HAL_ERROR;
  }

  uint32_t bytes_remaining =
      ILI9341_SCREEN_WIDTH * ILI9341_SCREEN_HEIGHT * 2U;
  while (bytes_remaining != 0U)
  {
    uint16_t chunk = (bytes_remaining > VIDEO_IO_BUFFER_SIZE) ?
        VIDEO_IO_BUFFER_SIZE : (uint16_t)bytes_remaining;
    if (HAL_SPI_Transmit(video_spi, video_io_buffer, chunk, 100U) != HAL_OK)
    {
      Video_EndPixels();
      return HAL_ERROR;
    }
    bytes_remaining -= chunk;
    if (service != NULL)
    {
      service();
    }
  }

  Video_EndPixels();
  return HAL_OK;
}

HAL_StatusTypeDef Video_Init(SPI_HandleTypeDef *display_spi)
{
  if (display_spi == NULL)
  {
    return HAL_ERROR;
  }
  video_spi = display_spi;
  return HAL_OK;
}

HAL_StatusTypeDef Video_DrawRgb565Frame(const uint8_t *frame,
                                        uint16_t width, uint16_t height)
{
  if ((frame == NULL) || (width == 0U) || (height == 0U) ||
      (width > ILI9341_SCREEN_WIDTH) || (height > ILI9341_SCREEN_HEIGHT))
  {
    return HAL_ERROR;
  }

  uint16_t x = (ILI9341_SCREEN_WIDTH - width) / 2U;
  uint16_t y = (ILI9341_SCREEN_HEIGHT - height) / 2U;
  if (Video_BeginPixels(x, y, width, height) != HAL_OK)
  {
    return HAL_ERROR;
  }

  uint32_t bytes_remaining = (uint32_t)width * height * 2U;
  while (bytes_remaining != 0U)
  {
    uint16_t chunk = (bytes_remaining > VIDEO_IO_BUFFER_SIZE) ?
        VIDEO_IO_BUFFER_SIZE : (uint16_t)bytes_remaining;
    if (HAL_SPI_Transmit(video_spi, (uint8_t *)frame, chunk, 100U) != HAL_OK)
    {
      Video_EndPixels();
      return HAL_ERROR;
    }
    frame += chunk;
    bytes_remaining -= chunk;
  }

  Video_EndPixels();
  return HAL_OK;
}

HAL_StatusTypeDef Video_GetFileInfo(const char *path, Video_Info *info)
{
  FIL file;
  uint8_t header[VIDEO_GVID_HEADER_SIZE];
  UINT bytes_read = 0U;

  if ((path == NULL) || (info == NULL) ||
      (f_open(&file, path, FA_READ) != FR_OK))
  {
    return HAL_ERROR;
  }

  HAL_StatusTypeDef status = HAL_ERROR;
  if ((f_read(&file, header, sizeof(header), &bytes_read) == FR_OK) &&
      (bytes_read == sizeof(header)) &&
      (Video_ParseHeader(header, info) == HAL_OK))
  {
    uint64_t frame_bytes = (uint64_t)info->width * info->height * 2U;
    uint64_t required_size = VIDEO_GVID_HEADER_SIZE +
                             frame_bytes * info->frame_count;
    if (required_size <= (uint64_t)f_size(&file))
    {
      status = HAL_OK;
    }
  }

  (void)f_close(&file);
  return status;
}

static HAL_StatusTypeDef Video_StreamFrame(FIL *file, const Video_Info *info,
                                           Video_ServiceHandler service)
{
  uint16_t x = (ILI9341_SCREEN_WIDTH - info->width) / 2U;
  uint16_t y = (ILI9341_SCREEN_HEIGHT - info->height) / 2U;
  uint32_t bytes_remaining = (uint32_t)info->width * info->height * 2U;

  if (Video_BeginPixels(x, y, info->width, info->height) != HAL_OK)
  {
    return HAL_ERROR;
  }

  while (bytes_remaining != 0U)
  {
    UINT requested = (bytes_remaining > VIDEO_IO_BUFFER_SIZE) ?
        VIDEO_IO_BUFFER_SIZE : (UINT)bytes_remaining;
    UINT bytes_read = 0U;
    if ((f_read(file, video_io_buffer, requested, &bytes_read) != FR_OK) ||
        (bytes_read != requested) ||
        (HAL_SPI_Transmit(video_spi, video_io_buffer,
                          (uint16_t)bytes_read, 100U) != HAL_OK))
    {
      Video_EndPixels();
      return HAL_ERROR;
    }

    bytes_remaining -= bytes_read;
    if (service != NULL)
    {
      service();
    }
  }

  Video_EndPixels();
  return HAL_OK;
}

HAL_StatusTypeDef Video_PlayFile(const char *path,
                                 Video_ServiceHandler service,
                                 Video_AbortHandler abort)
{
  FIL file;
  Video_Info info;
  uint8_t header[VIDEO_GVID_HEADER_SIZE];
  UINT bytes_read = 0U;

  if ((video_spi == NULL) || (path == NULL) ||
      (f_open(&file, path, FA_READ) != FR_OK))
  {
    return HAL_ERROR;
  }

  HAL_StatusTypeDef status = HAL_ERROR;
  if ((f_read(&file, header, sizeof(header), &bytes_read) != FR_OK) ||
      (bytes_read != sizeof(header)) ||
      (Video_ParseHeader(header, &info) != HAL_OK))
  {
    goto close_file;
  }

  uint64_t frame_bytes = (uint64_t)info.width * info.height * 2U;
  if ((uint64_t)VIDEO_GVID_HEADER_SIZE + frame_bytes * info.frame_count >
      (uint64_t)f_size(&file))
  {
    goto close_file;
  }

  if (((info.width < ILI9341_SCREEN_WIDTH) ||
       (info.height < ILI9341_SCREEN_HEIGHT)) &&
      (Video_ClearBlack(service) != HAL_OK))
  {
    goto close_file;
  }

  uint32_t start_tick = HAL_GetTick();
  status = HAL_OK;
  for (uint32_t frame = 0U; frame < info.frame_count; frame++)
  {
    if ((abort != NULL) && abort())
    {
      break;
    }
    if (Video_StreamFrame(&file, &info, service) != HAL_OK)
    {
      status = HAL_ERROR;
      break;
    }

    uint32_t deadline = start_tick +
        (uint32_t)(((uint64_t)(frame + 1U) * 1000000U) / info.fps_milli);
    while ((int32_t)(deadline - HAL_GetTick()) > 0)
    {
      if ((abort != NULL) && abort())
      {
        goto close_file;
      }
      if (service != NULL)
      {
        service();
      }
      HAL_Delay(1U);
    }
  }

close_file:
  (void)f_close(&file);
  return status;
}
