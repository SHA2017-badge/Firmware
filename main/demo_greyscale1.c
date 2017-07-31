#include "sdkconfig.h"

#include <string.h>

#include <esp_event.h>

#include <badge_input.h>
#include <badge_eink.h>
#include <badge_eink_dev.h>
#include <badge_eink_fb.h>

void demoGreyscale1(void) {
  uint32_t *tmpbuf = (uint32_t *) badge_eink_fb;

  int i;
  for (i = 0; i < 2; i++) {
    /* draw test pattern */
    {
      uint32_t *dptr = tmpbuf;
      int x, y;
      for (y = 0; y < DISP_SIZE_Y; y++) {
        for (x = 0; x < 4; x++)
          *dptr++ = (x & 1) ? 0xffffffff : 0x00000000;
      }
    }

    struct badge_eink_update eink_upd = {
      .lut      = BADGE_EINK_LUT_DEFAULT,
      .reg_0x3a = 26,   // 26 dummy lines per gate
      .reg_0x3b = 0x08, // 62us per line
      .y_start  = 0,
      .y_end    = 295,
    };
    badge_eink_update(tmpbuf, &eink_upd);
  }

  int y = 0;
  int n = 8;
  while (y < DISP_SIZE_Y) {
    /* draw new test pattern */
    {
      uint32_t *dptr = tmpbuf;
      int x, y;
      for (y = 0; y < DISP_SIZE_Y; y++) {
        for (x = 0; x < 4; x++)
          *dptr++ = 0xffff0000;
      }
    }

    if (y + n > DISP_SIZE_Y)
      n = DISP_SIZE_Y - y;

    struct badge_eink_update eink_upd = {
      .lut      = BADGE_EINK_LUT_DEFAULT,
      .reg_0x3a = 26,   // 26 dummy lines per gate
      .reg_0x3b = 0x08, // 62us per line
      .y_start  = y,
      .y_end    = y+n,
    };
    badge_eink_update(tmpbuf, &eink_upd);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    y += n;
    n += 4;
  }
  y = 0;
  n = 4;
  while (y < DISP_SIZE_Y) {
    /* draw new test pattern */
    {
      uint32_t *dptr = tmpbuf;
      int x, y;
      for (y = 0; y < DISP_SIZE_Y; y++) {
        for (x = 0; x < 4; x++)
          *dptr++ = (x & 2) ? 0xffffffff : 0x00000000;
      }
    }

    if (y + n > DISP_SIZE_Y)
      n = DISP_SIZE_Y - y;

    struct badge_eink_update eink_upd = {
      .lut      = BADGE_EINK_LUT_DEFAULT,
      .reg_0x3a = 26,   // 26 dummy lines per gate
      .reg_0x3b = 0x08, // 62us per line
      .y_start  = y,
      .y_end    = y+n,
    };
    badge_eink_update(tmpbuf, &eink_upd);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    y += n;
    n += 2;
  }

  // wait for random keypress
  badge_input_get_event(-1);
}
