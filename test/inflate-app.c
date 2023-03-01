#include <stdio.h>
#include <stdlib.h>

#include "inflate.h"

int main(int argc, char** argv)
{

	// 16 MB
	unsigned char *inbuf = (unsigned char*)malloc(4096 * 4096);
	unsigned char *outbuf = (unsigned char*)malloc(4096 * 4096);

	if (inbuf == NULL || outbuf == NULL) {
		return 1;
	}

        size_t dict_size = 0;
        unsigned char *dict_buf = NULL;
        if (argc > 1) {
          dict_buf = (unsigned char*)malloc(4096 * 4096);
          FILE* dict_file = fopen(argv[1], "rb");
          if (!dict_buf || !dict_file) {
            return 1;
          }
          dict_size = fread(dict_buf, 1, 4096 * 4096, dict_file);
          if (dict_size <= 0) {
            return 1;
          }
          dict_size /= 2;
          fclose(dict_file);
        }

	size_t in_size = fread(inbuf, 1, 4096 * 4096, stdin);

	if (in_size == 0) {
		return 1;
	}

	int16_t out_size = inflate_zlib(inbuf, in_size, outbuf, 65535, dict_buf, dict_size);

	if (out_size < 0) {
		return -out_size;
	}

	fwrite(outbuf, 1, out_size, stdout);

	return 0;
}
