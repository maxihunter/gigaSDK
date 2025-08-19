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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

//#define DEGUB

static void print_help(char * name) {
    printf("%s - tool to compress data by RLE algorithm by MaxiHunter\n", name);
    printf("Usage:\n");
    printf("%s <type> <input> <output> <threshold>\n", name);
    printf("\t <type> - \"b8\" or \"b16\" or \"p8\" or \"p16\"\n");
    printf("\t\t Where: b - is a binary format, p - printable format\n");
    printf("\t <input> - input file name\n");
    printf("\t <output> - output file name\n");
    printf("\t <threshold> - skipping threshold of repeating data to the head index\n");
}


void main(int argc, char ** argv) {

    if (argc < 5) {
        print_help(argv[0]);
        return;
    }
    // TODO change to vargs parsing; currently just for testing
    if ( strcmp(argv[1], "b8") != 0 &&
         strcmp(argv[1], "b16") != 0 &&
         strcmp(argv[1], "p8") != 0 &&
         strcmp(argv[1], "p16") != 0 ) {
        printf("Error: Invalid compress data type\n");
        return;
    }
    const char *input = argv[2];
    const char *output = argv[3];
    unsigned int thres = atoi(argv[4]);
    // size of encoded block in bytes
    unsigned int block_size = atoi(argv[1]+1) / 8;

    FILE *fp = fopen(input, "r");
    if (fp == NULL) {
        printf("File open failed: %s\n", strerror(errno));
        return;
    }
    fseek(fp, 0, SEEK_END);
    unsigned int size = ftell(fp);

    char * in_data = (char *) malloc(sizeof(char) * size);
    if (in_data == NULL) {
        printf("Memory allocation failed: %s\n", strerror(errno));
        return;
    }
    fseek(fp, 0, SEEK_SET);

    fread(in_data, 1, size, fp);
    fclose(fp);

#ifdef DEBUG
    printf("data = %s\n", in_data);
#endif
    char *compressed = (char *) malloc(size * sizeof(char) * 3);
    if (compressed == NULL) {
        printf("Memory allocation failed: %s\n", strerror(errno));
        return;
    }
    char head[MAX_HEADER_LEN*4] = {0};
    uint32_t comp_size = 0;
    //printf("Input data size = %d\n", size);

    // Compress the input string via RLE algorithm,
    if ( strcmp(argv[1]+1, "8") == 0 )
        minirle_compress(in_data, size, compressed, &comp_size, head, thres);
    else {
        // because in_data is a 8-bit char, and we want to encode as 16-bit, split size by 2
        minirle_compress16((uint16_t *)in_data, size / 2, (uint16_t *)compressed, &comp_size, (uint32_t*) head, thres);
    }

    // big-endian, WTF ??????
    unsigned int header_size = (((*head+3) << 24 ) & 0x00000000) | ((*(head+2) << 16) & 0xff0000) | ((*(head+1) << 8) & 0xff00) | ((*head) & 0xff);
    unsigned char * data = (char *) malloc (sizeof(char) * (size+(header_size*4)));
    if (data == NULL) {
        printf("Memory allocation failed: %s\n", strerror(errno));
        return;
    }
    char * decomp = (char *) malloc(size * sizeof(char));
    memset(decomp, 0, size * sizeof(char));
    memcpy((unsigned int*)data, head, 4096);
    printf("%d-bit commpression summary:\n", 8 *block_size);
    printf("\tOriginal input data size: %u bytes\n", size);
    printf("\tCompressed data size: %u bytes (including headers: %u bytes)\n", (comp_size*block_size) + header_size*4, header_size*4);
    printf("\tHeader lines: %u records\n", header_size+1);
    if ( strcmp(argv[1]+1, "8") == 0 ) {
        printf("8-bit Compressed(%u/%u) Headersize(%x) (%x %x %x %x): \n", size, comp_size, header_size, head[0], head[1], head[2], head[3]);
        memcpy(data+((header_size+1)*4), compressed, comp_size);
    } else {
        printf("16-bit Compressed(%u/%u) Headersize(%x) (%x %x %x %x): \n", size, comp_size*2, header_size, head[0], head[1], head[2], head[3]);
        memcpy(data+((header_size+1)*4), compressed, comp_size*2);
    }


    fp = fopen(output, "w");
    if (fp == NULL) {
        printf("File open failed: %s\n", strerror(errno));
        return;
    }
    if ( *(argv[1]) == 'b') {
        fwrite(data, 1, comp_size+(header_size*4)+4, fp);
    } else {
        char buff[256] = {0};
        char buff2[32] = {0};
        if ( strcmp(argv[1]+1, "8") == 0 ) {
            for(unsigned int i = 0; i < (header_size*4) + comp_size+4; i++) {
                sprintf(buff2, "0x%02X,", data[i]);
                strncat(buff, buff2, 10);
                if ( (i + 1) % 16 == 0 && i != 0 ) {
                    fputs(buff, fp);
                    fputc('\n', fp);
                    buff[0] = '\0';
                }
            }
        } else {
            for(unsigned int i = 0; i < (header_size*4) + comp_size+4; i+=2) {
                sprintf(buff2, "0x%02X%02X,", data[i+1], data[i]);
                strncat(buff, buff2, 10);
                if ( (i/2) % 15 == 0 && i > 5 ) {
                    fputs(buff, fp);
                    fputc('\n', fp);
                    buff[0] = '\0';
                }
            }
        }
        if (strlen(buff))
            fputs(buff, fp);
    }
    fclose(fp);
#ifdef DEBUG
    printf("comp: %s\n", compressed);
#endif
    // Decompress the compressed output to verify that all went good
    if (decomp == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }
    memset(decomp, 0, size * sizeof(char));
    if ( strcmp(argv[1]+1, "8") == 0 )
        minirle_decompress(data, comp_size, decomp);
    else
        minirle_decompress16((uint16_t *)data, comp_size, (uint16_t *)decomp);

    for (unsigned int i = 0; i < size; i++) {
        if (*(in_data+i) != *(decomp+i)) {
            printf("Data verification failed! (index=%d, val1!=,val2 [0x%02X,0x%02X])\n", i, *(in_data+i), *(decomp+i));
            break;
        }
    }
#ifdef DEBUG
    printf("Decompressed: %s\n", decomp);
#endif
    free(decomp);
    free(data);
    free(compressed);
    free(in_data);

}

