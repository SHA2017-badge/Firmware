#include "sdkconfig.h"

#ifdef CONFIG_SHA_BADGE_EINK_GDEH029A1
#include <string.h>

#include <badge_input.h>
#include <badge_eink_dev.h>
#include <badge_eink_lut.h>
#include <badge_eink.h>

#include "img_hacking.h"

void demoGreyscaleImg2(void) {
  // curved
  const uint8_t lvl_buf[64] = {
      0,   9,   18,  26,  34,  42,  50,  57,  64,  71,  78,  85,  91,
      97,  103, 109, 115, 120, 126, 131, 136, 141, 145, 150, 154, 159,
      163, 167, 171, 175, 178, 182, 186, 189, 192, 195, 199, 202, 204,
      207, 210, 213, 215, 218, 220, 223, 225, 227, 229, 231, 233, 235,
      237, 239, 241, 243, 244, 246, 248, 249, 251, 252, 254, 255 // e=0.15, n=63
  };

  // trying to get rid of all ghosting and end with a black screen.
  int i;
  for (i = 0; i < 3; i++) {
    /* draw initial pattern */
    badge_eink_set_ram_area(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
    badge_eink_set_ram_pointer(0, 0);
    badge_eink_dev_write_command_init(0x24);
    int c;
    for (c = 0; c < DISP_SIZE_X_B * DISP_SIZE_Y; c++)
      badge_eink_dev_write_byte((i & 1) ? 0xff : 0x00);
    badge_eink_dev_write_command_end();

    struct badge_eink_update eink_upd = {
      .lut      = BADGE_EINK_LUT_DEFAULT,
      .reg_0x3a = 26,   // 26 dummy lines per gate
      .reg_0x3b = 0x08, // 62us per line
      .y_start  = 0,
      .y_end    = 295,
    };
    badge_eink_update(&eink_upd);
  }

  int j;
  for (j = 0; j < 4; j++) {
    int y_start = 0 + j * (DISP_SIZE_Y / 4);
    int y_end = y_start + (DISP_SIZE_Y / 4) - 1;

    for (i = 32; i > 0; i >>= 1) {
      badge_eink_set_ram_area(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
      badge_eink_set_ram_pointer(0, 0);
      badge_eink_dev_write_command_init(0x24);
      int x, y;
      const uint8_t *ptr = img_hacking;
      for (y = 0; y < DISP_SIZE_Y; y++) {
        uint8_t res = 0;
        for (x = 0; x < DISP_SIZE_X; x++) {
          res <<= 1;
          uint8_t pixel = *ptr++;
          int j;
          for (j = 0; pixel > lvl_buf[j]; j++)
            ;
          if (y >= y_start && y <= y_end && j & i)
            res++;
          if ((x & 7) == 7)
            badge_eink_dev_write_byte(res);
        }
      }
      badge_eink_dev_write_command_end();

      // LUT:
      //   Ignore old state;
      //   Do nothing when bit is not set;
      //   Make pixel whiter when bit is set;
      //   Duration is <i> cycles.
	  struct badge_eink_lut_entry lut[] = {
		  { .length = i, .voltages = 0x88, },
		  { .length = 0 }
	  };

      struct badge_eink_update eink_upd = {
        .lut      = BADGE_EINK_LUT_CUSTOM,
        .lut_custom = lut,
        .reg_0x3a = 0,  // no dummy lines per gate
        .reg_0x3b = 0,  // 30us per line
        .y_start  = y_start,
        .y_end    = y_end + 1,
      };
      badge_eink_update(&eink_upd);
    }
  }

  // wait for random keypress
  badge_input_get_event(-1);
}

#endif // CONFIG_SHA_BADGE_EINK_GDEH029A1
