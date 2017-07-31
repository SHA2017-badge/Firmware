#include "sdkconfig.h"

#include <string.h>

#include <badge_input.h>
#include <badge_eink_dev.h>
#include <badge_eink_lut.h>
#include <badge_eink_fb.h>
#include <badge_eink.h>

void demoDot1(void) {
  uint32_t *tmpbuf = (uint32_t *) badge_eink_fb;

  /* clear screen */
  memset(tmpbuf, 0xff, DISP_SIZE_X_B * DISP_SIZE_Y);

  struct badge_eink_update eink_upd = {
    .lut      = BADGE_EINK_LUT_DEFAULT,
    .reg_0x3a = 26,   // 26 dummy lines per gate
    .reg_0x3b = 0x08, // 62us per line
    .y_start  = 0,
    .y_end    = 295,
  };
  badge_eink_update(tmpbuf, &eink_upd);

  // init LUT
  struct badge_eink_lut_entry lut[] = {
    { .length = 1, .voltages = 0x99, },
    { .length = 0 }
  };

  int px = 100;
  int py = 64;
  int xd = 1;
  int yd = 1;

  uint16_t dots[16] = {
      0,
  };
  int dot_pos = 0;

  while (1) {
    if (badge_input_get_event(10) != 0)
      return;

    /* update dot */
    if (px + xd < 0)
      xd = -xd;
    if (px + xd >= DISP_SIZE_X)
      xd = -xd;
    px += xd;

    if (py + yd < 0)
      yd = -yd;
    if (py + yd >= DISP_SIZE_Y)
      yd = -yd;
    py += yd;

    dots[dot_pos] = (py << 7) | px;
    dot_pos++;
    if (dot_pos == 16)
      dot_pos = 0;

    /* clear screen */
    memset(tmpbuf, 0xff, DISP_SIZE_X_B * DISP_SIZE_Y);

    int i;
    for (i = 0; i < 16; i++) {
      int px = dots[i] & 127;
      int py = dots[i] >> 7;
      tmpbuf[(px >> 5) + py*4] &= ~( 0x80000000 >> (px & 31) );
    }

    struct badge_eink_update eink_upd = {
      .lut      = BADGE_EINK_LUT_CUSTOM,
      .lut_custom = lut,
      .reg_0x3a = 2,  // 2 dummy lines per gate
      .reg_0x3b = 0,  // 30us per line
      .y_start  = 0,
      .y_end    = 295,
    };
    badge_eink_update(tmpbuf, &eink_upd);
  }
}
