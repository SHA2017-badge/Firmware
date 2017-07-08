#ifndef LIB_PNG_READER_H
#define LIB_PNG_READER_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "reader.h"

struct lib_png_chunk {
	bool in_chunk;
	uint32_t type;
	uint32_t len;
	uint32_t crc;
};

struct lib_png_ihdr {
	uint32_t width;
	uint32_t height;
	uint8_t bit_depth;
	uint8_t color_type;
	uint8_t compression_method;
	uint8_t filter_method;
	uint8_t interlace_method;
} __attribute__((packed));

struct lib_png_reader {
	lib_reader_read_t read;
	void *read_p;

	struct lib_png_chunk chunk;
	struct lib_png_ihdr ihdr;

	uint8_t *palette;
	int palette_len;

	uint8_t scanline_bpp; // 1, 2, 3, 4, 6 or 8
	uint32_t scanline_width;
	uint8_t *scanline; // large enough for scanline + temp secondary scanline

	struct lib_deflate_reader *dr;
	uint32_t adler;
};

extern struct lib_png_reader * lib_png_new(lib_reader_read_t read, void *read_p);
extern int lib_png_load_image(struct lib_png_reader *pr, uint8_t *dst, int dst_width, int dst_height, int dst_linelen);
extern void lib_png_destroy(struct lib_png_reader *pr);

#endif // LIB_PNG_READER_H
