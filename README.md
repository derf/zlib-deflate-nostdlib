**zlib-deflate-nostdlib** provides a zlib decompressor (RFC 1950) and deflate
reader (RFC 1951) suitable for 8- and 16-bit microcontrollers. It works
fine on MCUs as small as ATMega328P (used, for example, in the Arduino Nano)
and MSP430FR5994. It is compatible with both C (tested with c99) and C++
(tested with c++14). Apart from type definitions for (u)int8\_t, (u)int16\_t,
and (u)int32\_t, which are typically provided by stdint.h, it has no external
dependencies.

zlib-deflate-nostdlib is focused on a low memory footprint. It is not optimized
for speed. Right now, the implementation is naive, but usable. See below for
the current status and TODOs. Be aware that this library has not been
extensively tested yet.

## Usage

Embed `deflate.c` and `deflate.h` into your project. You can rename `deflate.c`
to `deflate.cc` and/or compile it with g++ instead of gcc, if you like. Use
`deflate_zlib(input, input_len, output, output_len)` to decompress zlib data,
and `deflate(input, input_len, output, output_len)` to decompress deflate data
without zlib header.

input and output must be `unsigned char *`, input\_len and output\_len are
expected to be unsigned 16-bit integers. Both functions return the number of
bytes written to `output`, or a negative value on error.

Example for zlib decompression (RFC 1950):

```
#include "deflate.h"

unsigned char deflate_input[] = { /* some compressed data, e.g.: */
    120, 156, 243, 72, 205, 201, 201, 215, 81, 8, 207, 47, 202, 73, 177, 87,
    240, 64, 226, 41, 2, 0, 128, 125, 9, 17
};

unsigned char deflate_output[128];

// within some function
{
    int16_t out_bytes = deflate_zlib(deflate_input, sizeof(deflate_input),
                                     deflate_output, sizeof(deflate_output));
    if (out_bytes < 0) {
        // error
    } else {
        // success. deflate_output contains "Hello, World? Hello, World!"
        // out_bytes contains the number of bytes written to deflate_output
    }
}

```

Decompressing deflate (RFC 1951) data works as follows:

```
#include "deflate.h"

unsigned char deflate_input[] = { /* some compressed data, e.g.: */
    243, 72, 205, 201, 201, 215, 81, 8, 207, 47, 202, 73, 177, 87,
    240, 64, 226, 41, 2, 0
};

unsigned char deflate_output[128];

// within some function
{
    int16_t out_bytes = deflate(deflate_input, sizeof(deflate_input),
                                deflate_output, sizeof(deflate_output));
    if (out_bytes < 0) {
        // error
    } else {
        // success. deflate_output contains "Hello, World? Hello, World!"
        // out_bytes contains the number of bytes written to deflate_output
    }
}

```

## Compilation flags

Compile with `-DDEFLATE_CHECKSUM` to enable verification of the zlib ADLER32
checksum in `deflate_zlib`.

## Compliance

The code *almost* complies with RFC 1950 (decompression only), with the
following exceptions.

* Unless compiled with `-DDEFLATE_CHECKSUM`, zlib-deflate-nostdlib does not
  verify the ADLER32 checksum embedded into zlib-compressed data.

The code *almost* complies with RFC 1951, with the following exceptions.

* zlib-deflate-nostdlib assumes that Huffman codes are limited to a length
  of 12 bits and that there are no more than 255 codes per length. This appears
  to be a reasonable assumption for embedded devices, whose decompression
  abilities are limited by the amount of RAM anyways. I have not yet determined
  whether longer Huffman codes can appear in practice or not, and if so, under
  which conditions.
* zlib-deflate-nostdlib does not yet support compressed items consisting of
  more than one deflate block. I intend to fix this.

## Requirements

RAM usage excludes the space needed for input and output buffer. Numbers
rounded up to the next multiple of 16B.

| Architecture | ROM | RAM
| :--- | ---: | ---: |
| 8-bit ATMega328P | 1584 B | 624 B |
| 16-bit MSP430FR5994 | 2304 B | 432 B |
| 20-bit MSP430FR5994 | 2608 B | 432 B |
| 32-bit STM32F446RE (ARM Cortex M3) | 1744 B | 432 B |
