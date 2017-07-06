/** @file badge_gpiobutton.h */
#ifndef BADGE_GPIOBUTTON_H
#define BADGE_GPIOBUTTON_H

#include <stdint.h>

/** configure gpio pin as button */
extern void badge_gpiobutton_add(int gpio_num, uint32_t button_id);

#endif // BADGE_GPIOBUTTON_H

