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

Note: This library *inflates* (i.e., decompresses) data. The source files and
API are named as such, as is the corresponding function in the original zlib
implementation. However, as the algorithm is called *deflate*, the project is
named zlib-*deflate*-nostdlib even though it does not support compression.

## Usage

Embed `inflate.c` and `inflate.h` into your project. You can rename `inflate.c`
to `inflate.cc` and/or compile it with g++ instead of gcc, if you like. Use
`inflate_zlib(input, input_len, output, output_len)` to decompress zlib data,
and `inflate(input, input_len, output, output_len)` to decompress deflate data
without zlib header.

input and output must be `unsigned char *`, input\_len and output\_len are
expected to be unsigned 16-bit integers. Both functions return the number of
bytes written to `output`, or a negative value on error.

Example for zlib decompression (RFC 1950):

```
#include "inflate.h"

unsigned char inflate_input[] = { /* some compressed data, e.g.: */
    120, 156, 243, 72, 205, 201, 201, 215, 81, 8, 207, 47, 202, 73, 177, 87,
    240, 64, 226, 41, 2, 0, 128, 125, 9, 17
};

unsigned char inflate_output[128];

// within some function
{
    int16_t out_bytes = inflate_zlib(inflate_input, sizeof(inflate_input),
                                     inflate_output, sizeof(inflate_output));
    if (out_bytes < 0) {
        // error
    } else {
        // success. inflate_output contains "Hello, World? Hello, World!"
        // out_bytes contains the number of bytes written to inflate_output
    }
}

```

Decompressing deflate (RFC 1951) data works as follows:

```
#include "inflate.h"

unsigned char inflate_input[] = { /* some compressed data, e.g.: */
    243, 72, 205, 201, 201, 215, 81, 8, 207, 47, 202, 73, 177, 87,
    240, 64, 226, 41, 2, 0
};

unsigned char inflate_output[128];

// within some function
{
    int16_t out_bytes = inflate(inflate_input, sizeof(inflate_input),
                                inflate_output, sizeof(inflate_output));
    if (out_bytes < 0) {
        // error
    } else {
        // success. inflate_output contains "Hello, World? Hello, World!"
        // out_bytes contains the number of bytes written to inflate_output
    }
}

```

## Compilation flags

Compile with `-DDEFLATE_CHECKSUM` to enable verification of the zlib ADLER32
checksum in `inflate_zlib`.

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

## Requirements and Performance

RAM usage excludes the space needed for input and output buffer. ROM/RAM usage
rounded up to the next multiple of 16B. Performance tested with text files of
various sizes, minimum file size 500 bytes, maximum file size determined by the
amount of available RAM.

| Architecture | ROM | RAM | Speed
| :--- | ---: | ---: | ---: |
| 8-bit ATMega328P @ 16 MHz | 1440 B | 624 B | 10 .. 22 kB/s |
| 16-bit MSP430FR5994 @ 16 MHz | 2224 B | 432 B | 8 .. 16 kB/s |
| 20-bit MSP430FR5994 @ 16 MHz | 2512 B | 432 B | 8 .. 16 kB/s |
| 32-bit STM32F446RE (ARM Cortex M3) @ 168 MHz | 1552 B | 432 B | 258 .. 898 kB/s |
