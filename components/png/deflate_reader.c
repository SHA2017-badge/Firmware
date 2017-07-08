#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "deflate_reader.h"

static inline int
lib_deflate_get_bit(struct lib_deflate_reader *dr)
{
	if (dr->bitlen == 0)
	{
		ssize_t res = dr->read(dr->read_p, &(dr->bitbuf[1]), 1);
		if (res < 0)
			return res;
		if (res < 1)
			return -LIB_DEFLATE_ERROR_UNEXPECTED_END_OF_FILE;
		int value = dr->bitbuf[1] & 1;
		dr->bitlen = 7;
		return value;
	}

	int value = (dr->bitbuf[1] >> (8 - dr->bitlen)) & 1;
	dr->bitlen--;

	return value;
}

static inline int
lib_deflate_get_bits(struct lib_deflate_reader *dr, int num)
{
	int value = dr->bitbuf[1];
	if (num > dr->bitlen + 8)
	{
		ssize_t res = dr->read(dr->read_p, dr->bitbuf, 2);
		if (res < 0)
			return res;
		if (res < 2)
			return -LIB_DEFLATE_ERROR_UNEXPECTED_END_OF_FILE;
		value |= (dr->bitbuf[1] << 16) | (dr->bitbuf[0] << 8);
	}
	else if (num > dr->bitlen)
	{
		ssize_t res = dr->read(dr->read_p, &(dr->bitbuf[1]), 1);
		if (res < 0)
			return res;
		if (res < 1)
			return -LIB_DEFLATE_ERROR_UNEXPECTED_END_OF_FILE;
		value |= dr->bitbuf[1] << 8;
	}

	value >>= 8 - dr->bitlen;
	value &= (1 << num) - 1;
	dr->bitlen -= num;
	dr->bitlen &= 7;

	return value;
}

static inline void
lib_deflate_build_huffman(const uint8_t *tbl, int tbl_len, uint16_t *tree)
{
	int bits;
	int bits_prev = 0;
	for (bits=1; bits<=16; bits++)
	{
		uint16_t *base = NULL;
		int i;
		for (i=0; i<tbl_len; i++)
		{
			if (tbl[i] == bits)
			{
				if (base == NULL)
				{
					base = tree++;
					*base = (1 << 4) | (bits - bits_prev);
					bits_prev = bits;
				}
				else
				{
					*base += 1 << 4;
				}
				*tree++ = i;
			}
		}
	}
}

static inline int
lib_deflate_get_huffman(struct lib_deflate_reader *dr, const uint16_t *tbl)
{
	int value = 0;
	while (1)
	{
		uint16_t bits = *tbl++;
		uint16_t entries = bits >> 4;
		bits &= 15;
		while (bits > 0)
		{
			int res = lib_deflate_get_bit(dr);
			if (res < 0)
				return res;

			value <<= 1;
			value |= res;
			bits--;
		}

		if (value < entries)
			return tbl[value];

		value -= entries;
		tbl = &tbl[entries];
	}

	return value;
}

static inline int
lib_deflate_check_huffman(const uint8_t *tbl, int tbl_len)
{
	int total = 0;
	int i;
	for (i=0; i<tbl_len; i++)
	{
		int bits = tbl[i];

		if (bits)
			total += 0x10000 >> bits;
	}

	if (total != 0x10000)
	{
		// table not well-balanced. something is wrong.
		return -LIB_DEFLATE_ERROR_UNBALANCED_HUFFMAN_TREE;
	}

	return 0;
}

struct lib_deflate_reader *
lib_deflate_new(lib_reader_read_t read, void *read_p)
{
	struct lib_deflate_reader *dr = (struct lib_deflate_reader *) malloc(sizeof(struct lib_deflate_reader));
	if (dr == NULL)
		return NULL;

	memset(dr, 0, sizeof(struct lib_deflate_reader));
	dr->read = read;
	dr->read_p = read_p;

	return dr;
}

ssize_t
lib_deflate_read(struct lib_deflate_reader *dr, uint8_t *buf, size_t buf_len)
{
	ssize_t buf_pos = 0;
	while (buf_pos < buf_len)
	{
		if (dr->state == LIB_DEFLATE_STATE_NEW_BLOCK)
		{ // initial state; have to read block header
			if (dr->is_last_block)
				return buf_pos;

			int block_type = lib_deflate_get_bits(dr, 3);
			if (block_type < 0)
				return block_type;

			dr->is_last_block = (block_type & 1) ? true : false;
			block_type >>= 1;

			if (block_type == 0)
			{ // stored block
				dr->bitlen = 0;

				uint8_t rd_buf[4];
				ssize_t res = dr->read(dr->read_p, rd_buf, 4);
				if (res < 0)
					return res;
				if (res < 4)
					return -LIB_DEFLATE_ERROR_UNEXPECTED_END_OF_FILE;

				int len_lo = rd_buf[0];
				int len_hi = rd_buf[1];
				int nlen_lo = rd_buf[2];
				int nlen_hi = rd_buf[3];

				int len = (len_hi << 8) | len_lo;
				int nlen = (nlen_hi << 8) | nlen_lo;
				if (len + nlen != 0xffff)
					return -LIB_DEFLATE_ERROR_INVALID_COPY_LENGTH;

				dr->copy_len = len;
				dr->state = LIB_DEFLATE_STATE_STORED_BLOCK;
			}
			else if (block_type == 1)
			{ // static huffman
				uint8_t huffman_lc[288];
				memset(&huffman_lc[  0], 8, 144 - 0);
				memset(&huffman_lc[144], 9, 256 - 144);
				memset(&huffman_lc[256], 7, 280 - 256);
				memset(&huffman_lc[280], 8, 288 - 280);
				lib_deflate_build_huffman(huffman_lc, 288, dr->huffman_lc_tree);

				uint8_t huffman_dc[32];
				memset(&huffman_dc[  0], 5, 32);
				lib_deflate_build_huffman(huffman_dc, 32, dr->huffman_dc_tree);

				dr->state = LIB_DEFLATE_STATE_HUFFMAN;
			}
			else if (block_type == 2)
			{ // dynamic huffman
				int lc_num = lib_deflate_get_bits(dr, 5);
				if (lc_num < 0)
					return lc_num;
				lc_num += 257;

				int dc_num = lib_deflate_get_bits(dr, 5);
				if (dc_num < 0)
					return dc_num;
				dc_num += 1;

				int blc_num = lib_deflate_get_bits(dr, 4);
				if (blc_num < 0)
					return blc_num;
				blc_num += 4;

				static const uint8_t blc_order[19] = {
					16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15, 
				};
				uint8_t blc[19];
				memset(blc, 0, sizeof(blc));
				int i;
				for (i=0; i<blc_num; i++)
				{
					int bits = lib_deflate_get_bits(dr, 3);
					if (bits < 0)
						return bits;
					blc[blc_order[i]] = bits;
				}
				int res = lib_deflate_check_huffman(blc, sizeof(blc));
				if (res < 0)
					return res; // invalid table

				uint16_t blc_tree[19 + 15];
				lib_deflate_build_huffman(blc, sizeof(blc), blc_tree);

				uint8_t huffman_lc[288];
				int lc_i=0;
				while (lc_i < lc_num)
				{
					int len = lib_deflate_get_huffman(dr, blc_tree);
					if (len < 0)
						return len;

					if (len < 16)
					{
						huffman_lc[lc_i++] = len;
					}
					else if (len == 16)
					{
						if (lc_i == 0)
							return -LIB_DEFLATE_ERROR_DYNAMIC_HUFFMAN_SETUP_ERROR;
						uint8_t prev_len = huffman_lc[lc_i - 1];
						int repeat = lib_deflate_get_bits(dr, 2);
						if (repeat < 0)
							return repeat;
						repeat += 3;
						if (lc_i + repeat > lc_num)
							return -LIB_DEFLATE_ERROR_DYNAMIC_HUFFMAN_SETUP_ERROR; // overflow
						while (repeat--)
							huffman_lc[lc_i++] = prev_len;
					}
					else if (len == 17)
					{
						int repeat = lib_deflate_get_bits(dr, 3);
						if (repeat < 0)
							return repeat;
						repeat += 3;
						if (lc_i + repeat > lc_num)
							return -LIB_DEFLATE_ERROR_DYNAMIC_HUFFMAN_SETUP_ERROR; // overflow
						while (repeat--)
							huffman_lc[lc_i++] = 0;
					}
					else if (len == 18)
					{
						int repeat = lib_deflate_get_bits(dr, 7);
						if (repeat < 0)
							return repeat;
						repeat += 11;
						if (lc_i + repeat > lc_num)
							return -LIB_DEFLATE_ERROR_DYNAMIC_HUFFMAN_SETUP_ERROR; // overflow
						while (repeat--)
							huffman_lc[lc_i++] = 0;
					}
					else return -LIB_DEFLATE_ERROR_DYNAMIC_HUFFMAN_SETUP_ERROR;
				}
				res = lib_deflate_check_huffman(huffman_lc, lc_num);
				if (res < 0)
					return res; // invalid table

				lib_deflate_build_huffman(huffman_lc, lc_num, dr->huffman_lc_tree);

				uint8_t huffman_dc[32];
				int dc_i=0;
				while (dc_i < dc_num)
				{
					int len = lib_deflate_get_huffman(dr, blc_tree);
					if (len < 0)
						return len;

					if (len < 16)
					{
						huffman_dc[dc_i++] = len;
					}
					else if (len == 16)
					{
						if (dc_i == 0)
							return -LIB_DEFLATE_ERROR_DYNAMIC_HUFFMAN_SETUP_ERROR;
						uint8_t prev_len = huffman_dc[dc_i - 1];
						int repeat = lib_deflate_get_bits(dr, 2);
						if (repeat < 0)
							return repeat;
						repeat += 3;
						if (dc_i + repeat > dc_num)
							return -LIB_DEFLATE_ERROR_DYNAMIC_HUFFMAN_SETUP_ERROR; // overflow
						while (repeat--)
							huffman_dc[dc_i++] = prev_len;
					}
					else if (len == 17)
					{
						int repeat = lib_deflate_get_bits(dr, 3);
						if (repeat < 0)
							return repeat;
						repeat += 3;
						if (dc_i + repeat > dc_num)
							return -LIB_DEFLATE_ERROR_DYNAMIC_HUFFMAN_SETUP_ERROR; // overflow
						while (repeat--)
							huffman_dc[dc_i++] = 0;
					}
					else if (len == 18)
					{
						int repeat = lib_deflate_get_bits(dr, 7);
						if (repeat < 0)
							return repeat;
						repeat += 11;
						if (dc_i + repeat > dc_num)
							return -LIB_DEFLATE_ERROR_DYNAMIC_HUFFMAN_SETUP_ERROR; // overflow
						while (repeat--)
							huffman_dc[dc_i++] = 0;
					}
					else return -LIB_DEFLATE_ERROR_DYNAMIC_HUFFMAN_SETUP_ERROR;
				}
				res = lib_deflate_check_huffman(huffman_dc, dc_num);
				if (res < 0)
					return res; // invalid table

				lib_deflate_build_huffman(huffman_dc, dc_num, dr->huffman_dc_tree);

				dr->state = LIB_DEFLATE_STATE_HUFFMAN;
			}
			else
			{ // reserved block_type
				return -LIB_DEFLATE_ERROR_RESERVED_BLOCK_TYPE;
			}
		}

		if (dr->state == LIB_DEFLATE_STATE_STORED_BLOCK)
		{ // stored block
			while (dr->copy_len > 0)
			{
				if (buf_pos >= buf_len)
					return buf_pos;

				size_t copylen = dr->copy_len;
				if (copylen > buf_len - buf_pos)
					copylen = buf_len - buf_pos;
				if (copylen > 32768 - dr->lb_pos)
					copylen = 32768 - dr->lb_pos;

				uint8_t *rd_buf = &dr->look_behind[ dr->lb_pos ];
				ssize_t res = dr->read(dr->read_p, rd_buf, copylen);
				if (res < 0)
					return res;
				if (res < copylen)
					return -LIB_DEFLATE_ERROR_UNEXPECTED_END_OF_FILE;

				memcpy(&buf[buf_pos], rd_buf, copylen);
				buf_pos += copylen;

				dr->copy_len -= copylen;

				dr->lb_pos += copylen;
				dr->lb_pos &= 32767;
				if (dr->lb_size < 32768)
					dr->lb_size += copylen;
			}
			dr->state = LIB_DEFLATE_STATE_NEW_BLOCK;
		}

		if (dr->state == LIB_DEFLATE_STATE_HUFFMAN)
		{ // huffman encoded block.
			int token = lib_deflate_get_huffman(dr, dr->huffman_lc_tree);
			if (token < 0)
				return token;
			if (token < 256)
			{ // store token and return
				buf[buf_pos++] = token;

				dr->look_behind[ dr->lb_pos ] = token;
				dr->lb_pos++;
				dr->lb_pos &= 32767;
				if (dr->lb_size < 32768)
					dr->lb_size++;
			}
			else if (token == 256)
			{ // next block
				dr->state = LIB_DEFLATE_STATE_NEW_BLOCK;
			}
			else
			{ // copy data from look_behind
				// determine copy-length
				token -= 257;
				if (token >= 29)
					return -LIB_DEFLATE_ERROR_HUFFMAN_RESERVED_LENGTH; // invalid index
				static const uint16_t clp[29] = {
					3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258
				};
				static const uint8_t clb[29] = {
					0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0
				};
				int copy_len = lib_deflate_get_bits(dr, clb[ token ]);
				if (copy_len < 0)
					return copy_len;
				copy_len += clp[ token ];

				// determine distance
				int token = lib_deflate_get_huffman(dr, dr->huffman_dc_tree);
				if (token < 0)
					return token;
				if (token >= 30)
					return -LIB_DEFLATE_ERROR_HUFFMAN_RESERVED_LENGTH; // invalid index
				static const uint16_t dcp[30] = {
					1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577
				};
				static const uint8_t dcb[30] = {
					0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13
				};
				int dist = lib_deflate_get_bits(dr, dcb[ token ]);
				if (dist < 0)
					return dist;
				dist += dcp[ token ];

				if (dist > dr->lb_size)
					return -LIB_DEFLATE_ERROR_HUFFMAN_INVALID_DISTANCE; // invalid distance

				dr->copy_dist = dist;
				dr->copy_len = copy_len;
				dr->state = LIB_DEFLATE_STATE_HUFFMAN_REPEAT;
			}
		}

		if (dr->state == LIB_DEFLATE_STATE_HUFFMAN_REPEAT)
		{ // repeating old data. will continue huffman decoding afterwards
			while (dr->copy_len > 0)
			{
				if (buf_pos >= buf_len)
					return buf_pos;

				// int copylen = dr->copy_len;
				// copylen = buf_len - buf_pos if copylen > buf_len - buf_pos
				// copylen = dr->copy_dist if copylen > dr->copy_dist
				// copylen = 32768 - dr->lb_pos if copylen > 32768 - dr->lb_pos
				// copylen = 32768 - pos if copylen > 32768 - pos
				dr->copy_len--;
				int pos = (dr->lb_pos - dr->copy_dist) & 32767;
				int token = dr->look_behind[ pos ];

				buf[buf_pos++] = token;

				dr->look_behind[ dr->lb_pos ] = token;
				dr->lb_pos++;
				dr->lb_pos &= 32767;
				if (dr->lb_size < 32768)
					dr->lb_size++;
			}
			dr->state = LIB_DEFLATE_STATE_HUFFMAN;
		}
	}

	return buf_pos;
}

void
lib_deflate_destroy(struct lib_deflate_reader *dr)
{
	free(dr);
}
