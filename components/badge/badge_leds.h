/** @file badge_leds.h */
#ifndef BADGE_LEDS_H
#define BADGE_LEDS_H

#include <stdint.h>

extern void badge_leds_init(void);

extern int badge_leds_enable(void);
extern int badge_leds_disable(void);

extern int badge_leds_set_state(uint8_t *rgbw);
extern int badge_leds_send_data(uint8_t *data, int len);

#endif // BADGE_LEDS_H
