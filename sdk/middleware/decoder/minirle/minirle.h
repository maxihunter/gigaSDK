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

#ifndef MINIRLE_H
#define MINIRLE_H

#include <stddef.h>

#define MAX_HEADER_LEN 512

/**
 * 
 * @brief Compresses data using the Mini RLE algorithm.
 *
 * @details This function takes an input string and compresses it using the Micro Run-Length encoding.
 * The compressed output, compressed size are returned through the parameters.
 *
 * @param input The input data to be compressed.
 * @param data_size The input data length to be compressed.
 * @param compressed A pointer to an array that will store the compressed data.
 * @param comp_size A pointer to a variable that will store the size of the compressed data.
 * 
 */
void minirle_compress(
    char *input,
    size_t data_size,
    char *compressed,
    size_t *comp_size,
    char *header,
    size_t *header_size
);

/**
 * 
 * @brief Decompresses data that was compressed using Mini RLE algorithm.
 *
 * This function takes a compressed input, decompresses it using the Mini Run-Length encoding,
 * and stores the decompressed output in the provided output buffer.
 *
 * @param compressed A pointer to an array containing the compressed data.
 * @param comp_size The size of the compressed data.
 * @param output A pointer to an array that will store the decompressed output.
 * @param output_size The size of the memory used in the decompression.
 * 
 */
void minirle_decompress(
    char *compressed,
    size_t comp_size,
    char *output
);

#endif
