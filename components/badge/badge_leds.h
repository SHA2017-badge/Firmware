/** @file badge_leds.h */
#ifndef BADGE_LEDS_H
#define BADGE_LEDS_H

#include <stdint.h>

/**
 * Initialize the leds driver. (configure SPI bus and GPIO pins)
 */
extern void badge_leds_init(void);

/**
 * Enable power to the leds bar.
 * @return 0 on succes; -1 on error.
 */
extern int badge_leds_enable(void);

/**
 * Disable power to the leds bar.
 * @return 0 on succes; -1 on error.
 */
extern int badge_leds_disable(void);

/**
 * Configure the 6 on-board leds with the given rgbw data.
 * @param rgbw array with red, green, blue and white data for every
 *   led. First index is the left led.
 * @return 0 on succes; -1 on error.
 * @deprecated use badge_leds_send_data() instead.
 */
extern int badge_leds_set_state(uint8_t *rgbw) __attribute__((deprecated));

/**
 * Send color-data to the leds bus.
 * @param data the data-bytes to send on the bus.
 * @param len the data-length.
 * @note The first 6 leds on the bus are probably SK6812 leds.
 *   SK6812 expects 4 bytes per led: red, green, blue and white.
 * @return 0 on succes; -1 on error.
 */
extern int badge_leds_send_data(uint8_t *data, int len);

#endif // BADGE_LEDS_H
