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

#endif // BADGE_POWER_H
