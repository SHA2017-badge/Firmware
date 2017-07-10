/** @file badge_i2c.h */
#ifndef BADGE_I2C_H
#define BADGE_I2C_H

#include <stdint.h>
#include <esp_err.h>

/** initialize i2c bus */
extern void badge_i2c_init(void);

/** read register via i2c bus */
extern esp_err_t badge_i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *value, size_t value_len);

/** write to register via i2c bus */
extern esp_err_t badge_i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value);

/** read event via i2c bus */
extern esp_err_t badge_i2c_read_event(uint8_t addr, uint8_t *buf);

#endif // BADGE_I2C_H
