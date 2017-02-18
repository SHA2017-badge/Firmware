#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include <esp_event.h>
#include <gde.h>
#include <gdeh029a1.h>

#include "event_queue.h"

void demoGreyscale2(void) {
  /* update LUT */
  writeLUT(LUT_DEFAULT);

  int i;
  for (i = 0; i < 3; i++) {
    /* draw initial pattern */
    setRamArea(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
    setRamPointer(0, 0);
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

    /* update display */
    updateDisplay();
    gdeBusyWait();
  }

  gdeWriteCommand_p1(0x3a, 0x02); // 2 dummy lines per gate
  //	gdeWriteCommand_p1(0x3b, 0x08); // 62us per line
  gdeWriteCommand_p1(0x3b, 0x00); // 30us per line

  for (i = 1; i < 16; i++) {
    /* draw pattern */
    int y_first = 19;
    int y_next = (i + 1) * 19;
    if (y_next > DISP_SIZE_Y)
      y_next = DISP_SIZE_Y;

    setRamArea(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
    setRamPointer(0, 0);
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
    gdeWriteCommandStream(0x32, lut, 30);

    /* update display */
    updateDisplay();
    gdeBusyWait();
  }

  gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
  gdeWriteCommand_p1(0x3b, 0x08); // 62us per line

  // wait for random keypress
  uint32_t buttons_down = 0;
  while ((buttons_down & 0x7f) == 0)
    xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY);
}
