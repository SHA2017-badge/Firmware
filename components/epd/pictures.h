#ifndef EPD_PICTURES_H
#define EPD_PICTURES_H

#include <stdint.h>

#define NUM_PICTURES 5

extern const uint8_t gImage_sha[4736];
extern const uint8_t gImage_menu[4736];
extern const uint8_t gImage_nick[4736];
extern const uint8_t gImage_weather[4736];
extern const uint8_t gImage_test[4736];

extern const uint8_t *pictures[NUM_PICTURES];

#endif
