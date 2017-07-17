#ifndef REDUNDANCY_H
#define REDUNDANCY_H

#include <stdint.h>

// define the size of our finite field (galois binary field)
#define GBF_BITS 16

/* used word-size and known-good polynomes */
#if GBF_BITS == 32
 typedef uint32_t gbf_int_t;
 // polynome: x^32 + x^7 + x^3 + x^2 + 1
 #define GBF_POLYNOME 0x8d

#elif GBF_BITS == 16
 typedef uint16_t gbf_int_t;
 // polynome: x^16 + x^5 + x^3 + x + 1
 #define GBF_POLYNOME 0x2b

#elif GBF_BITS == 8
 typedef uint8_t gbf_int_t;
 // polynome: x^8 + x^4 + x^3 + x + 1
 #define GBF_POLYNOME 0x1b
#endif

/* gbf_init(polynome);
 *   Library needs to be initialized with a specific polynome.
 *
 *   To initialize the library with the default known-good polynome, use:
 *     gbf_init(GBF_POLYNOME);
 */
extern void gbf_init(gbf_int_t polynome);


/***************************************************************************
** basic operations on a word;                                            **
** no need to call them if you only use gbf_encode_one() and gbf_decode() **
***************************************************************************/

/* gbf_mul(v1, v2)
 *   Multiply two values with each other. (modulo given polynome)
 */
extern gbf_int_t gbf_mul(gbf_int_t v1, gbf_int_t v2);

/* gbf_prw(v1, v2)
 *   v1 to the power of v2. (modulo given polynome)
 */
extern gbf_int_t gbf_pwr(gbf_int_t v1, gbf_int_t v2);

/* gbf_inv(v1)
 *   The multiplicative inverse of v1. (modulo given polynome)
 */
extern gbf_int_t gbf_inv(gbf_int_t r);
extern gbf_int_t gbf_inv_phi(gbf_int_t v1); // slower, alternative method

/* gbf_invmatrix(matrix[size*size], size)
 *   Calculate the inverse matrix.
 */
extern void gbf_invmatrix(gbf_int_t *matrix, int size);


/***************************************************************************
** encode and decode methods                                              **
***************************************************************************/

/* gbf_encode_one(out[size], data[num_frag*size], vec, num_frag, size)
 *   Encode one packet. At least <num_frag> packets are needed to decode
 *   the data. <vec> should be unique and is not allowed to be 0.
 */
extern void gbf_encode_one(gbf_int_t *out, gbf_int_t *data, gbf_int_t vec, int num_frag, int size);

/* gbf_decode(out[num_frag*size], data[num_frag*size], vec[num_frag], num_frag, size)
 *   Decode the original data. <data> contains all concatenated fragments.
 *   <vec> contains all used vector-values (should be in the same order as
 *   the fragments).
 */
extern void gbf_decode(gbf_int_t *out, gbf_int_t *data, gbf_int_t *vec, int num_frag, int size);

#endif // REDUNDANCY_H
