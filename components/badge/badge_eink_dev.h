/** @file badge_eink_dev.h */
#ifndef BADGE_EINK_DEV_H
#define BADGE_EINK_DEV_H

#include <stdbool.h>
#include <stdint.h>
#include <esp_err.h>

// low-level display, 90 degrees rotated
#define DISP_SIZE_X 128
#define DISP_SIZE_Y 296
#define DISP_SIZE_X_B ((DISP_SIZE_X + 7) >> 3)

/** Initialize the SPI bus to the eink display.
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_eink_dev_init(void);
/** Send reset to the device
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_eink_dev_reset(void);
extern bool badge_eink_dev_is_busy(void);
extern void badge_eink_dev_busy_wait(void);
extern void badge_eink_dev_write_byte(uint8_t data);
extern void badge_eink_dev_write_command(uint8_t command);
extern void badge_eink_dev_write_command_init(uint8_t command);
extern void badge_eink_dev_write_command_end(void);

static inline void badge_eink_dev_write_command_p1(uint8_t command, uint8_t para1)
{
	badge_eink_dev_write_command_init(command);
	badge_eink_dev_write_byte(para1);
	badge_eink_dev_write_command_end();
}

static inline void badge_eink_dev_write_command_p2(uint8_t command, uint8_t para1,
                                      uint8_t para2)
{
	badge_eink_dev_write_command_init(command);
	badge_eink_dev_write_byte(para1);
	badge_eink_dev_write_byte(para2);
	badge_eink_dev_write_command_end();
}

static inline void badge_eink_dev_write_command_p3(uint8_t command, uint8_t para1,
                                      uint8_t para2, uint8_t para3)
{
	badge_eink_dev_write_command_init(command);
	badge_eink_dev_write_byte(para1);
	badge_eink_dev_write_byte(para2);
	badge_eink_dev_write_byte(para3);
	badge_eink_dev_write_command_end();
}

static inline void badge_eink_dev_write_command_p4(uint8_t command, uint8_t para1,
                                      uint8_t para2, uint8_t para3,
                                      uint8_t para4)
{
	badge_eink_dev_write_command_init(command);
	badge_eink_dev_write_byte(para1);
	badge_eink_dev_write_byte(para2);
	badge_eink_dev_write_byte(para3);
	badge_eink_dev_write_byte(para4);
	badge_eink_dev_write_command_end();
}

static inline void badge_eink_dev_write_command_stream(uint8_t command, const uint8_t *data,
                                         unsigned int datalen)
{
	badge_eink_dev_write_command_init(command);
	while (datalen-- > 0) {
		badge_eink_dev_write_byte(*(data++));
	}
	badge_eink_dev_write_command_end();
}

static inline void badge_eink_dev_write_byte_u32(uint32_t data)
{
	badge_eink_dev_write_byte(data >> 24);
	badge_eink_dev_write_byte(data >> 16);
	badge_eink_dev_write_byte(data >> 8);
	badge_eink_dev_write_byte(data);
}

static inline void badge_eink_dev_write_command_stream_u32(uint8_t command, const uint32_t *data,
                                         unsigned int datalen)
{
	badge_eink_dev_write_command_init(command);
	while (datalen-- > 0) {
		badge_eink_dev_write_byte_u32(*(data++));
	}
	badge_eink_dev_write_command_end();
}

#endif // BADGE_EINK_DEV_H
