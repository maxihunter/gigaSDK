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

#ifdef AUDIO_ERROR_DIAGNOSTICS
typedef enum
{
  AUDIO_ERROR_NONE = 0,
  AUDIO_ERROR_ARGUMENT,
  AUDIO_ERROR_CLOCK,
  AUDIO_ERROR_FILE_OPEN,
  AUDIO_ERROR_HEADER_READ,
  AUDIO_ERROR_HEADER_INVALID,
  AUDIO_ERROR_SAMPLE_RATE,
  AUDIO_ERROR_FILE_TRUNCATED,
  AUDIO_ERROR_STREAM_READ,
  AUDIO_ERROR_PLAYBACK
} Audio_Error;
#endif

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

/* Only present in diagnostic builds compiled with AUDIO_ERROR_DIAGNOSTICS. */
#ifdef AUDIO_ERROR_DIAGNOSTICS
Audio_Error Audio_GetLastError(void);
#endif

/* Short pulse-wave boot jingle with a pseudo-polyphonic chord ending. */
HAL_StatusTypeDef Audio_PlayTestBeep(void);

/*
 * Plays a headerless file of signed 16-bit little-endian samples interleaved
 * L, R, L, R at the configured sample rate, as produced by
 * sdk/tools/audio-to-pcm.sh. The volume must already be mounted.
 */
HAL_StatusTypeDef Audio_PlayPcmFile(const char *path);

/*
 * Plays a GIMA v1 IMA ADPCM file produced by sdk/tools/audio-to-adpcm.py.
 * Use the .gim extension on FAT volumes configured for 8.3 file names.
 * Mono streams are duplicated to both I2S channels; stereo is preserved.
 */
HAL_StatusTypeDef Audio_PlayImaAdpcmFile(const char *path);

/* ------------------------------------------------------------------ mixer */

/* Starts non-blocking background playback from SD. loop != 0 rewinds at EOF. */
HAL_StatusTypeDef Audio_MixerStartPcmMusic(const char *path, uint8_t loop);
HAL_StatusTypeDef Audio_MixerStartImaAdpcmMusic(const char *path, uint8_t loop);
/* Starts the DMA mixer without a music source. Useful for UI sounds. */
HAL_StatusTypeDef Audio_MixerStartSilence(void);

/* Short non-blocking navigation click. Starts a silent mixer if necessary. */
HAL_StatusTypeDef Audio_PlayUiClick(void);
/* Short brighter effect intended for collisions in simple games. */
HAL_StatusTypeDef Audio_PlayUiBounce(void);

/*
 * Mixes one memory-resident GIMA effect over the music. data must remain valid
 * until the effect ends or Audio_MixerStopEffect() is called. volume is Q15:
 * 0 = mute, 16384 = 50 %, 32767 = 100 %.
 */
HAL_StatusTypeDef Audio_MixerPlayImaAdpcmEffect(const uint8_t *data,
                                                uint32_t size,
                                                uint16_t volume);
void Audio_MixerStopEffect(void);
void Audio_MixerSetMusicVolume(uint16_t volume);

/* Must be called more often than once per DMA half (about 46 ms currently). */
HAL_StatusTypeDef Audio_MixerProcess(void);
HAL_StatusTypeDef Audio_MixerStop(void);
uint8_t Audio_MixerIsRunning(void);

#endif
