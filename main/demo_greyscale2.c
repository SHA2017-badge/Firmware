#include "sdkconfig.h"

#ifdef CONFIG_SHA_BADGE_EINK_GDEH029A1
#include <freertos/FreeRTOS.h>
#include <esp_event.h>

#include <badge_input.h>
#include <badge_eink.h>
#include <badge_eink_dev.h>

void demoGreyscale2(void) {
  int i;
  for (i = 0; i < 3; i++) {
    /* draw initial pattern */
    badge_eink_set_ram_area(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
    badge_eink_set_ram_pointer(0, 0);
    gdeWriteCommandInit(0x24);
    {
      int x, y;
      for (y = 0; y < DISP_SIZE_Y; y++) {
        for (x = 0; x < 8; x++)
          gdeWriteByte((i & 1) ? 0xff : 0x00);
        for (x = 8; x < 16; x++)
          gdeWriteByte((i & 1) ? 0x00 : 0xff);
      }
    }
    gdeWriteCommandEnd();

    struct badge_eink_update eink_upd = {
      .lut      = BADGE_EINK_LUT_DEFAULT,
      .reg_0x3a = 26,   // 26 dummy lines per gate
      .reg_0x3b = 0x08, // 62us per line
      .y_start  = 0,
      .y_end    = 295,
    };
    badge_eink_update(&eink_upd);
  }


  for (i = 1; i < 16; i++) {
    /* draw pattern */
    int y_first = 19;
    int y_next = (i + 1) * 19;
    if (y_next > DISP_SIZE_Y)
      y_next = DISP_SIZE_Y;

    badge_eink_set_ram_area(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
    badge_eink_set_ram_pointer(0, 0);
    gdeWriteCommandInit(0x24);
    int x, y;
    for (y = 0; y < y_first; y++) {
      for (x = 0; x < 8; x++)
        gdeWriteByte(0x00);
      for (x = 8; x < 16; x++)
        gdeWriteByte(0xff);
    }
    for (y = y_first; y < y_next; y++) {
      for (x = 0; x < 8; x++)
        gdeWriteByte(0xff);
      for (x = 8; x < 16; x++)
        gdeWriteByte(0x00);
    }
    for (y = y_next; y < DISP_SIZE_Y; y++) {
      for (x = 0; x < 8; x++)
        gdeWriteByte(0x00);
      for (x = 8; x < 16; x++)
        gdeWriteByte(0xff);
    }
    gdeWriteCommandEnd();

    /* update LUT */
    uint8_t lut[30] = {
        0x18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,    0, 0, 0, 0, i, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    struct badge_eink_update eink_upd = {
      .lut      = BADGE_EINK_LUT_CUSTOM,
      .lut_custom = lut,
      .reg_0x3a = 2,    // 2 dummy lines per gate
      .reg_0x3b = 0x00, // 30us per line
      .y_start  = 0,
      .y_end    = 295,
    };
    badge_eink_update(&eink_upd);
  }

  // wait for random keypress
  uint32_t buttons_down = 0;
  while ((buttons_down & 0xffff) == 0)
    xQueueReceive(badge_input_queue, &buttons_down, portMAX_DELAY);
}

#endif // CONFIG_SHA_BADGE_EINK_GDEH029A1
