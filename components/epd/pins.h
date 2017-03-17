#ifndef EPD_PINS_H
#define EPD_PINS_H

#include "sdkconfig.h"

#ifdef CONFIG_SHA_BADGE_V1
#define PIN_NUM_CLK 5
#define PIN_NUM_MOSI 17
#define PIN_NUM_CS 18
#define PIN_NUM_DATA 19
#define PIN_NUM_RESET 23
// PIN_NUM_RESET: hardware: patch 20 to 23
#define PIN_NUM_BUSY 21

#else
#define PIN_NUM_CLK 18
#define PIN_NUM_MOSI 5
#define PIN_NUM_CS 19
#define PIN_NUM_DATA 21
#define PIN_NUM_RESET 23
#define PIN_NUM_BUSY 22
#endif

#endif
