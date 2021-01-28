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

Example for deflate (RFC 1951) decompression:

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

`inflate` is fully compliant with RFC 1951 for data with a decompressed size
of up to 65 kB.

When compiled with `-DDEFLATE_CHECKSUM`, `inflate_zlib` is fully compliant with
RFC 1950 (decompression only) for data with a decompressed size of up to 65 kB.
By default (without `-DDEFLATE_CHECKSUM`), it does not verify the ADLER32
checksum embedded into zlib-compressed data and is therefore not compliant with
RFC 1950.

For files larger than 65 kB, you only need to change some size arguments to
`uint32_t`. However, if you are decompressing files of that size, you probably
have more RAM than this library is designed for. In that case, you may be
better off with [udeflate](https://github.com/jlublin/udeflate),
[uzlib](https://github.com/pfalcon/uzlib), or similar.

## Memory Requirements

Excluding the decompressed data buffer, zlib-deflate-nostdlib needs about
2.5 kB of ROM and 500 Bytes of RAM. Actual values depend on the architecture,
see the tables below. ROM/RAM values are rounded up to the next mupltiple of
16B.

### default (no checksum verification)

| Architecture | ROM | RAM |
| :--- | ---: | ---: | ---: |
| 8-bit ATMega328P | 1824 B | 640 B |
| 16-bit MSP430FR5994 | 2272 B | 448 B |
| 20-bit MSP430FR5994 | 2576 B | 464 B |
| 32-bit ESP8266 | 1888 B | 656 B |
| 32-bit STM32F446RE (ARM Cortex M3) | 1600 B | 464 B |

### compliant mode (-DDEFLATE\_CHECKSUM)

| Architecture | ROM | RAM |
| :--- | ---: | ---: | ---: |
| 8-bit ATMega328P | 2032 B | 640 B |
| 16-bit MSP430FR5994 | 2560 B | 448 B |
| 20-bit MSP430FR5994 | 2896 B | 464 B |
| 32-bit ESP8266 | 2048 B | 656 B |
| 32-bit STM32F446RE (ARM Cortex M3) | 1782 B | 464 B |

## Performance

Due to its focus on low RAM usage, zlib-deflate-nostdlib is very slow. Expect
about 1kB/s per MHz on 16-bit and 2kB/s per MHz on 32-bit architectures. Tested
with text files of various sizes, minimum file size 500 bytes, maximum file
size determined by the amount of available RAM.

| Architecture | Speed @ 1 MHz | Speed | CPU Clock |
| :--- | ---: | ---: | ---: |
| 8-bit ATMega328P | 1 kB/s | 10 .. 22 kB/s | 16 MHz |
| 16-bit MSP430FR5994 | 1 kB/s | 8..15 kB/s | 16 MHz |
| 20-bit MSP430FR5994 | 1 kB/s | 8..17 kB/s | 16 MHz |
| 32-bit ESP8266 | 1 .. 3 kB/s | 79..246 kB/s | 80 MHz |
| 32-bit STM32F446RE (ARM Cortex M3) | 1 .. 5 kB/s | 282..875 kB/s | 168 MHz |
