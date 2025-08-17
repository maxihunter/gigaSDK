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

#define SPLIT_16_TO_8(x) (x & 0xff00) >> 8, x & 0xff

    
void minirle_compress(const char *input, uint32_t data_size, char *compressed, uint32_t *comp_size, char * header, uint32_t threshold) {
    int prev_ch = input[0], idx = 1;
    int ch_count = 1;
    unsigned int header_id[MAX_HEADER_LEN] = {0};
    unsigned int h_count = 0;
    //printf("data = %s\n", input);

    while(idx < data_size) {
        unsigned char curr_ch = input[idx++];
        int block = 0;

        while( prev_ch == curr_ch && ch_count < 255 && idx+block < data_size) {
            printf("Encode:id:%d block(%d)(total:%d) prev[%X] curr[%X] count[%d] hcount[%d]\n", idx, block, idx+block, prev_ch, curr_ch, ch_count, h_count);
            curr_ch = input[idx+(block++)];
            ch_count++;
        }
        if (ch_count > threshold && h_count < MAX_HEADER_LEN) {
            header_id[h_count] = idx - 1;
            h_count++;
            compressed[(*comp_size)++] = ch_count;
            ch_count = 1;
        }
        while (ch_count > 0) {
            compressed[(*comp_size)++] = prev_ch;
            ch_count--;
            idx++;
        }
        ch_count = 1;
        prev_ch = curr_ch;
    }
    compressed[(*comp_size)++] = prev_ch;
    *(((unsigned int *)header)) = h_count;
    for (int i = 0; i < h_count; i++) {
        *(((unsigned int *)header) + i + 1) = header_id[i];
    }
}

void minirle_decompress(const char *compressed, uint32_t comp_size, char *output) {
    int idx = 0, out_idx = 0;
    unsigned int header_id[MAX_HEADER_LEN] = {0};
    unsigned int headers = *((unsigned int *)compressed);

    //printf("WHAT HEADER SIZE = %d comp_size = %d \n", headers, comp_size);
    for (int i = 0; i < headers; i++) {
        //printf("[0x%02X]", compressed[i]);
        header_id[i] = *(((unsigned int *)compressed)+i+1);
    }
    //printf("\n");
    const char * new_p = compressed + (sizeof(unsigned int) * (headers+1));
    int header_pos = 0;

    while(idx < comp_size) {
        unsigned char code = 1;
        if (out_idx == header_id[header_pos] - 1  ) {
            code = new_p[idx++];
            //printf("POSS FOUND AT %d(%d)==%d [next:%d]  (%x:%c) count = %d\n", idx, out_idx, header_id[header_pos], header_id[header_pos+1], new_p[idx], new_p[idx+1], code);
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
}

void minirle_compress16(const uint16_t *input, uint32_t data_size, uint16_t *compressed, uint32_t *comp_size, uint32_t * header, uint32_t threshold) {
    uint16_t prev_ch = input[0], idx = 1;
    uint32_t header_id[MAX_HEADER_LEN] = {0};
    uint32_t h_count = 0;
    *comp_size = 0;
    //printf("data = %s\n", input);

    while(idx < data_size) {
        uint16_t ch_count = 1;
        uint16_t curr_ch = input[idx];

        //printf("Encode:id:%d comp_id(%d) prev[%04X:%d] curr[%04X:%d]\n", idx, *comp_size, prev_ch, prev_ch, curr_ch, curr_ch);
        while( prev_ch == curr_ch && ch_count < 65535 && idx+ch_count < data_size) {
            curr_ch = input[idx+ch_count];
            ch_count++;
        }
        if (ch_count > threshold && h_count < MAX_HEADER_LEN) {
            //printf("Encode:id:%d block(%d)(jump_pos:%d) prev[%04X] curr[%04X] count[%d] hcount[%d]\n", idx, ch_count, idx+ch_count-1, prev_ch, curr_ch, ch_count, h_count);
            header_id[h_count] = idx - 1;
            h_count++;
            compressed[(*comp_size)++] = ch_count;
            idx += ch_count - 1;
            ch_count = 1;
        }
        while (ch_count > 0) {
            compressed[(*comp_size)++] = prev_ch;
            ch_count--;
            idx++;
        }
        prev_ch = curr_ch;
    }
    compressed[(*comp_size)++] = prev_ch;
    *(header) = h_count;
    for (int i = 0; i < h_count; i++) {
        *(header + i + 1) = header_id[i];
    }
}

void minirle_decompress16(const uint16_t *compressed, uint32_t comp_size, uint16_t *output) {
    uint32_t idx = 0, out_idx = 0;
    uint16_t header_id[MAX_HEADER_LEN] = {0};
    uint16_t headers = *((uint32_t *)compressed);

    //printf("WHAT HEADER SIZE = %d comp_size = %d \n", headers, comp_size);
    for (int i = 0; i < headers; i++) {
        header_id[i] = *(((uint32_t *)compressed)+i+1);
        /*printf("%d:[0x%08X]", i, ((uint32_t*)compressed)[i+1]);
        if (i%10 == 0) {
            printf("\n");
        }*/
    }
    printf("\n");
    const uint16_t * new_p = (uint16_t *)((uint32_t *)compressed + headers + 1);
    uint32_t header_pos = 0;

    while(idx < comp_size) {
        uint16_t code = 1;
        if (out_idx == header_id[header_pos] ) {
            code = new_p[idx++];
            //printf("POSS FOUND AT %d(%d)==%d [next:%d]  (%04x:%d) count = %d\n", idx, out_idx, header_id[header_pos], header_id[header_pos+1], new_p[idx], new_p[idx], code);
            header_pos++;
        }
        uint16_t ch = new_p[idx];
        //printf("Decode: AT %d(%d)==(%04x:%d) count = %d\n", idx, out_idx, new_p[idx], new_p[idx], code);

        while ( code > 0 ) {
            output[out_idx++] = ch;
            code--;
        }
        idx++;
    }
}

