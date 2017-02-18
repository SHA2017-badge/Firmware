#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include <esp_event.h>
#include <gde.h>
#include <gdeh029a1.h>

#include "event_queue.h"

void demoDot1(void) {
  /* update LUT */
  writeLUT(LUT_DEFAULT);

  /* clear screen */
  setRamArea(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
  setRamPointer(0, 0);
  gdeWriteCommandInit(0x24);
  {
    int x, y;
    for (y = 0; y < DISP_SIZE_Y; y++) {
      for (x = 0; x < 16; x++)
        gdeWriteByte(0xff);
    }
  }
  gdeWriteCommandEnd();

  /* update display */
  updateDisplay();
  gdeBusyWait();

  // init LUT
  static const uint8_t lut[30] = {
      //		0x18,0,0,0,0,0,0,0,0,0,
      0, 0x99, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0,    0, 0, 0, 17, 1, 0, 0, 0, 0, 0, 0, 0, 0,
  };
  gdeWriteCommandStream(0x32, lut, 30);

  // tweak timing a bit..
  gdeWriteCommand_p1(0x3a, 0x02); // 2 dummy lines per gate
  gdeWriteCommand_p1(0x3b, 0x00); // 30us per line

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
    if (xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY))
      if ((buttons_down & 0x7f) != 0)
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
    setRamArea(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
    setRamPointer(0, 0);
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
      setRamPointer(px >> 3, py);
      gdeWriteCommand_p1(0x24, 0xff ^ (128 >> (px & 7)));
    }

    /* update display */
    updateDisplay();
    gdeBusyWait();
  }
}
