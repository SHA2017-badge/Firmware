#include "sdkconfig.h"

#ifdef CONFIG_SHA_BADGE_EINK_GDEH029A1
#include <freertos/FreeRTOS.h>
#include <esp_event.h>

#include <gde.h>
#include <gde-driver.h>

#include <badge_input.h>

void demoGreyscale1(void) {
  /* update LUT */
  writeLUT(LUT_DEFAULT);

  gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
  gdeWriteCommand_p1(0x3b, 0x08); // 62us per line

  int i;
  for (i = 0; i < 2; i++) {
    /* draw test pattern */
    setRamArea(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
    setRamPointer(0, 0);
    gdeWriteCommandInit(0x24);
    {
      int x, y;
      for (y = 0; y < DISP_SIZE_Y; y++) {
        for (x = 0; x < 16; x++)
          gdeWriteByte(x & 4 ? 0xff : 0x00);
      }
    }
    gdeWriteCommandEnd();

    updateDisplay();
    gdeBusyWait();
  }

  int y = 0;
  int n = 8;
  while (y < DISP_SIZE_Y) {
    /* draw new test pattern */
    setRamArea(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
    setRamPointer(0, 0);
    gdeWriteCommandInit(0x24);
    {
      int x, y;
      for (y = 0; y < DISP_SIZE_Y; y++) {
        for (x = 0; x < 16; x++)
          gdeWriteByte(x & 2 ? 0xff : 0x00);
      }
    }
    gdeWriteCommandEnd();

    if (y + n > DISP_SIZE_Y)
      n = DISP_SIZE_Y - y;
    updateDisplayPartial(y, y + n);
    gdeBusyWait();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    y += n;
    n += 4;
  }
  y = 0;
  n = 4;
  while (y < DISP_SIZE_Y) {
    /* draw new test pattern */
    setRamArea(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
    setRamPointer(0, 0);
    gdeWriteCommandInit(0x24);
    {
      int x, y;
      for (y = 0; y < DISP_SIZE_Y; y++) {
        for (x = 0; x < 16; x++)
          gdeWriteByte(x & 8 ? 0xff : 0x00);
      }
    }
    gdeWriteCommandEnd();

    if (y + n > DISP_SIZE_Y)
      n = DISP_SIZE_Y - y;
    updateDisplayPartial(y, y + n);
    gdeBusyWait();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    y += n;
    n += 2;
  }

  // wait for random keypress
  uint32_t buttons_down = 0;
  while ((buttons_down & 0xffff) == 0)
    xQueueReceive(badge_input_queue, &buttons_down, portMAX_DELAY);
}

#endif // CONFIG_SHA_BADGE_EINK_GDEH029A1
