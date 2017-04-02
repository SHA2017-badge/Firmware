#ifndef BADGE_PINS_H
#define BADGE_PINS_H

#include "sdkconfig.h"

#ifdef CONFIG_SHA_BADGE_V1

// Badge revision 0.0.2
#define PIN_NUM_LED          22

#define PIN_NUM_BUTTON_A      0
#define PIN_NUM_BUTTON_B     27
#define PIN_NUM_BUTTON_MID   25
#define PIN_NUM_BUTTON_UP    26
#define PIN_NUM_BUTTON_DOWN  32
#define PIN_NUM_BUTTON_LEFT  33
#define PIN_NUM_BUTTON_RIGHT 35

#define PIN_NUM_EPD_CLK       5
#define PIN_NUM_EPD_MOSI     17
#define PIN_NUM_EPD_CS       18
#define PIN_NUM_EPD_DATA     19
#define PIN_NUM_EPD_RESET    23
#define PIN_NUM_EPD_BUSY     21

#else

// Badge revision 0.1.0
#define PIN_NUM_LEDS         32

#define PIN_NUM_BUTTON_FLASH  0

#define PIN_NUM_EPD_CLK      18
#define PIN_NUM_EPD_MOSI      5
#define PIN_NUM_EPD_CS       19
#define PIN_NUM_EPD_DATA     21
#define PIN_NUM_EPD_RESET    23
#define PIN_NUM_EPD_BUSY     22

#define PIN_NUM_I2C_INT      25
#define PIN_NUM_I2C_DATA     26
#define PIN_NUM_I2C_CLOCK    27

#endif

#endif // BADGE_PINS_H
