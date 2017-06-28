/** @file badge_power.h */
#ifndef BADGE_POWER_H
#define BADGE_POWER_H

#include <stdbool.h>

/**
 * initializes the battery and usb power sensing
 */
extern void badge_power_init(void);

/**
 * returns the charging state.
 *
 * @return true if charging; false otherwise.
 */
extern bool badge_battery_charge_status(void);

/**
 * returns the Vbat voltage.
 *
 * @return Vbat voltage in mV; -1 on error
 */
extern int badge_battery_volt_sense(void);

/**
 * returns the Vusb voltage.
 *
 * @return Vusb voltage in mV; -1 on error
 */
extern int badge_usb_volt_sense(void);

/**
 * enable power to the leds-bar
 *
 * @return 0 on ok, -1 on error
 */
extern int badge_power_leds_enable(void);

/**
 * disable power to the leds-bar
 *
 * @return 0 on ok, -1 on error
 * @note shared resource: if the power to the sd-card is also enabled,
 *   the power will stay on.
 */
extern int badge_power_leds_disable(void);

/**
 * enable power to the sd-card
 *
 * @return 0 on ok, -1 on error
 */
extern int badge_power_sdcard_enable(void);

/**
 * disable power to the sd-card
 *
 * @return 0 on ok, -1 on error
 * @note shared resource: if the power to the leds-bar is also enabled,
 *   the power will stay on.
 */
extern int badge_power_sdcard_disable(void);

#endif // BADGE_POWER_H
