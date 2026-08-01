#ifndef AUDIO_H
#define AUDIO_H

#include "stm32f4xx_hal.h"

/*
 * Playback of 16-bit stereo audio through an I2S DAC such as the PCM5102A.
 *
 * The application owns the hardware: it configures the I2S peripheral together
 * with a circular DMA stream and hands the handle to Audio_Init(). Samples then
 * travel through an internal double buffer whose halves are refilled from the
 * context that called the play function, never from the DMA interrupt, so a
 * source may block on slow storage.
 */

/* Polled while audio plays; a non-zero result stops the stream early. */
typedef uint8_t (*Audio_AbortHandler)(void);

typedef struct
{
  uint32_t i2s_clock;      /* clock feeding the I2S prescaler, Hz */
  uint32_t prescaler;      /* 2 * I2SDIV + ODD */
  uint32_t bits_per_frame; /* bit clocks per stereo frame, 32 or 64 */
  uint32_t bit_clock;      /* BCK frequency, Hz */
  uint32_t sample_rate;    /* stereo frames per second */
} Audio_ClockInfo;

/*
 * The handle must already be initialised as master transmit with a circular DMA
 * stream linked to it. abort may be NULL when early stopping is not wanted.
 */
HAL_StatusTypeDef Audio_Init(I2S_HandleTypeDef *i2s, Audio_AbortHandler abort);

/* Rates the prescaler actually produces, which differ slightly from the
   AudioFreq that was requested at initialisation. */
HAL_StatusTypeDef Audio_GetClockInfo(Audio_ClockInfo *info);

/* Three short sine bursts of rising pitch for checking the DAC wiring. */
HAL_StatusTypeDef Audio_PlayTestBeep(void);

/*
 * Plays a headerless file of signed 16-bit little-endian samples interleaved
 * L, R, L, R at the configured sample rate, as produced by
 * sdk/tools/audio-to-pcm.sh. The volume must already be mounted.
 */
HAL_StatusTypeDef Audio_PlayPcmFile(const char *path);

/*
 * Plays a GIMA v1 IMA ADPCM file produced by sdk/tools/audio-to-adpcm.py.
 * Mono streams are duplicated to both I2S channels; stereo is preserved.
 */
HAL_StatusTypeDef Audio_PlayImaAdpcmFile(const char *path);

#endif
