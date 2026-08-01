#include "audio/audio.h"
#include "codec/ima_adpcm.h"

#include "ff.h"
#include <string.h>

/*
 * Samples travel through a circular DMA buffer whose halves are refilled as the
 * DMA releases them. A blocking HAL_I2S_Transmit() per block does not work: the
 * peripheral holds a single word, so it runs dry while the next block is being
 * prepared and repeats stale samples at every block boundary.
 *
 * The refills run in the caller's context, not in the DMA callbacks. Reading a
 * file polls HAL_GetTick(), and SysTick sits at a lower interrupt priority than
 * the audio DMA stream, so the tick would never advance inside the callback and
 * every storage timeout would hang. The callbacks therefore only flag which half
 * became free.
 */
#define AUDIO_HALF_FRAMES 1024U
#define AUDIO_HALF_WORDS  (AUDIO_HALF_FRAMES * 2U)
#define AUDIO_HALF_BYTES  (AUDIO_HALF_WORDS * sizeof(uint16_t))

/* Fills a half completely, padding with silence, and returns 0 once the source
   has no audio left to produce. */
typedef uint8_t (*Audio_FillHalf)(uint16_t *half);

/* Word aligned: the halves are handed to the storage driver as read buffers,
   and block drivers commonly move them as 32-bit words. */
static uint16_t audio_buffer[AUDIO_HALF_WORDS * 2U] __attribute__((aligned(4)));
static I2S_HandleTypeDef *audio_i2s;
static Audio_AbortHandler audio_abort;
static Audio_FillHalf audio_fill_half;
static uint8_t audio_flushed_halves;
static volatile uint8_t audio_first_half_free;
static volatile uint8_t audio_second_half_free;

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  if ((audio_i2s != NULL) && (hi2s->Instance == audio_i2s->Instance))
  {
    audio_first_half_free = 1U;
  }
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  if ((audio_i2s != NULL) && (hi2s->Instance == audio_i2s->Instance))
  {
    audio_second_half_free = 1U;
  }
}

HAL_StatusTypeDef Audio_GetClockInfo(Audio_ClockInfo *info)
{
  if ((info == NULL) || (audio_i2s == NULL))
  {
    return HAL_ERROR;
  }

  SPI_TypeDef *spi = audio_i2s->Instance;

  memset(info, 0, sizeof(*info));
  info->i2s_clock = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_I2S);
  info->prescaler = 2U * (spi->I2SPR & SPI_I2SPR_I2SDIV);
  info->bits_per_frame = ((spi->I2SCFGR & SPI_I2SCFGR_CHLEN) != 0U) ? 64U : 32U;

  if ((spi->I2SPR & SPI_I2SPR_ODD) != 0U)
  {
    info->prescaler++;
  }
  if ((info->i2s_clock == 0U) || (info->prescaler == 0U))
  {
    return HAL_ERROR;
  }

  /* With the master clock enabled the peripheral always divides down to 256
     clocks per frame, otherwise the channel length sets the frame. */
  uint32_t clocks_per_frame =
      ((spi->I2SPR & SPI_I2SPR_MCKOE) != 0U) ? 256U : info->bits_per_frame;

  info->sample_rate = info->i2s_clock / (clocks_per_frame * info->prescaler);
  info->bit_clock = info->sample_rate * info->bits_per_frame;

  return (info->sample_rate != 0U) ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef Audio_Init(I2S_HandleTypeDef *i2s, Audio_AbortHandler abort)
{
  Audio_ClockInfo info;

  if (i2s == NULL)
  {
    return HAL_ERROR;
  }

  audio_i2s = i2s;
  audio_abort = abort;

  if (Audio_GetClockInfo(&info) != HAL_OK)
  {
    audio_i2s = NULL;
    return HAL_ERROR;
  }

  return HAL_OK;
}

/* Refills whichever half the DMA has finished with. Returns 0 once every queued
   sample has reached the DAC. */
static uint8_t Audio_Pump(void)
{
  uint16_t *half;

  if (audio_first_half_free)
  {
    audio_first_half_free = 0U;
    half = &audio_buffer[0];
  }
  else if (audio_second_half_free)
  {
    audio_second_half_free = 0U;
    half = &audio_buffer[AUDIO_HALF_WORDS];
  }
  else
  {
    return 1U;
  }

  if (audio_fill_half(half))
  {
    return 1U;
  }

  /* The half just filled still carried the tail of the stream, so both halves
     have to be played once more before the DMA may be stopped. */
  if (audio_flushed_halves < 2U)
  {
    audio_flushed_halves++;
    return 1U;
  }

  return 0U;
}

static HAL_StatusTypeDef Audio_Play(Audio_FillHalf fill, uint32_t timeout_ms)
{
  audio_fill_half = fill;
  audio_flushed_halves = 0U;
  audio_first_half_free = 0U;
  audio_second_half_free = 0U;

  (void)fill(&audio_buffer[0]);
  (void)fill(&audio_buffer[AUDIO_HALF_WORDS]);

  HAL_StatusTypeDef status =
      HAL_I2S_Transmit_DMA(audio_i2s, audio_buffer, (uint16_t)(AUDIO_HALF_WORDS * 2U));
  if (status != HAL_OK)
  {
    return status;
  }

  uint32_t tickstart = HAL_GetTick();
  while (Audio_Pump())
  {
    if ((HAL_GetTick() - tickstart) > timeout_ms)
    {
      status = HAL_TIMEOUT;
      break;
    }
    if ((audio_abort != NULL) && audio_abort())
    {
      break;
    }
  }

  if (HAL_I2S_DMAStop(audio_i2s) != HAL_OK)
  {
    status = HAL_ERROR;
  }

  return status;
}

/* ---------------------------------------------------------------- test beep */

#define AUDIO_BEEP_TONE_COUNT 3U

/* One sine period over 32 entries; sampled with linear interpolation and scaled
   to about 73 % of full scale, so the coarse table costs no audible harmonics. */
static const int16_t audio_sine_table[32] = {
     0,  1171,  2296,  3333,  4243,  4989,  5543,  5885,
  6000,  5885,  5543,  4989,  4243,  3333,  2296,  1171,
     0, -1171, -2296, -3333, -4243, -4989, -5543, -5885,
 -6000, -5885, -5543, -4989, -4243, -3333, -2296, -1171
};
static const uint16_t audio_beep_frequencies[AUDIO_BEEP_TONE_COUNT] = {
  500U, 1000U, 2000U
};

static struct
{
  uint32_t sample_rate;
  uint32_t tone_frames;
  uint32_t gap_frames;
  uint32_t fade_frames;
  uint32_t tone;
  uint32_t frame;
  uint32_t phase;
  uint32_t phase_step;
  uint8_t in_gap;
} audio_beep;

static uint32_t Audio_BeepPhaseStep(uint32_t frequency)
{
  return (uint32_t)(((uint64_t)frequency << 32) / audio_beep.sample_rate);
}

static int16_t Audio_BeepNextSample(void)
{
  if (audio_beep.tone >= AUDIO_BEEP_TONE_COUNT)
  {
    return 0;
  }

  if (audio_beep.in_gap)
  {
    audio_beep.frame++;
    if (audio_beep.frame >= audio_beep.gap_frames)
    {
      audio_beep.tone++;
      audio_beep.in_gap = 0U;
      audio_beep.frame = 0U;
      audio_beep.phase = 0U;
      if (audio_beep.tone < AUDIO_BEEP_TONE_COUNT)
      {
        audio_beep.phase_step =
            Audio_BeepPhaseStep(audio_beep_frequencies[audio_beep.tone]);
      }
    }
    return 0;
  }

  uint32_t index = audio_beep.phase >> 27;
  int32_t lower = audio_sine_table[index];
  int32_t upper = audio_sine_table[(index + 1U) & 31U];
  int32_t fraction = (int32_t)((audio_beep.phase >> 11) & 0xFFFFU);
  int32_t sample = (lower + (((upper - lower) * fraction) >> 16)) * 4;

  uint32_t remaining = audio_beep.tone_frames - audio_beep.frame;
  uint32_t gain = audio_beep.fade_frames;
  if (audio_beep.frame < audio_beep.fade_frames)
  {
    gain = audio_beep.frame;
  }
  else if (remaining <= audio_beep.fade_frames)
  {
    gain = remaining - 1U;
  }
  sample = (sample * (int32_t)gain) / (int32_t)audio_beep.fade_frames;

  audio_beep.phase += audio_beep.phase_step;
  audio_beep.frame++;
  if (audio_beep.frame >= audio_beep.tone_frames)
  {
    audio_beep.in_gap = 1U;
    audio_beep.frame = 0U;
  }

  return (int16_t)sample;
}

static uint8_t Audio_BeepFillHalf(uint16_t *half)
{
  for (uint32_t frame = 0U; frame < AUDIO_HALF_FRAMES; frame++)
  {
    uint16_t sample = (uint16_t)Audio_BeepNextSample();
    half[frame * 2U] = sample;
    half[(frame * 2U) + 1U] = sample;
  }

  return (audio_beep.tone < AUDIO_BEEP_TONE_COUNT) ? 1U : 0U;
}

HAL_StatusTypeDef Audio_PlayTestBeep(void)
{
  Audio_ClockInfo info;

  if (Audio_GetClockInfo(&info) != HAL_OK)
  {
    return HAL_ERROR;
  }

  memset(&audio_beep, 0, sizeof(audio_beep));
  audio_beep.sample_rate = info.sample_rate;
  audio_beep.tone_frames = info.sample_rate / 4U;
  audio_beep.gap_frames = info.sample_rate / 10U;
  audio_beep.fade_frames = info.sample_rate / 200U;
  if (audio_beep.fade_frames == 0U)
  {
    audio_beep.fade_frames = 1U;
  }
  audio_beep.phase_step = Audio_BeepPhaseStep(audio_beep_frequencies[0]);

  uint32_t total_ms = (AUDIO_BEEP_TONE_COUNT *
                       (audio_beep.tone_frames + audio_beep.gap_frames) * 1000U) /
                      info.sample_rate;

  return Audio_Play(Audio_BeepFillHalf, total_ms + 500U);
}

/* --------------------------------------------------------------- PCM file */

static FIL audio_file;
static uint8_t audio_file_exhausted;

static uint8_t Audio_FileFillHalf(uint16_t *half)
{
  UINT read_bytes = 0U;

  if (audio_file_exhausted)
  {
    memset(half, 0, AUDIO_HALF_BYTES);
    return 0U;
  }

  if (f_read(&audio_file, half, AUDIO_HALF_BYTES, &read_bytes) != FR_OK)
  {
    read_bytes = 0U;
  }

  if (read_bytes < AUDIO_HALF_BYTES)
  {
    memset((uint8_t *)half + read_bytes, 0, AUDIO_HALF_BYTES - read_bytes);
    audio_file_exhausted = 1U;
  }

  /* A trailing partial frame is padded away rather than played half-formed. */
  return (read_bytes >= (2U * sizeof(uint16_t))) ? 1U : 0U;
}

HAL_StatusTypeDef Audio_PlayPcmFile(const char *path)
{
  Audio_ClockInfo info;

  if (path == NULL)
  {
    return HAL_ERROR;
  }
  if (Audio_GetClockInfo(&info) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (f_open(&audio_file, path, FA_READ) != FR_OK)
  {
    return HAL_ERROR;
  }

  uint32_t frames = (uint32_t)(f_size(&audio_file) / (2U * sizeof(uint16_t)));
  uint32_t duration_ms = (uint32_t)(((uint64_t)frames * 1000U) / info.sample_rate);

  audio_file_exhausted = 0U;
  HAL_StatusTypeDef status = Audio_Play(Audio_FileFillHalf, duration_ms + 2000U);

  (void)f_close(&audio_file);

  return status;
}

/* --------------------------------------------------------- GIMA ADPCM file */

#define AUDIO_ADPCM_IO_BUFFER_SIZE 512U

static IMA_ADPCM_StreamInfo audio_adpcm_info;
static IMA_ADPCM_State audio_adpcm_state[2];
static uint8_t audio_adpcm_io_buffer[AUDIO_ADPCM_IO_BUFFER_SIZE];
static uint32_t audio_adpcm_frames_remaining;
static uint32_t audio_adpcm_data_remaining;
static UINT audio_adpcm_io_size;
static UINT audio_adpcm_io_position;
static uint8_t audio_adpcm_first_frame;
static uint8_t audio_adpcm_have_high_nibble;
static uint8_t audio_adpcm_nibble_byte;
static uint8_t audio_adpcm_failed;

static uint8_t Audio_AdpcmReadByte(uint8_t *value)
{
  if ((value == NULL) || (audio_adpcm_data_remaining == 0U))
  {
    return 0U;
  }

  if (audio_adpcm_io_position >= audio_adpcm_io_size)
  {
    UINT requested = AUDIO_ADPCM_IO_BUFFER_SIZE;
    if (audio_adpcm_data_remaining < requested)
    {
      requested = (UINT)audio_adpcm_data_remaining;
    }

    audio_adpcm_io_size = 0U;
    audio_adpcm_io_position = 0U;
    if ((f_read(&audio_file, audio_adpcm_io_buffer, requested,
                &audio_adpcm_io_size) != FR_OK) ||
        (audio_adpcm_io_size == 0U))
    {
      audio_adpcm_failed = 1U;
      return 0U;
    }
  }

  *value = audio_adpcm_io_buffer[audio_adpcm_io_position++];
  audio_adpcm_data_remaining--;
  return 1U;
}

static uint8_t Audio_AdpcmReadNibble(uint8_t *nibble)
{
  if (nibble == NULL)
  {
    return 0U;
  }

  if (!audio_adpcm_have_high_nibble)
  {
    if (!Audio_AdpcmReadByte(&audio_adpcm_nibble_byte))
    {
      return 0U;
    }
    *nibble = audio_adpcm_nibble_byte & 0x0FU;
    audio_adpcm_have_high_nibble = 1U;
  }
  else
  {
    *nibble = audio_adpcm_nibble_byte >> 4;
    audio_adpcm_have_high_nibble = 0U;
  }

  return 1U;
}

static uint8_t Audio_AdpcmNextFrame(int16_t *left, int16_t *right)
{
  if ((left == NULL) || (right == NULL) ||
      (audio_adpcm_frames_remaining == 0U))
  {
    return 0U;
  }

  if (audio_adpcm_first_frame)
  {
    *left = audio_adpcm_info.initial_predictor[0];
    *right = (audio_adpcm_info.channels == 2U) ?
        audio_adpcm_info.initial_predictor[1] : *left;
    audio_adpcm_first_frame = 0U;
  }
  else
  {
    uint8_t nibble;
    if (!Audio_AdpcmReadNibble(&nibble))
    {
      return 0U;
    }
    *left = IMA_ADPCM_DecodeNibble(&audio_adpcm_state[0], nibble);

    if (audio_adpcm_info.channels == 2U)
    {
      if (!Audio_AdpcmReadNibble(&nibble))
      {
        return 0U;
      }
      *right = IMA_ADPCM_DecodeNibble(&audio_adpcm_state[1], nibble);
    }
    else
    {
      *right = *left;
    }
  }

  audio_adpcm_frames_remaining--;
  return 1U;
}

static uint8_t Audio_AdpcmFillHalf(uint16_t *half)
{
  uint32_t produced_frames = 0U;
  memset(half, 0, AUDIO_HALF_BYTES);

  while (produced_frames < AUDIO_HALF_FRAMES)
  {
    int16_t left;
    int16_t right;
    if (!Audio_AdpcmNextFrame(&left, &right))
    {
      break;
    }

    half[produced_frames * 2U] = (uint16_t)left;
    half[(produced_frames * 2U) + 1U] = (uint16_t)right;
    produced_frames++;
  }

  return (produced_frames != 0U) ? 1U : 0U;
}

HAL_StatusTypeDef Audio_PlayImaAdpcmFile(const char *path)
{
  Audio_ClockInfo clock_info;
  uint8_t header[IMA_ADPCM_GIMA_HEADER_SIZE];
  UINT header_bytes = 0U;

  if ((path == NULL) || (Audio_GetClockInfo(&clock_info) != HAL_OK))
  {
    return HAL_ERROR;
  }
  if (f_open(&audio_file, path, FA_READ) != FR_OK)
  {
    return HAL_ERROR;
  }

  HAL_StatusTypeDef status = HAL_ERROR;
  if ((f_read(&audio_file, header, sizeof(header), &header_bytes) != FR_OK) ||
      (header_bytes != sizeof(header)) ||
      (IMA_ADPCM_ParseGimaHeader(header, sizeof(header),
                                 &audio_adpcm_info) != 0))
  {
    goto close_file;
  }

  uint32_t rate_difference =
      (clock_info.sample_rate > audio_adpcm_info.sample_rate) ?
      (clock_info.sample_rate - audio_adpcm_info.sample_rate) :
      (audio_adpcm_info.sample_rate - clock_info.sample_rate);
  if ((rate_difference > (audio_adpcm_info.sample_rate / 50U)) ||
      ((uint64_t)IMA_ADPCM_GIMA_HEADER_SIZE + audio_adpcm_info.data_bytes >
       (uint64_t)f_size(&audio_file)))
  {
    goto close_file;
  }

  IMA_ADPCM_StateInit(&audio_adpcm_state[0],
                      audio_adpcm_info.initial_predictor[0],
                      audio_adpcm_info.initial_step_index[0]);
  IMA_ADPCM_StateInit(&audio_adpcm_state[1],
                      audio_adpcm_info.initial_predictor[1],
                      audio_adpcm_info.initial_step_index[1]);
  audio_adpcm_frames_remaining = audio_adpcm_info.frame_count;
  audio_adpcm_data_remaining = audio_adpcm_info.data_bytes;
  audio_adpcm_io_size = 0U;
  audio_adpcm_io_position = 0U;
  audio_adpcm_first_frame = 1U;
  audio_adpcm_have_high_nibble = 0U;
  audio_adpcm_failed = 0U;

  uint32_t duration_ms = (uint32_t)(
      ((uint64_t)audio_adpcm_info.frame_count * 1000U) /
      audio_adpcm_info.sample_rate);
  status = Audio_Play(Audio_AdpcmFillHalf, duration_ms + 2000U);
  if (audio_adpcm_failed)
  {
    status = HAL_ERROR;
  }

close_file:
  (void)f_close(&audio_file);
  return status;
}
