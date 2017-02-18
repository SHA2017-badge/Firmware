#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include <esp_event.h>
#include <gde.h>
#include <gdeh029a1.h>

#include "event_queue.h"

void demoText2(void) {
  /* update LUT */
  writeLUT(LUT_DEFAULT);

  /* draw test pattern */
  setRamArea(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
  setRamPointer(0, 0);
  gdeWriteCommandInit(0x24);
  {
    int x, y;
    for (y = 0; y < DISP_SIZE_Y; y++) {
      for (x = 0; x < 16; x++)
        gdeWriteByte((y & 1) ? 0x55 : 0xaa);
    }
  }
  gdeWriteCommandEnd();

  /* draw text with 16px font */
  const char *line_1 = "esp-idf supports compiling multiple files in parallel, "
                       "so all of the above commands can be run as `make -jN` "
                       "where `N` is the number of parallel make processes to "
                       "run (generally N should be equal to or one more than "
                       "the number of CPU cores in your system.)";

  int pos = 0;
  int row = 14;
  while (line_1[pos]) {
    int num = drawText(row, 16, -16, &line_1[pos],
                       FONT_16PX | FONT_INVERT | FONT_FULL_WIDTH);
    if (num == 0)
      break;
    pos += num;
    row -= 2;
  }
  drawText(row, 16, -16, "", FONT_16PX | FONT_INVERT | FONT_FULL_WIDTH);
  row -= 2;

  drawText(0, 0, 0, " Just a status line. Wifi status: not connected",
           FONT_FULL_WIDTH);

  updateDisplay();
  gdeBusyWait();

  // wait for random keypress
  uint32_t buttons_down = 0;
  while ((buttons_down & 0x7f) == 0)
    xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY);
}
