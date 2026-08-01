#ifndef VIDEO_H
#define VIDEO_H

#include "stm32f4xx_hal.h"

#include <stdint.h>

#define VIDEO_GVID_HEADER_SIZE 24U
#define VIDEO_GVID_VERSION     1U

typedef void (*Video_ServiceHandler)(void);
typedef uint8_t (*Video_AbortHandler)(void);

typedef struct
{
  uint16_t width;
  uint16_t height;
  uint32_t fps_milli;
  uint32_t frame_count;
} Video_Info;

HAL_StatusTypeDef Video_Init(SPI_HandleTypeDef *display_spi);

/*
 * Draws one big-endian RGB565 frame, centred on the 320x240 display. The frame
 * pointer must contain exactly width * height * 2 bytes.
 */
HAL_StatusTypeDef Video_DrawRgb565Frame(const uint8_t *frame,
                                        uint16_t width, uint16_t height);

/* Reads and validates only the GVID header. */
HAL_StatusTypeDef Video_GetFileInfo(const char *path, Video_Info *info);

/*
 * Streams an uncompressed .vid file from FatFs. service is called between SD
 * and SPI chunks and while waiting for the next frame; it may be NULL. abort
 * may be NULL or return non-zero to stop playback normally.
 */
HAL_StatusTypeDef Video_PlayFile(const char *path,
                                 Video_ServiceHandler service,
                                 Video_AbortHandler abort);

#endif
