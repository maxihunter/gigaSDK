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

static void print_help(char * name) {
    printf("%s - tool to compress data by RLE algorithm\n", name);
    printf("Usage:\n");
    printf("%s <type> <input> <output>\n", name);
    printf("\t <type> - \"s8\" or \"s16\"\n");
    printf("\t <input> - input file name\n");
    printf("\t <output> - output file name\n");
}


void main(int argc, char ** argv) {

    if (argc < 4) {
        print_help(argv[0]);
        return;
    }
    const char *input = argv[2];
    const char *output = argv[3];
    printf("input: %s\n", input);
    // Declare compression dictionary size
    // and buffer size.

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

    printf("data = %s\n", in_data);
    char compressed[65535] = {0};
    char head[4096] = {0};
    size_t comp_size = 0;
    size_t header_size = 0;
    printf("Input data size = %d\n", size);

    // Compress the input string via RLE algorithm,
    // then pass the output to th `compressed`
    // variable.
    if ( strcmp(argv[1], "s8") == 0 )
        minirle_compress(in_data, size, compressed, &comp_size, head);
    else
        minirle_compress16(in_data, size, compressed, &comp_size, head);


    // Iterate to print all the compression output
    // integer values.
    /*for(unsigned int i = 0; i < comp_size; ++i) {
        printf("[0x%02X]", compressed[i]);
//        printf("0x%02X(%c), ", (compressed[i] & 0xff), (compressed[i] & 0xff) );
//        printf("i=%d, div10=%d \n", i, i % 10);
        if ( i % 10 == 0 && i != 0 ) {
            printf("\n");\
        }
    }*/
    printf("\n");
    header_size = *head;
    char * data = (char *) malloc (sizeof(char) * (size+(header_size*4)));
    if (data == NULL) {
        printf("Memory allocation failed: %s\n", strerror(errno));
        return;
    }
    printf("Compressed(%d/%d) Headersize(%u): \n", size, comp_size, header_size);
    //*((unsigned int *)data) = header_size;
    char * decomp = (char *) malloc(size * sizeof(char));
    memset(decomp, 0, size * sizeof(char));
    memcpy((unsigned int*)data, head, 4096);
    memcpy(data+((header_size+1)*4), compressed, comp_size);

    fp = fopen(output, "w");
    if (fp == NULL) {
        printf("File open failed: %s\n", strerror(errno));
        return;
    }
    fwrite(data, 1, comp_size+(header_size*4), fp);
    fclose(fp);
    for(unsigned int i = 0; i < (header_size*4) + comp_size; ++i) {
        printf("[0x%02X]", data[i]);
        //printf("0x%02X(%c), ", (compressed[i] & 0xff), (compressed[i] & 0xff) );
        if ( i % 16 == 0 && i != 0 ) {
            printf("\n%d: ", i / 16);
        }
    }
    printf("\n");\
    printf("comp: %s\n", compressed);
    // Decompress the compressed output
    // then pass the decompressed string
    // to the variable `decompressed`.
    if (decomp == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }
    memset(decomp, 0, size * sizeof(char));
    if ( strcmp(argv[1], "s8") == 0 )
        minirle_decompress(data, comp_size, decomp);
    else
        minirle_decompress16(data, comp_size, decomp);

    for (unsigned int i = 0; i < size; i++) {
        if (*(in_data+i) != *(decomp+i)) {
            printf("Data verification failed! (index=%d, val1,val2 [0x%02X,0x%02X])\n", i, *(in_data+i), *(decomp+i));
            break;
        }
    }
    printf("Decompressed: %s\n", decomp);
    free(decomp);
    free(data);
    free(in_data);

}

