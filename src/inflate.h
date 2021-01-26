/*
 * zlib-deflate-nostdlib
 *
 * Copyright 2021 Daniel Friesel
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>

#define DEFLATE_ERR_INPUT_LENGTH (-1)
#define DEFLATE_ERR_METHOD (-2)
#define DEFLATE_ERR_FDICT (-3)
#define DEFLATE_ERR_BLOCK (-4)
#define DEFLATE_ERR_CHECKSUM (-5)
#define DEFLATE_ERR_OUTPUT_LENGTH (-6)
#define DEFLATE_ERR_FCHECK (-7)
#define DEFLATE_ERR_NLEN (-8)
#define DEFLATE_ERR_HUFFMAN (-9)

int16_t inflate(unsigned char *input_buf, uint16_t input_len,
		unsigned char *output_buf, uint16_t output_len);
int16_t inflate_zlib(unsigned char *input_buf, uint16_t input_len,
		     unsigned char *output_buf, uint16_t output_len);
