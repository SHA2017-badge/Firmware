#include "sdkconfig.h"

#ifdef CONFIG_SHA_BADGE_EINK_GDEH029A1
#include <freertos/FreeRTOS.h>
#include <esp_event.h>

#include <badge_input.h>
#include <badge_eink.h>
#include <badge_eink_dev.h>

void demoDot1(void) {
  /* clear screen */
  badge_eink_set_ram_area(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
  badge_eink_set_ram_pointer(0, 0);
  gdeWriteCommandInit(0x24);
  {
    int x, y;
    for (y = 0; y < DISP_SIZE_Y; y++) {
      for (x = 0; x < 16; x++)
        gdeWriteByte(0xff);
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

  // init LUT
  static const uint8_t lut[30] = {
      //		0x18,0,0,0,0,0,0,0,0,0,
      0, 0x99, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0,    0, 0, 0, 17, 1, 0, 0, 0, 0, 0, 0, 0, 0,
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
    uint32_t buttons_down = 0;
    if (xQueueReceive(badge_input_queue, &buttons_down, portMAX_DELAY))
      if ((buttons_down & 0xffff) != 0)
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
    badge_eink_set_ram_area(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
    badge_eink_set_ram_pointer(0, 0);
    gdeWriteCommandInit(0x24);
    {
      int x, y;
      for (y = 0; y < DISP_SIZE_Y; y++) {
        for (x = 0; x < 16; x++)
          gdeWriteByte(0xff);
      }
    }
    gdeWriteCommandEnd();

    int i;
    for (i = 0; i < 16; i++) {
      int px = dots[i] & 127;
      int py = dots[i] >> 7;
      badge_eink_set_ram_pointer(px >> 3, py);
      gdeWriteCommand_p1(0x24, 0xff ^ (128 >> (px & 7)));
    }

    struct badge_eink_update eink_upd = {
      .lut      = BADGE_EINK_LUT_CUSTOM,
      .lut_custom = lut,
      .reg_0x3a = 2,  // 2 dummy lines per gate
      .reg_0x3b = 0,  // 30us per line
      .y_start  = 0,
      .y_end    = 295,
    };
    badge_eink_update(&eink_upd);
  }
}

#endif // CONFIG_SHA_BADGE_EINK_GDEH029A1
