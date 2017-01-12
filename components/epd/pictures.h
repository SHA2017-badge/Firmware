#ifndef EPD_PICTURES_H
#define EPD_PICTURES_H

#include <stdint.h>

// GDE029
const uint8_t gImage_sha[0] = {
};

const uint8_t gImage_menu[0] = {
};

const uint8_t gImage_nick[0] = {
};

const uint8_t gImage_weather[0] = {
};

const uint8_t gImage_test[0] = {
};

#define NUM_PICTURES 5

const uint8_t *pictures[NUM_PICTURES] = {
  gImage_sha,
  gImage_menu,
  gImage_nick,
  gImage_weather,
  gImage_test,
};

#endif
