#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include <esp_event.h>
#include <gde.h>
#include <gdeh029a1.h>

#include "event_queue.h"
#include "img_hacking.h"

/*
 * The greyscale curve was generated with:
 *
 * perl -MPOSIX -we 'my $e = 0.15; my $n=15; for (my $i=0; $i<=$n; $i++) { print
 * (POSIX::round(255 / (1-$e) - ($e**($i/$n)) * 255 / (1-$e)),"\n") }' | xargs |
 * sed 's/ /,/g'
 */
void demoGreyscaleImg1(void) {
  /* update LUT */
  writeLUT(LUT_DEFAULT);
  gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
  gdeWriteCommand_p1(0x3b, 0x08); // 62us per line

  // trying to get rid of all ghosting and end with a black screen.
  int i;
  for (i = 0; i < 3; i++) {
    /* draw initial pattern */
    setRamArea(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
    setRamPointer(0, 0);
    gdeWriteCommandInit(0x24);
    int c;
    for (c = 0; c < DISP_SIZE_X_B * DISP_SIZE_Y; c++)
      gdeWriteByte((i & 1) ? 0xff : 0x00);
    gdeWriteCommandEnd();

    /* update display */
    updateDisplay();
    gdeBusyWait();
  }

  gdeWriteCommand_p1(0x3a, 0x00); // no dummy lines per gate
  gdeWriteCommand_p1(0x3b, 0x00); // 30us per line

  for (i = 0; i < 16; i++) {
    //		// linear
    //		uint8_t lvl = 17 * i;

    // curved
    const uint8_t lvl_buf[16] = {
        //			0,32,61,87,110,131,150,167,182,196,209,220,230,239,248,255
        //// 0.21
        0,   36,  67,  95,  119, 141, 160, 176,
        191, 204, 215, 225, 234, 242, 249, 255 // 0.15
    };
    uint8_t lvl = lvl_buf[i];

    setRamArea(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
    setRamPointer(0, 0);
    gdeWriteCommandInit(0x24);
    int x, y;
    const uint8_t *ptr = img_hacking;
    for (y = 0; y < DISP_SIZE_Y; y++) {
      uint8_t res = 0;
      for (x = 0; x < DISP_SIZE_X; x++) {
        res <<= 1;
        if (*ptr++ <= lvl)
          res++;
        if ((x & 7) == 7)
          gdeWriteByte(res);
      }
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
