#ifndef BADGE_POWER_H
#define BADGE_POWER_H

extern void badge_power_init(void);
extern bool badge_battery_charge_status(void);

// returns Vbat voltage in mV
extern int badge_battery_volt_sense(void);

// returns Vusb voltage in mV
extern int badge_usb_volt_sense(void);

#endif // BADGE_POWER_H
