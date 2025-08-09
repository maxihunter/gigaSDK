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

#define MAX_HEADER_LEN 512
void minirle_compress(char *input, size_t data_size, char *compressed, size_t *comp_size, char * header, size_t *header_size) {
    int prev_ch = input[0], idx = 1;
    int ch_count = 1;
    unsigned int header_id[MAX_HEADER_LEN] = {0};
    unsigned int h_count = 0;

    while(idx < data_size) {
        unsigned char curr_ch = input[idx++];
        int block = 0;

        while( prev_ch == curr_ch && ch_count < 255) {
            //printf("Encode:%d prev[%c] curr[%c] count[%d]\n", idx, prev_ch, curr_ch, ch_count);
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
    *(((unsigned int *)header)) = h_count;
    for (int i = 0; i < h_count; i++) {
        *(((unsigned int *)header) + i + 1) = header_id[i];
    }
    *header_size = h_count;
}

void minirle_decompress(char *compressed, size_t comp_size, char *output) {
    int idx = 0, out_idx = 0;
    unsigned int header_id[MAX_HEADER_LEN] = {0};
    unsigned int headers = *((unsigned int *)compressed);

    printf("WHAT HEADER SIZE = %d comp_size = %d \n", headers, comp_size);
    for (int i = 0; i < headers; i++) {
        printf("[0x%02X]", compressed[i]);
        header_id[i] = *(((unsigned int *)compressed)+i+1);
    }
    printf("\n");
    char * new_p = compressed + (sizeof(unsigned int) * (headers+1));
    int header_pos = 0;

    while(idx < comp_size) {
        unsigned char code = 1;
        if (out_idx == header_id[header_pos] - 1  ) {
            code = new_p[idx++];
            printf("POSS FOUND AT %d(%d)==%d [next:%d]  (%x:%c) count = %d\n", idx, out_idx, header_id[header_pos], header_id[header_pos+1], new_p[idx], new_p[idx+1], code);
            header_pos++;
        }
        printf("out_idx= %d(idx: %d)\n", out_idx, idx);
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

