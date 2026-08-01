#include "codec/ima_adpcm.h"

#include <string.h>

static const int16_t ima_step_table[89] = {
       7,     8,     9,    10,    11,    12,    13,    14,
      16,    17,    19,    21,    23,    25,    28,    31,
      34,    37,    41,    45,    50,    55,    60,    66,
      73,    80,    88,    97,   107,   118,   130,   143,
     157,   173,   190,   209,   230,   253,   279,   307,
     337,   371,   408,   449,   494,   544,   598,   658,
     724,   796,   876,   963,  1060,  1166,  1282,  1411,
    1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
    3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
    7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
   15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
   32767
};

static const int8_t ima_index_table[16] = {
  -1, -1, -1, -1, 2, 4, 6, 8,
  -1, -1, -1, -1, 2, 4, 6, 8
};

static int32_t IMA_ADPCM_Clamp(int32_t value, int32_t low, int32_t high)
{
  if (value < low)
  {
    return low;
  }
  if (value > high)
  {
    return high;
  }
  return value;
}

static uint16_t IMA_ADPCM_ReadU16(const uint8_t *value)
{
  return (uint16_t)value[0] | ((uint16_t)value[1] << 8);
}

static uint32_t IMA_ADPCM_ReadU32(const uint8_t *value)
{
  return (uint32_t)value[0] |
         ((uint32_t)value[1] << 8) |
         ((uint32_t)value[2] << 16) |
         ((uint32_t)value[3] << 24);
}

void IMA_ADPCM_StateInit(IMA_ADPCM_State *state, int16_t predictor,
                         uint8_t step_index)
{
  if (state == NULL)
  {
    return;
  }

  state->predictor = predictor;
  state->step_index = IMA_ADPCM_Clamp(step_index, 0, 88);
}

int16_t IMA_ADPCM_DecodeNibble(IMA_ADPCM_State *state, uint8_t nibble)
{
  if (state == NULL)
  {
    return 0;
  }

  nibble &= 0x0FU;
  int32_t step = ima_step_table[state->step_index];
  int32_t difference = step >> 3;

  if ((nibble & 1U) != 0U) difference += step >> 2;
  if ((nibble & 2U) != 0U) difference += step >> 1;
  if ((nibble & 4U) != 0U) difference += step;

  if ((nibble & 8U) != 0U)
  {
    state->predictor -= difference;
  }
  else
  {
    state->predictor += difference;
  }

  state->predictor = IMA_ADPCM_Clamp(state->predictor, -32768, 32767);
  state->step_index = IMA_ADPCM_Clamp(
      state->step_index + ima_index_table[nibble], 0, 88);

  return (int16_t)state->predictor;
}

uint8_t IMA_ADPCM_EncodeSample(IMA_ADPCM_State *state, int16_t sample)
{
  if (state == NULL)
  {
    return 0U;
  }

  int32_t step = ima_step_table[state->step_index];
  int32_t difference = (int32_t)sample - state->predictor;
  uint8_t nibble = 0U;

  if (difference < 0)
  {
    nibble = 8U;
    difference = -difference;
  }

  int32_t reconstructed = step >> 3;
  if (difference >= step)
  {
    nibble |= 4U;
    difference -= step;
    reconstructed += step;
  }
  step >>= 1;
  if (difference >= step)
  {
    nibble |= 2U;
    difference -= step;
    reconstructed += step;
  }
  step >>= 1;
  if (difference >= step)
  {
    nibble |= 1U;
    reconstructed += step;
  }

  if ((nibble & 8U) != 0U)
  {
    state->predictor -= reconstructed;
  }
  else
  {
    state->predictor += reconstructed;
  }

  state->predictor = IMA_ADPCM_Clamp(state->predictor, -32768, 32767);
  state->step_index = IMA_ADPCM_Clamp(
      state->step_index + ima_index_table[nibble], 0, 88);

  return nibble;
}

int IMA_ADPCM_ParseGimaHeader(const uint8_t *header, size_t header_size,
                             IMA_ADPCM_StreamInfo *info)
{
  if ((header == NULL) || (info == NULL) ||
      (header_size < IMA_ADPCM_GIMA_HEADER_SIZE))
  {
    return -1;
  }
  if ((memcmp(header, "GIMA", 4U) != 0) ||
      (header[4] != IMA_ADPCM_GIMA_VERSION) ||
      (IMA_ADPCM_ReadU16(&header[6]) != IMA_ADPCM_GIMA_HEADER_SIZE))
  {
    return -1;
  }
  if ((header[5] == 0U) || (header[5] > 2U))
  {
    return -1;
  }

  memset(info, 0, sizeof(*info));
  info->channels = header[5];
  info->sample_rate = IMA_ADPCM_ReadU32(&header[8]);
  info->frame_count = IMA_ADPCM_ReadU32(&header[12]);
  info->initial_predictor[0] = (int16_t)IMA_ADPCM_ReadU16(&header[16]);
  info->initial_step_index[0] = header[18];
  info->initial_predictor[1] = (int16_t)IMA_ADPCM_ReadU16(&header[20]);
  info->initial_step_index[1] = header[22];
  info->data_bytes = IMA_ADPCM_ReadU32(&header[24]);

  if ((info->sample_rate == 0U) || (info->frame_count == 0U) ||
      (info->initial_step_index[0] > 88U) ||
      ((info->channels == 2U) && (info->initial_step_index[1] > 88U)))
  {
    return -1;
  }

  uint64_t encoded_nibbles =
      (uint64_t)(info->frame_count - 1U) * info->channels;
  uint32_t required_bytes = (uint32_t)((encoded_nibbles + 1U) / 2U);
  return (info->data_bytes >= required_bytes) ? 0 : -1;
}
