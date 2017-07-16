#include <alloca.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "redundancy.h"

#define LU_BITS 4
typedef uint8_t gbf_clmul_t;

#define LU_NUM (1<<LU_BITS)
#define LU_MASK (LU_NUM-1)
#define LU_BLOCKS (GBF_BITS / LU_BITS)

#define MAX_STACK_ALLOC 256

static gbf_clmul_t gbf_clmul[LU_NUM*LU_NUM];
static gbf_int_t gbf_clmod[LU_NUM];

void
gbf_init(gbf_int_t polynome)
{
	int i;
	for (i=0; i<LU_NUM; i++)
	{
		int j;
		for (j=0; j<LU_NUM; j++)
		{
			gbf_clmul_t v = 0;
			int _i = i;
			int _j = j;
			while (_i)
			{
				if (_i & 1)
					v ^= _j;
				_i >>= 1;
				_j <<= 1;
			}
			gbf_clmul[i*LU_NUM + j] = v;
		}
	}

	gbf_int_t bits[LU_BITS];
	bits[0] = polynome;
	for (i=1; i<LU_BITS; i++)
	{
		bits[i] = bits[i-1] << 1;
		if (bits[i-1] >> (GBF_BITS - 1))
			bits[i] ^= polynome;
	}

	for (i=0; i<LU_NUM; i++)
	{
		gbf_int_t v = 0;
		int _i = i;
		int bit = 0;
		while (_i)
		{
			if (_i & 1)
				v ^= bits[bit];
			_i >>= 1;
			bit++;
		}
		gbf_clmod[i] = v;
	}
}

gbf_int_t
gbf_mul(gbf_int_t v1, gbf_int_t v2)
{
	gbf_int_t p = 0;

	int bl;
	for (bl=2*(LU_BLOCKS-1); bl>=0; bl--)
	{
		p = (p << LU_BITS) ^ gbf_clmod[p >> (GBF_BITS - LU_BITS)];
		int x;
		gbf_int_t i_x = v1;
		for (x=0; x<LU_BLOCKS; x++)
		{
			int y = bl - x;
			if (y >= 0 && y < LU_BLOCKS)
			{
				gbf_int_t j_y = v2 >> (y*LU_BITS);
				p ^= gbf_clmul[ ((i_x & LU_MASK) * LU_NUM) + (j_y & LU_MASK) ];

			}
			i_x >>= LU_BITS;
		}
	}
	return p;
}

gbf_int_t
gbf_pwr(gbf_int_t v1, gbf_int_t v2)
{
	gbf_int_t p = 1;
	while (v2)
	{
		if (v2 & 1)
			p = gbf_mul(p, v1);
		v2 >>= 1;
		if (v2 == 0)
			return p;
		v1 = gbf_mul(v1, v1);
	}
	return p;
}

/* extended eudclidean algorithm to get the modular multiplicative inverse */
gbf_int_t
gbf_inv(gbf_int_t r)
{
	// remove the special-cases. these cause overflows in our algorithm..
	assert(r != 0); // the inverse of 0 doesn't exist.

	if (r == 1)
		return 1;

	gbf_int_t old_r = gbf_clmod[1];

	gbf_int_t t = 1;
	gbf_int_t old_t = 0;

	/* first iteration has bit 'GBF_BITS' set in old_r; but type gbf_int_t is not large enough */
	int r_hbit;
	for (r_hbit=GBF_BITS-1; r_hbit>=0; r_hbit--)
		if (r >> r_hbit)
			break;

	int bit;
	gbf_int_t quo = 0;
	old_r ^= r << (GBF_BITS - r_hbit);
	quo |= 1 << (GBF_BITS - r_hbit);
	for (bit=GBF_BITS-1; bit>=r_hbit; bit--)
	{
		if (old_r >> bit)
		{
			old_r ^= r << (bit - r_hbit);
			quo |= 1 << (bit - r_hbit);
		}
	}

	old_t ^= gbf_mul(quo, t);

	{ gbf_int_t tmp = old_r; old_r = r; r = tmp; } // swap(r, old_r)
	{ gbf_int_t tmp = old_t; old_t = t; t = tmp; } // swap(t, old_t)

	/* all next iterations do fit in gbf_int_t */
	while (r)
	{
		int r_hbit;
		for (r_hbit=GBF_BITS-1; r_hbit>=0; r_hbit--)
			if (r >> r_hbit)
				break;

		int bit;
		gbf_int_t quo = 0;
		for (bit=GBF_BITS-1; bit>=r_hbit; bit--)
		{
			if (old_r >> bit)
			{
				old_r ^= r << (bit - r_hbit);
				quo |= 1 << (bit - r_hbit);
			}
		}

		old_t ^= gbf_mul(quo, t);

		{ gbf_int_t tmp = old_r; old_r = r; r = tmp; } // swap(r, old_r)
		{ gbf_int_t tmp = old_t; old_t = t; t = tmp; } // swap(t, old_t)
	}

	assert(old_r == 1); // should not happen. something's wrong with our binary field.

	return t ^ old_t;
}

// slower method by using finite group characteristics.
// x^phi == 1, thus x^(phi-1) is the inverse.
gbf_int_t
gbf_inv_phi(gbf_int_t v1)
{
	assert(v1 != 0); // the inverse of 0 doesn't exist.

	return gbf_pwr(v1, -2);
}

void
gbf_invmatrix(gbf_int_t *matrix, int size)
{
	int i;
	for (i=0; i<size; i++)
	{
		gbf_int_t *invrow = &matrix[i*size];
		gbf_int_t inv = gbf_inv(*invrow);
		int j;
		for (j=0; j<size-1; j++)
			invrow[j] = gbf_mul(invrow[j+1], inv);
		invrow[size-1] = inv;
		for (j=0; j<size; j++)
		{
			if (i == j)
				continue;
			gbf_int_t *row = &matrix[j*size];
			gbf_int_t mul = *row;
			int k;
			for (k=0; k<size-1; k++)
				row[k] = row[k+1] ^ gbf_mul(mul, invrow[k]);
			row[size-1] = gbf_mul(mul, invrow[k]);
		}
	}
}

void
gbf_encode_one(gbf_int_t *out, gbf_int_t *data, gbf_int_t vec, int num_frag, int size)
{
	assert(vec != 0); // not allowed to use vec 0.

	gbf_int_t *x;
	if (sizeof(gbf_int_t) * num_frag > MAX_STACK_ALLOC) {
		x = (gbf_int_t *) malloc(sizeof(gbf_int_t) * num_frag);
		assert(x != NULL); // FIXME: have to do this more gracefully
	} else {
		x = (gbf_int_t *) alloca(sizeof(gbf_int_t) * num_frag);
	}

	int i;
	x[0] = 1;
	for (i=1; i<num_frag; i++)
		x[i] = gbf_mul(x[i-1], vec);

	for (i=0; i<size; i++)
	{
		int j;
		gbf_int_t v = 0;
		for (j=0; j<num_frag; j++)
			v ^= gbf_mul(*data++, x[j]);
		*out++ = v;
	}

	if (sizeof(gbf_int_t) * num_frag > MAX_STACK_ALLOC) {
		free(x);
	}
}

void
gbf_decode(gbf_int_t *out, gbf_int_t *data, gbf_int_t *vec, int num_frag, int size)
{
	gbf_int_t *x;
	if (sizeof(gbf_int_t) * num_frag * num_frag > MAX_STACK_ALLOC) {
		x = (gbf_int_t *) malloc(sizeof(gbf_int_t) * num_frag * num_frag);
		assert(x != NULL); // FIXME: have to do this more gracefully
	} else {
		x = (gbf_int_t *) alloca(sizeof(gbf_int_t) * num_frag * num_frag);
	}

	int i;
	for (i=0; i<num_frag; i++)
	{
		x[i*num_frag] = 1;
		int j;
		for (j=1; j<num_frag; j++)
			x[i*num_frag+j] = gbf_mul(x[i*num_frag+j-1], vec[i]);
	}

	gbf_invmatrix(x, num_frag);

	for (i=0; i<num_frag; i++)
	{
		int j;
		for (j=0; j<size; j++)
		{
			int k;
			gbf_int_t v = 0;
			for (k=0; k<num_frag; k++)
				v ^= gbf_mul(data[k*size + j], x[i*num_frag + k]);
			out[j*num_frag + i] = v;
		}
	}

	if (sizeof(gbf_int_t) * num_frag * num_frag > MAX_STACK_ALLOC) {
		free(x);
	}
}
