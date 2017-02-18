#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include <esp_event.h>
#include <gde.h>
#include <gdeh029a1.h>
#include <pictures.h>

#include "event_queue.h"

void demoPartialUpdate(void) {
  /* update LUT */
  writeLUT(LUT_DEFAULT);

  // tweak timing a bit.. (FIXME)
  gdeWriteCommand_p1(0x3a, 0x3f); // 63 dummy lines per gate
  gdeWriteCommand_p1(0x3b, 0x0f); // 208us per line

  int i;
  for (i = 0; i < 8; i++) {
    int j = ((i << 1) | (i >> 2)) & 7;
    drawImage(gImage_sha);
    updateDisplayPartial(37 * j, 37 * j + 36);
    gdeBusyWait();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  for (i = 0; i < 8; i++) {
    int j = ((i << 1) | (i >> 2)) & 7;
    drawImage(gImage_nick);
    updateDisplayPartial(37 * j, 37 * j + 36);
    gdeBusyWait();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
  gdeWriteCommand_p1(0x3b, 0x08); // 62us per line

  // wait for random keypress
  uint32_t buttons_down = 0;
  while ((buttons_down & 0x7f) == 0)
    xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY);
}
