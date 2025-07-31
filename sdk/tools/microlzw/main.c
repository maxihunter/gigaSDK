#include "microlzw.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
//#include "bootup1_rgb565.h"

void main() {

    //const char *input = bootup1;
    const char *input = "bootup1 Heeeeeeeeww boot up need to be advanced for produuuuuuuuuuuuusing nnnnneeeeeeeeeeeeeeeeeeeewww items. So we can have this very very soon";
    // Declare compression dictionary size
    // and buffer size.
    const int dict_size = 4096, buffer_size = 256;

    int compressed[dict_size];
    size_t comp_size = 0;

    // Compress the input string via LZW algorithm,
    // then pass the output to th `compressed`
    // variable.
    mlzw_compress(input, compressed, &comp_size, dict_size);

    printf("Compressed(%d/%d): \n", 0/*sizeof(bootup1)*/, comp_size);
    // Iterate to print all the compression output
    // integer values.
    for(unsigned int i = 0; i < comp_size; ++i) {
        printf("0x%02X, ", (compressed[i] & 0xff00) >> 8);
        printf("0x%02X, ", (compressed[i] & 0xff));
//        printf("i=%d, div10=%d \n", i, i % 10);
        if ( i % 10 == 0 && i != 0 ) {
            printf("\n");\
        }
    }
    printf("\n");

    // Decompress the compressed output
    // then pass the decompressed string
    // to the variable `decompressed`.
    char * decomp = (char *) malloc(sizeof(bootup1) * sizeof(char));
    if (decomp == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }
    mlzw_decompress(compressed, comp_size, decomp, dict_size);

/*    for (unsigned int i = 0; i < sizeof(bootup1); i++) {
        if (*(input+i) != *(decomp+i)) {
            printf("Data verification failed! (index=%d, val1,val2 [0x%02X,0x%02X])\n", i, *(input+i), *(decomp+i));
    //        return;
        }
    }*/
    printf("Decompressed: %s\n", decomp);
}

