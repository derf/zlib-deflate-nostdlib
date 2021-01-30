#include <stdio.h>
#include <stdlib.h>

#include "inflate.h"

unsigned char *inbuf;
unsigned char *outbuf;

int main(void)
{
	// 16 MB
	inbuf = (unsigned char*)malloc(4096 * 4096);
	outbuf = (unsigned char*)malloc(4096 * 4096);

	if (inbuf == NULL || outbuf == NULL) {
		return 1;
	}

	size_t in_size = fread(inbuf, 1, 4096 * 4096, stdin);

	if (in_size == 0) {
		return 1;
	}

	int16_t out_size = inflate_zlib(inbuf, in_size, outbuf, 65535);

	if (out_size < 0) {
		return -out_size;
	}

	fwrite(outbuf, 1, out_size, stdout);

	return 0;
}
