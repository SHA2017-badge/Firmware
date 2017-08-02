#include "sdkconfig.h"

#include <badge_input.h>
#include <badge_eink_dev.h>
#include <badge_eink_lut.h>
#include <badge_eink_fb.h>
#include <badge_eink.h>

void demoGreyscale2(void) {
  uint32_t *tmpbuf = (uint32_t *) badge_eink_fb;

  int i;
  for (i = 0; i < 3; i++) {
    /* draw initial pattern */
    {
      uint32_t *dptr = tmpbuf;
      int y;
      for (y = 0; y < DISP_SIZE_Y; y++) {
        *dptr++ = (i & 1) ? 0xffffffff : 0x00000000;
        *dptr++ = (i & 1) ? 0xffffffff : 0x00000000;
        *dptr++ = (i & 1) ? 0x00000000 : 0xffffffff;
        *dptr++ = (i & 1) ? 0x00000000 : 0xffffffff;
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


  for (i = 1; i < 16; i++) {
    /* draw pattern */
    int y_first = 19;
    int y_next = (i + 1) * 19;
    if (y_next > DISP_SIZE_Y)
      y_next = DISP_SIZE_Y;

    uint32_t *dptr = tmpbuf;
    int y;
    for (y = 0; y < y_first; y++) {
      *dptr++ = 0x00000000;
      *dptr++ = 0x00000000;
      *dptr++ = 0xffffffff;
      *dptr++ = 0xffffffff;
    }
    for (y = y_first; y < y_next; y++) {
      *dptr++ = 0xffffffff;
      *dptr++ = 0xffffffff;
      *dptr++ = 0x00000000;
      *dptr++ = 0x00000000;
    }
    for (y = y_next; y < DISP_SIZE_Y; y++) {
      *dptr++ = 0x00000000;
      *dptr++ = 0x00000000;
      *dptr++ = 0xffffffff;
      *dptr++ = 0xffffffff;
    }

    /* update LUT */
    struct badge_eink_lut_entry lut[] = {
      { .length = i, .voltages = 0x18, },
      { .length = 0 }
    };

    struct badge_eink_update eink_upd = {
      .lut      = BADGE_EINK_LUT_CUSTOM,
      .lut_custom = lut,
      .reg_0x3a = 2,    // 2 dummy lines per gate
      .reg_0x3b = 0x00, // 30us per line
      .y_start  = 0,
      .y_end    = 295,
    };
    badge_eink_update(tmpbuf, &eink_upd);
  }

  // wait for random keypress
  badge_input_get_event(-1);
}
