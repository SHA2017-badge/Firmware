/** @file crc32.h */
#ifndef LIB_CRC32_H
#define LIB_CRC32_H

#include <stdint.h>
#include <stddef.h>

/** initial value for lib_crc32 */
#define LIB_CRC32_INIT 0

/**
 * Calculate crc32 checksum over data-block
 * @param buf data-block
 * @param buf_len the length of the data-block
 * @param crc the initial crc32 checksum
 * @return the updated crc32 checksum
 */
extern uint32_t lib_crc32(const uint8_t *buf, size_t buf_len, uint32_t crc);

#endif // LIB_CRC32_H
