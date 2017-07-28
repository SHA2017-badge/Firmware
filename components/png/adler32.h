/** @file adler32.h */
#ifndef LIB_ADLER32_H
#define LIB_ADLER32_H

#include <stdint.h>
#include <stddef.h>

/** initial value for lib_adler32 */
#define LIB_ADLER32_INIT 1

/**
 * Calculate adler32 checksum over data-block
 * @param buf data-block
 * @param buf_len the length of the data-block
 * @param adler the initial adler32 checksum
 * @return the updated adler32 checksum
 */
extern uint32_t lib_adler32(const uint8_t *buf, size_t buf_len, uint32_t adler);

#endif // LIB_ADLER32_H
