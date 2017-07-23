/** @file badge_leds.h */
#ifndef BADGE_LEDS_H
#define BADGE_LEDS_H

#include <stdint.h>
#include <esp_err.h>

/**
 * Initialize the leds driver. (configure SPI bus and GPIO pins)
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_leds_init(void);

/**
 * Enable power to the leds bar.
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_leds_enable(void);

/**
 * Disable power to the leds bar.
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_leds_disable(void);

/**
 * Send color-data to the leds bus.
 * @param data the data-bytes to send on the bus.
 * @param len the data-length.
 * @note The first 6 leds on the bus are probably SK6812RGBW leds.
 *   SK6812RGBW expects 4 bytes per led: green, red, blue and white.
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_leds_send_data(uint8_t *data, int len);

#endif // BADGE_LEDS_H
