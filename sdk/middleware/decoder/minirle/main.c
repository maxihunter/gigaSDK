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
//#include "bootup1_rgb565.h"

void main() {

    //const char *input = bootup1;
    const char *input = "bootup1 Heeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeoooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooottttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttt                                                                                                                                      opppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppeeww boot up need to be advanced for produuuuuuuuuuuuusing nnnnneeeeeeeeeeeeeeeeeeeewww items. So we can have this very very soon";
    printf("input: %s\n", input);
    // Declare compression dictionary size
    // and buffer size.

    char compressed[4096] = {0};
    char head[4096] = {0};
    size_t comp_size = 0;
    size_t header_size = 0;

    // Compress the input string via RLE algorithm,
    // then pass the output to th `compressed`
    // variable.
    minirle_compress(input, strlen(input), compressed, &comp_size, head);

    printf("Compressed(%d/%d): \n", strlen(input)/*sizeof(bootup1)*/, comp_size );
    // Iterate to print all the compression output
    // integer values.
    for(unsigned int i = 0; i < comp_size; ++i) {
        printf("[0x%02X]", compressed[i]);
//        printf("0x%02X(%c), ", (compressed[i] & 0xff), (compressed[i] & 0xff) );
//        printf("i=%d, div10=%d \n", i, i % 10);
        if ( i % 10 == 0 && i != 0 ) {
            printf("\n");\
        }
    }
    printf("\n");
    char data[8096] = {0};
    header_size = *head;
    //*((unsigned int *)data) = header_size;
    memcpy((unsigned int*)data, head, 4096);
    memcpy((unsigned int*)data+header_size+1, compressed, 4096);

    for(unsigned int i = 0; i < header_size*4; ++i) {
        printf("[0x%02X]", data[i]);
//        printf("0x%02X(%c), ", (compressed[i] & 0xff), (compressed[i] & 0xff) );
//        printf("i=%d, div10=%d \n", i, i % 10);
        if ( i % 10 == 0 && i != 0 ) {
            printf("\n");\
        }
    }
    printf("\n");\
    // Decompress the compressed output
    // then pass the decompressed string
    // to the variable `decompressed`.
    char * decomp = (char *) malloc(strlen(input) * sizeof(char));
    memset(decomp, 0, strlen(input) * sizeof(char));
    if (decomp == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }
    minirle_decompress(data, comp_size, decomp);

    printf("DEcomp: %s\n", decomp);
    for (unsigned int i = 0; i < strlen(input); i++) {
        if (*(input+i) != *(decomp+i)) {
            printf("Data verification failed! (index=%d, val1,val2 [0x%02X,0x%02X])\n", i, *(input+i), *(decomp+i));
            return;
        }
    }
    printf("Decompressed: %s\n", decomp);
}

