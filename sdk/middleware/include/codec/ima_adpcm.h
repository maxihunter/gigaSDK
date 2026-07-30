#ifndef IMA_ADPCM_H
#define IMA_ADPCM_H

#include <stddef.h>
#include <stdint.h>

#define IMA_ADPCM_GIMA_HEADER_SIZE 32U
#define IMA_ADPCM_GIMA_VERSION     1U

typedef struct
{
  int32_t predictor;
  int32_t step_index;
} IMA_ADPCM_State;

typedef struct
{
  uint32_t sample_rate;
  uint32_t frame_count;
  uint32_t data_bytes;
  uint8_t channels;
  int16_t initial_predictor[2];
  uint8_t initial_step_index[2];
} IMA_ADPCM_StreamInfo;

void IMA_ADPCM_StateInit(IMA_ADPCM_State *state, int16_t predictor,
                         uint8_t step_index);
int16_t IMA_ADPCM_DecodeNibble(IMA_ADPCM_State *state, uint8_t nibble);
uint8_t IMA_ADPCM_EncodeSample(IMA_ADPCM_State *state, int16_t sample);

/* Parses the little-endian GIMA v1 header produced by audio-to-adpcm.py. */
int IMA_ADPCM_ParseGimaHeader(const uint8_t *header, size_t header_size,
                             IMA_ADPCM_StreamInfo *info);

#endif
