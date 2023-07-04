/*
 * zlib-deflate-nostdlib
 *
 * Copyright 2021 Birte Kristina Friesel
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "inflate.h"

/*
 * The compressed (inflated) input data.
 */
const unsigned char *deflate_input_now;
const unsigned char *deflate_input_end;

/*
 * The decompressed (deflated) output stream.
 */
unsigned char *deflate_output_now;
unsigned char *deflate_output_end;

/*
 * The current bit offset in the input stream, if any.
 *
 * Deflate streams are read from least to most significant bit.
 * An offset of 1 indicates that the least significant bit is skipped
 * (i.e., only bits 7, 6, 5, 4, 3, 2, and 1 are read).
 */
uint8_t deflate_bit_offset = 0;

/*
 * Base lengths for length codes (code 257 to 285).
 * Code 257 corresponds to a copy of 3 bytes, etc.
 */
uint16_t const deflate_length_offsets[] = {
	3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
	67, 83, 99, 115, 131, 163, 195, 227, 258
};

/*
 * Extra bits for length codes (code 257 to 285).
 * Code 257 has no extra bits, code 265 has 1 extra bit
 * (and indicates a length of 11 or 12 depending on its value), etc.
 */
uint8_t const deflate_length_bits[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4,
	5, 5, 5, 5, 0
};

// can also be expressed as (index < 4 || index == 28) ? 0 : (index-4) >> 2

/*
 * Base distances for distance codes (code 0 to 29).
 * Code 0 indicates a distance of 1, etc.
 */
uint16_t const deflate_distance_offsets[] = {
	1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385,
	513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};

/*
 * Extra bits for distance codes (code 0 to 29).
 * Code 0 has no extra bits, code 4 has 1 bit, etc.
 */
uint8_t const deflate_distance_bits[] = {
	0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10,
	10, 11, 11, 12, 12, 13, 13
};

// can also be expressed as index < 2 ? 0 : (index-2) >> 1

/*
 * In block type 2 (dynamic huffman codes), the code lengths of literal/length
 * and distance alphabet are themselves stored as huffman codes. To save space
 * in case only a few code lengths are used, the code length codes are stored
 * in the following order. This allows a few bits to be saved if some code
 * lengths are unused and the unused code lengths are at the end of the list.
 */
uint8_t const deflate_hclen_index[] = {
	16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

/*
 * Code lengths of the "code length" code (see above).
 */
uint8_t deflate_hc_lengths[19];

/*
 * Code lengths of the literal/length and distance alphabets.
 * up to 288 literal/length codes + up to 30 distance codes.
 */
uint8_t deflate_lld_lengths[318];

#ifdef DEFLATE_WITH_LUT
uint16_t deflate_ll_codes[288];
uint16_t deflate_d_codes[30];
#endif

/*
 * Bit length counts and next code entries for Literal/Length alphabet.
 * Combined with the code lengths in deflate_lld_lengths, these make up the
 * Literal/Length alphabet. See the algorithm in RFC 1951 section 3.2.2 for
 * details.
 *
 * Assumption: There are no more than 255 huffman codes with the same length.
 * As the largest alphabet (the literal/length alphabet) contains just 288
 * codes in total, this should be reasonable.
 *
 * These variables are also used for the huffman alphabet in dynamic huffman
 * blocks.
 */
uint8_t deflate_bl_count_ll[16];
uint16_t deflate_next_code_ll[16];

/*
 * Bit length counts and next code entries for Distance alphabet. Note that,
 * even though there are just 30 different distance codes, individual
 * codes may be up to 16 bits long.
 */
uint8_t deflate_bl_count_d[16];
uint16_t deflate_next_code_d[16];

static uint16_t deflate_rev_word(uint16_t word, uint8_t bits)
{
	uint16_t ret = 0;
	uint16_t mask = 1;
	for (uint16_t rmask = 1 << (bits - 1); rmask > 0; rmask >>= 1) {
		if (word & rmask) {
			ret |= mask;
		}
		mask <<= 1;
	}
	return ret;
}

static uint16_t deflate_bitmask(uint8_t bit_count)
{
	return (1 << bit_count) - 1;
}

static uint16_t deflate_get_word()
{
	uint16_t ret = 0;
	ret |= (deflate_input_now[0] >> deflate_bit_offset);
	ret |= (uint16_t) deflate_input_now[1] << (8 - deflate_bit_offset);
	if (deflate_bit_offset) {
		ret |=
		    (uint16_t) (deflate_input_now[2] &
				deflate_bitmask(deflate_bit_offset)) << (16 -
									 deflate_bit_offset);
	}
	return ret;
}

static uint16_t deflate_get_bits(uint8_t num_bits)
{
	uint16_t ret = deflate_get_word();
	deflate_bit_offset += num_bits;
	while (deflate_bit_offset >= 8) {
		deflate_input_now++;
		deflate_bit_offset -= 8;
	}
	return ret & deflate_bitmask(num_bits);
}

#ifdef DEFLATE_WITH_LUT
static void deflate_build_alphabet(uint8_t * lengths, uint16_t size,
				   uint8_t * bl_count, uint16_t * next_code,
				   uint16_t * codes)
#else
static void deflate_build_alphabet(uint8_t * lengths, uint16_t size,
				   uint8_t * bl_count, uint16_t * next_code)
#endif
{
	uint16_t i;
	uint16_t code = 0;
	uint16_t max_len = 0;
	for (i = 0; i < 16; i++) {
		bl_count[i] = 0;
	}

	for (i = 0; i < size; i++) {
		if (lengths[i]) {
			bl_count[lengths[i]]++;
		}
		if (lengths[i] > max_len) {
			max_len = lengths[i];
		}
	}

	for (i = 1; i <= max_len; i++) {
		code = (code + bl_count[i - 1]) << 1;
		next_code[i] = code;
	}

#ifdef DEFLATE_WITH_LUT
	uint8_t j = 0;
	code = 0;
	for (j = 1; j <= max_len; j++) {
		for (i = 0; i < size; i++) {
			if (lengths[i] == j) {
				codes[code++] = i;
			}
		}
	}
#endif
}

#ifdef DEFLATE_WITH_LUT
static uint16_t deflate_huff(uint16_t * codes,
			     uint8_t * bl_count, uint16_t * next_code)
#else
/*
 * This function trades speed for low memory requirements. Instead of building
 * an actual huffman tree (at a cost of about 650 Bytes of RAM), we iterate
 * through the code lengths whenever we have found a huffman code.  This is
 * very slow, but memory-efficient.
 */
static uint16_t deflate_huff(uint8_t * lengths, uint16_t size,
			     uint8_t * bl_count, uint16_t * next_code)
#endif
{
	uint16_t next_word = deflate_get_word();
#ifdef DEFLATE_WITH_LUT
	uint16_t code = 0;
#endif
	for (uint8_t num_bits = 1; num_bits < 16; num_bits++) {
		uint16_t next_bits = deflate_rev_word(next_word, num_bits);
		if (bl_count[num_bits] && next_bits >= next_code[num_bits]
		    && next_bits < next_code[num_bits] + bl_count[num_bits]) {
			deflate_bit_offset += num_bits;
			while (deflate_bit_offset >= 8) {
				deflate_input_now++;
				deflate_bit_offset -= 8;
			}
#ifdef DEFLATE_WITH_LUT
			return codes[code + (next_bits - next_code[num_bits])];
#else
			uint8_t len_pos = next_bits;
			uint8_t cur_pos = next_code[num_bits];
			for (uint16_t i = 0; i < size; i++) {
				if (lengths[i] == num_bits) {
					if (cur_pos == len_pos) {
						return i;
					}
					cur_pos++;
				}
			}
#endif
		} else {
#ifdef DEFLATE_WITH_LUT
			code += bl_count[num_bits];
#endif
		}
	}
	return 65535;
}

#ifdef DEFLATE_WITH_LUT
static int8_t deflate_huffman(uint16_t * ll_codes, uint16_t * d_codes)
#else
static int8_t deflate_huffman(uint8_t * ll_lengths, uint16_t ll_size,
			      uint8_t * d_lengths, uint8_t d_size)
#endif
{
	uint16_t code;
	uint16_t dcode;
	while (1) {
#ifdef DEFLATE_WITH_LUT
		code =
		    deflate_huff(ll_codes, deflate_bl_count_ll,
				 deflate_next_code_ll);
#else
		code =
		    deflate_huff(ll_lengths, ll_size, deflate_bl_count_ll,
				 deflate_next_code_ll);
#endif
		if (code < 256) {
			if (deflate_output_now == deflate_output_end) {
				return DEFLATE_ERR_OUTPUT_LENGTH;
			}
			*deflate_output_now = code;
			deflate_output_now++;
		} else if (code == 256) {
			return 0;
		} else if (code == 65535) {
			return DEFLATE_ERR_HUFFMAN;
		} else {
			uint16_t len_val = deflate_length_offsets[code - 257];
			uint8_t extra_bits = deflate_length_bits[code - 257];
			if (extra_bits) {
				len_val += deflate_get_bits(extra_bits);
			}
#ifdef DEFLATE_WITH_LUT
			dcode =
			    deflate_huff(d_codes,
					 deflate_bl_count_d,
					 deflate_next_code_d);
#else
			dcode =
			    deflate_huff(d_lengths, d_size,
					 deflate_bl_count_d,
					 deflate_next_code_d);
#endif
			uint16_t dist_val = deflate_distance_offsets[dcode];
			extra_bits = deflate_distance_bits[dcode];
			if (extra_bits) {
				dist_val += deflate_get_bits(extra_bits);
			}
			while (len_val--) {
				if (deflate_output_now == deflate_output_end) {
					return DEFLATE_ERR_OUTPUT_LENGTH;
				}
				deflate_output_now[0] =
				    *(deflate_output_now - dist_val);
				deflate_output_now++;
			}
		}
		if (deflate_input_now >= deflate_input_end - 4) {
			return DEFLATE_ERR_INPUT_LENGTH;
		}
	}
}

static int8_t deflate_uncompressed()
{
	if (deflate_bit_offset) {
		deflate_input_now++;
		deflate_bit_offset = 0;
	}
	uint16_t len =
	    ((uint16_t) deflate_input_now[1] << 8) + deflate_input_now[0];
	uint16_t nlen =
	    ((uint16_t) deflate_input_now[3] << 8) + deflate_input_now[2];
	if (len & nlen) {
		return DEFLATE_ERR_NLEN;
	}
	deflate_input_now += 4;
	if (deflate_input_now + len >= deflate_input_end) {
		return DEFLATE_ERR_INPUT_LENGTH;
	}
	if (deflate_output_now + len >= deflate_output_end) {
		return DEFLATE_ERR_OUTPUT_LENGTH;
	}
	for (uint16_t i = 0; i < len; i++) {
		*(deflate_output_now++) = *(deflate_input_now++);
	}
	return 0;
}

static int8_t deflate_static_huffman()
{
	uint16_t i;
	for (i = 0; i <= 143; i++) {
		deflate_lld_lengths[i] = 8;
	}
	for (i = 144; i <= 255; i++) {
		deflate_lld_lengths[i] = 9;
	}
	for (i = 256; i <= 279; i++) {
		deflate_lld_lengths[i] = 7;
	}
	for (i = 280; i <= 287; i++) {
		deflate_lld_lengths[i] = 8;
	}
	for (i = 288; i <= 288 + 29; i++) {
		deflate_lld_lengths[i] = 5;
	}

#ifdef DEFLATE_WITH_LUT
	deflate_build_alphabet(deflate_lld_lengths, 288, deflate_bl_count_ll,
			       deflate_next_code_ll, deflate_ll_codes);
	deflate_build_alphabet(deflate_lld_lengths + 288, 29,
			       deflate_bl_count_d, deflate_next_code_d,
			       deflate_d_codes);
	return deflate_huffman(deflate_ll_codes, deflate_d_codes);
#else
	deflate_build_alphabet(deflate_lld_lengths, 288, deflate_bl_count_ll,
			       deflate_next_code_ll);
	deflate_build_alphabet(deflate_lld_lengths + 288, 29,
			       deflate_bl_count_d, deflate_next_code_d);
	return deflate_huffman(deflate_lld_lengths, 288,
			       deflate_lld_lengths + 288, 29);
#endif
}

static int8_t deflate_dynamic_huffman()
{
	uint16_t hlit = 257 + deflate_get_bits(5);
	uint8_t hdist = 1 + deflate_get_bits(5);
	uint8_t hclen = 4 + deflate_get_bits(4);

	for (uint8_t i = 0; i < hclen; i++) {
		deflate_hc_lengths[deflate_hclen_index[i]] =
		    deflate_get_bits(3);
	}
	for (uint8_t i = hclen; i < sizeof(deflate_hc_lengths); i++) {
		deflate_hc_lengths[deflate_hclen_index[i]] = 0;
	}

#ifdef DEFLATE_WITH_LUT
	deflate_build_alphabet(deflate_hc_lengths,
			       sizeof(deflate_hc_lengths),
			       deflate_bl_count_ll, deflate_next_code_ll,
			       deflate_ll_codes);
#else
	deflate_build_alphabet(deflate_hc_lengths,
			       sizeof(deflate_hc_lengths),
			       deflate_bl_count_ll, deflate_next_code_ll);
#endif

	uint16_t items_processed = 0;
	while (items_processed < hlit + hdist) {
#ifdef DEFLATE_WITH_LUT
		uint8_t code = deflate_huff(deflate_ll_codes,
					    deflate_bl_count_ll,
					    deflate_next_code_ll);
#else
		uint8_t code =
		    deflate_huff(deflate_hc_lengths, sizeof(deflate_hc_lengths),
				 deflate_bl_count_ll,
				 deflate_next_code_ll);
#endif
		if (code == 16) {
			uint8_t copy_count = 3 + deflate_get_bits(2);
			for (uint8_t i = 0; i < copy_count; i++) {
				deflate_lld_lengths[items_processed] =
				    deflate_lld_lengths[items_processed - 1];
				items_processed++;
			}
		} else if (code == 17) {
			uint8_t null_count = 3 + deflate_get_bits(3);
			for (uint8_t i = 0; i < null_count; i++) {
				deflate_lld_lengths[items_processed] = 0;
				items_processed++;
			}
		} else if (code == 18) {
			uint8_t null_count = 11 + deflate_get_bits(7);
			for (uint8_t i = 0; i < null_count; i++) {
				deflate_lld_lengths[items_processed] = 0;
				items_processed++;
			}
		} else {
			deflate_lld_lengths[items_processed] = code;
			items_processed++;
		}
	}

#ifdef DEFLATE_WITH_LUT
	deflate_build_alphabet(deflate_lld_lengths, hlit,
			       deflate_bl_count_ll, deflate_next_code_ll,
			       deflate_ll_codes);
	deflate_build_alphabet(deflate_lld_lengths + hlit, hdist,
			       deflate_bl_count_d, deflate_next_code_d,
			       deflate_d_codes);
	return deflate_huffman(deflate_ll_codes, deflate_d_codes);
#else
	deflate_build_alphabet(deflate_lld_lengths, hlit,
			       deflate_bl_count_ll, deflate_next_code_ll);
	deflate_build_alphabet(deflate_lld_lengths + hlit, hdist,
			       deflate_bl_count_d, deflate_next_code_d);
	return deflate_huffman(deflate_lld_lengths, hlit,
			       deflate_lld_lengths + hlit, hdist);
#endif
}

int16_t inflate(const unsigned char *input_buf, uint16_t input_len,
		unsigned char *output_buf, uint16_t output_len)
{
	deflate_input_now = input_buf;
	deflate_input_end = input_buf + input_len;
	deflate_bit_offset = 0;

	deflate_output_now = output_buf;
	deflate_output_end = output_buf + output_len;

	while (1) {
		uint8_t block_type = deflate_get_bits(3);
		uint8_t is_final = block_type & 0x01;
		int8_t ret;

		block_type >>= 1;

		switch (block_type) {
		case 0:
			ret = deflate_uncompressed();
			break;
		case 1:
			ret = deflate_static_huffman();
			break;
		case 2:
			ret = deflate_dynamic_huffman();
			break;
		default:
			return DEFLATE_ERR_BLOCK;
		}

		if (ret < 0) {
			return ret;
		}

		if (is_final) {
			return deflate_output_now - output_buf;
		}
	}
}

int16_t inflate_zlib(const unsigned char *input_buf, uint16_t input_len,
		     unsigned char *output_buf, uint16_t output_len)
{
	if (input_len < 4) {
		return DEFLATE_ERR_INPUT_LENGTH;
	}
	uint8_t zlib_method = input_buf[0] & 0x0f;
	uint8_t zlib_flags = input_buf[1];

	if (zlib_method != 8) {
		return DEFLATE_ERR_METHOD;
	}

	if (zlib_flags & 0x20) {
		return DEFLATE_ERR_FDICT;
	}

	if ((((uint16_t) input_buf[0] << 8) | input_buf[1]) % 31) {
		return DEFLATE_ERR_FCHECK;
	}

	int16_t ret =
	    inflate(input_buf + 2, input_len - 2, output_buf, output_len);

#ifdef DEFLATE_CHECKSUM
	if (ret >= 0) {
		uint16_t deflate_s1 = 1;
		uint16_t deflate_s2 = 0;

		deflate_output_end = deflate_output_now;
		for (deflate_output_now = output_buf;
		     deflate_output_now < deflate_output_end;
		     deflate_output_now++) {
			deflate_s1 =
			    ((uint32_t) deflate_s1 +
			     (uint32_t) (*deflate_output_now)) % 65521;
			deflate_s2 =
			    ((uint32_t) deflate_s2 +
			     (uint32_t) deflate_s1) % 65521;
		}

		if (deflate_bit_offset) {
			deflate_input_now++;
		}

		if ((deflate_s2 !=
		     (((uint16_t) deflate_input_now[0] << 8) | (uint16_t)
		      deflate_input_now[1]))
		    || (deflate_s1 !=
			(((uint16_t) deflate_input_now[2] << 8) | (uint16_t)
			 deflate_input_now[3]))) {
			return DEFLATE_ERR_CHECKSUM;
		}
	}
#endif

	return ret;
}
