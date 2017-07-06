/** @file badge_sdcard.h */
#ifndef BADGE_SDCARD_H
#define BADGE_SDCARD_H

#include <stdbool.h>

/** initialize the sdcard inserted sensor */
extern void badge_sdcard_init(void);

/** report if an sdcard is inserted */
extern bool badge_sdcard_detected(void);

#endif // BADGE_SDCARD_H
