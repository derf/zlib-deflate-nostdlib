#include <stdio.h>
#include <stdlib.h>

#include "inflate.h"

static int read_til_eof(unsigned char* into, size_t len, FILE* f) {
  int bytes_read = 0;
  while (!feof(f) && !ferror(f)) {
    bytes_read += fread(into, 1, len - bytes_read, f);
  }
  if (ferror(f)) {
    bytes_read = 0;
  }
  return bytes_read;
}

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
          dict_size = read_til_eof(dict_buf, 4096 * 4096, dict_file);
          if (dict_size <= 0) {
            return 1;
          }
          dict_size /= 2;
          fclose(dict_file);
        }

	size_t in_size = read_til_eof(inbuf, 4096 * 4096, stdin);

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
