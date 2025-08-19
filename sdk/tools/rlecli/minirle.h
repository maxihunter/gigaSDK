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
#include <stdint.h>

#define MAX_HEADER_LEN 1024

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
 * @param header A pointer to a variable that will store the header with necessary decompression data.
 * @param threshold A variable that will determine a minimum identical byte sequence to be encoded. Please 
 *          pay attention that maximum header size is limited by MAX_HEADER_LEN, if input data len is too large
 *          header would be exceeded during encoding process. To avoid it you can try different threshold and see what
 *          result will be. 0 =< threshold < MAX_HEADER_LEN
 * 
 */
void minirle_compress(
    const char *input,
    uint32_t data_size,
    char *compressed,
    uint32_t *comp_size,
    char *header,
    uint32_t threshold
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
    const char *compressed,
    uint32_t comp_size,
    char *output
);

/**
 * 
 * @brief Compresses data using 16-bit word Mini RLE algorithm.
 *
 * @details This function takes an input string and compresses it using the Micro Run-Length encoding.
 * The compressed output, compressed size are returned through the parameters.
 *
 * @param input The input data to be compressed. Encoding by 16-bit of input data.
 * @param data_size The input data length to be compressed.
 * @param compressed A pointer to an array that will store the compressed data.
 * @param comp_size A pointer to a variable that will store the size of the compressed data.
 * @param header A pointer to a variable that will store the header with necessary decompression data.
 * @param threshold A variable that will determine a minimum identical byte sequence to be encoded. Please 
 *          pay attention that maximum header size is limited by MAX_HEADER_LEN, if input data len is too large
 *          header would be exceeded during encoding process. To avoid it you can try different threshold and see what
 *          result will be. 0 =< threshold < MAX_HEADER_LEN
 * 
 */
void minirle_compress16(
    const uint16_t *input,
    uint32_t data_size,
    uint16_t *compressed,
    uint32_t *comp_size,
    uint32_t *header,
    uint32_t threshold
);

/**
 * 
 * @brief Decompresses data using 16-bit word that was compressed using Mini RLE algorithm.
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
void minirle_decompress16(
    const uint16_t *compressed,
    uint32_t comp_size,
    uint16_t *output
);

#endif
