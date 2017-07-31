#include "sdkconfig.h"

#include <string.h>

#include <badge_input.h>
#include <badge_eink_dev.h>
#include <badge_eink_lut.h>
#include <badge_eink_fb.h>
#include <badge_eink.h>

#include "img_hacking.h"

/*
 * The greyscale curve was generated with:
 *
 * perl -MPOSIX -we 'my $e = 0.15; my $n=15; for (my $i=0; $i<=$n; $i++) { print
 * (POSIX::round(255 / (1-$e) - ($e**($i/$n)) * 255 / (1-$e)),"\n") }' | xargs |
 * sed 's/ /,/g'
 */
void demoGreyscaleImg1(void) {
  uint32_t *tmpbuf = (uint32_t *) badge_eink_fb;

  // trying to get rid of all ghosting and end with a black screen.
  int i;
  for (i = 0; i < 3; i++) {
    /* draw initial pattern */
    memset(tmpbuf, (i & 1) ? 0xff : 0x00, DISP_SIZE_X_B * DISP_SIZE_Y);

    struct badge_eink_update eink_upd = {
      .lut      = BADGE_EINK_LUT_DEFAULT,
      .reg_0x3a = 26,   // 26 dummy lines per gate
      .reg_0x3b = 0x08, // 62us per line
      .y_start  = 0,
      .y_end    = 295,
    };
    badge_eink_update(tmpbuf, &eink_upd);
  }

  for (i = 0; i < 16; i++) {
    // curved
    const uint8_t lvl_buf[16] = {
        //  0,32,61,87,110,131,150,167,182,196,209,220,230,239,248,255
        //// 0.21
        0,   36,  67,  95,  119, 141, 160, 176,
        191, 204, 215, 225, 234, 242, 249, 255 // 0.15
    };
    uint8_t lvl = lvl_buf[i];
    int x, y;
    const uint8_t *ptr = img_hacking;
    uint32_t *dptr = tmpbuf;
    for (y = 0; y < DISP_SIZE_Y; y++) {
      uint32_t res = 0;
      for (x = 0; x < DISP_SIZE_X; x++) {
        res <<= 1;
        if (*ptr++ <= lvl)
          res++;
        if ((x & 31) == 31)
          *dptr++ = res;
      }
    }

    /* update LUT */
    struct badge_eink_lut_entry lut[] = {
      { .length = i, .voltages = 0x18, },
      { .length = 0 }
    };

    struct badge_eink_update eink_upd = {
      .lut      = BADGE_EINK_LUT_CUSTOM,
      .lut_custom = lut,
      .reg_0x3a = 0,  // no dummy lines per gate
      .reg_0x3b = 0,  // 30us per line
      .y_start  = 0,
      .y_end    = 295,
    };
    badge_eink_update(tmpbuf, &eink_upd);
  }

  // wait for random keypress
  badge_input_get_event(-1);
}
