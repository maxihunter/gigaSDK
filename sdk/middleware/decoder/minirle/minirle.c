/*
 * This file is part of the gigaSDK source code.
 * Copyright (c) 2025 MaxiHunter
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "minirle.h"
#include <stdio.h>

#define MAX_HEADER_LEN 512

#define SPLIT_16_TO_8(x) (x & 0xff00) >> 8, x & 0xff

#ifdef ENABLE_COMPRESSOR
void minirle_compress(const uint8_t *input, uint32_t data_size, uint8_t *compressed, uint32_t *comp_size, uint32_t * header) {
    int prev_ch = input[0], idx = 1;
    int ch_count = 1;
    uint32_t header_id[MAX_HEADER_LEN] = {0};
    uint32_t h_count = 0;
    //printf("data = %s\n", input);

    while(idx < data_size) {
        uint8_t curr_ch = input[idx++];
        uint32_t block = 0;

        while( prev_ch == curr_ch && ch_count < 255 && idx < data_size) {
            //printf("Encode:id:%d block(%d)(total:%d) prev[%X] curr[%X] count[%d]\n", idx, block, idx+block, prev_ch, curr_ch, ch_count);
            curr_ch = input[idx+(block++)];
            ch_count++;
        }
        if (ch_count > 1) {
            header_id[h_count] = idx - 1;
            h_count++;
            compressed[(*comp_size)++] = ch_count;
        }
        compressed[(*comp_size)++] = prev_ch;
        idx += block;
        ch_count = 1;
        prev_ch = curr_ch;
    }
    compressed[(*comp_size)++] = (1 << 8) | prev_ch;
    *(header) = h_count;
    for (uint32_t i = 0; i < h_count; i++) {
        *(header + i + 1) = header_id[i];
    }
}
#endif // ENABLE_COMPRESSOR

void minirle_decompress(const uint8_t *compressed, uint32_t comp_size, uint8_t *output) {
    int idx = 0, out_idx = 0;
    uint32_t header_id[MAX_HEADER_LEN] = {0};
    uint32_t headers = *((uint32_t *)compressed);

    //printf("WHAT HEADER SIZE = %d comp_size = %d \n", headers, comp_size);
    for (int i = 0; i < headers; i++) {
        //printf("[0x%02X]", compressed[i]);
        header_id[i] = *(((uint32_t *)compressed)+i+1);
    }
    //printf("\n");
    uint8_t * new_p = compressed + (sizeof(uint32_t) * (headers+1));
    int header_pos = 0;

    while(idx < comp_size) {
        uint8_t code = 1;
        if (out_idx == header_id[header_pos] - 1  ) {
            code = new_p[idx++];
            //printf("POSS FOUND AT %d(%d)==%d [next:%d]  (%x:%c) count = %d\n\r", idx, out_idx, header_id[header_pos], header_id[header_pos+1], new_p[idx], new_p[idx+1], code);
            header_pos++;
        }
        char ch = new_p[idx];

        output[out_idx++] = ch;
        code--;
        while ( code-- > 0 ) {
            output[out_idx++] = ch;
        }
        code = 0;
        idx++;
    }
    char ch = new_p[idx];
    output[out_idx++] = ch;
    //printf("FINISHED POSS idx=%d out_idx =%d \n\r", idx, out_idx);
}

#ifdef ENABLE_COMPRESSOR
void minirle_compress16(const uint16_t *input, uint32_t data_size, uint16_t *compressed, uint32_t *comp_size, uint32_t * header) {
    unsigned int prev_ch = (input[0] << 8) | input[1] ;
    int idx = 2;
    int ch_count = 1;
    unsigned int header_id[MAX_HEADER_LEN] = {0};
    unsigned int h_count = 0;

    while(idx - 1 < data_size) {
        unsigned int curr_ch = (input[idx] << 8) | input[idx+1];

        //printf("Encode:%d prev[%c%c] curr[%c%c] count[%d]\n", idx, SPLIT_16_TO_8(prev_ch), SPLIT_16_TO_8(curr_ch), ch_count);
        while( prev_ch == curr_ch && ch_count < 65535) {
            curr_ch = (input[idx+(ch_count*2)] << 8) | input[idx+1+(ch_count*2)];
            ch_count++;
        }
        if (ch_count > 1) {
            header_id[h_count] = idx;
            h_count++;
            compressed[(*comp_size)++] = (ch_count & 0xff00) >> 8;
            compressed[(*comp_size)++] = ch_count & 0xff;
        }
        compressed[(*comp_size)++] = (prev_ch & 0xff00) >> 8;
        compressed[(*comp_size)++] = prev_ch & 0xff;
        idx += ch_count*2;
        ch_count = 1;
        prev_ch = curr_ch;
    }
    compressed[(*comp_size)++] = (1 << 8) | prev_ch;
    *(header) = h_count;
    for (int i = 0; i < h_count; i++) {
        *(header + i + 1) = header_id[i];
    }
}
#endif // ENABLE_COMPRESSOR

void minirle_decompress16(const uint16_t *compressed, uint32_t comp_size, uint16_t *output) {
    int idx = 0, out_idx = 0;
    unsigned int header_id[MAX_HEADER_LEN] = {0};
    unsigned int headers = *((unsigned int *)compressed);

    printf("WHAT HEADER SIZE = %d comp_size = %d \n\r", headers, comp_size);
    for (int i = 0; i < headers; i++) {
        //printf("[0x%02X]", compressed[i]);
        header_id[i] = *(((unsigned int *)compressed)+i+1);
    }
    //printf("\n");
    const uint16_t * new_p = compressed + ((sizeof(unsigned int)/2) * (headers+1));
    //const uint16_t * new_p = *(((unsigned int *)compressed)+headers+1);
    int header_pos = 0;

    while(idx < comp_size) {
        printf("out_idx= %d(idx: %d) nchar(0x%x:) soutput:\n\r", out_idx, idx, new_p[idx]);
        uint16_t code = 1;
        if (out_idx == header_id[header_pos] - 1  ) {
            code = new_p[idx++];
            printf("POSS FOUND AT %d(%d)==%d [next:%d]  (%x:%c) count = %d\n\r", idx, out_idx, header_id[header_pos], header_id[header_pos+1], new_p[idx], new_p[idx+1], code);
            header_pos++;
        }
        uint16_t ch = new_p[idx];
        

        output[out_idx++] = ch;
        code--;
        while ( code-- > 0 ) {
            output[out_idx++] = ch;
        }
        code = 0;
        idx++;
    }
    char ch = new_p[idx];
    output[out_idx++] = ch;
#if 0
    while(idx < comp_size) {
        uint16_t code = 1;
        if (out_idx == header_id[header_pos] - 2 ) {
            code = new_p[idx];
            idx += 2;
            printf("POSS FOUND AT %d(%d)==%d [next:%d]  (%c%c) count = %d\n", idx, out_idx, header_id[header_pos], header_id[header_pos+1], new_p[idx], new_p[idx+1], code);
            header_pos++;
        }
        char ch = new_p[idx];
        char ch2 = new_p[idx + 1];
        //printf("out_idx= %d(idx: %d) nchar(%c%c) soutput:%s\n", out_idx, idx, ch, ch2, output);
        idx += 2;

        while ( code > 0 ) {
            output[out_idx] = ch;
            output[out_idx+1] = ch2;
            out_idx += 2;
            code--;
        }
    }
#endif
}

