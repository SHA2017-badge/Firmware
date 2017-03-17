#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include <esp_event.h>
#include <gde.h>
#include <gdeh029a1.h>

#include "event_queue.h"
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

  int j;
  for (j = 0; j < 4; j++) {
    int y_start = 0 + j * (DISP_SIZE_Y / 4);
    int y_end = y_start + (DISP_SIZE_Y / 4) - 1;

    for (i = 32; i > 0; i >>= 1) {
      setRamArea(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
      setRamPointer(0, 0);
      gdeWriteCommandInit(0x24);
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
            gdeWriteByte(res);
        }
      }
      gdeWriteCommandEnd();

      // LUT:
      //   Ignore old state;
      //   Do nothing when bit is not set;
      //   Make pixel whiter when bit is set;
      //   Duration is <i> cycles.
      if (i <= 15) {
        uint8_t lut[30] = {
            0x88, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0,    0, 0, 0, 0, i, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        };
        gdeWriteCommandStream(0x32, lut, 30);
      } else if (i <= 30) {
        uint8_t lut[30] = {
            0x88, 0x88, 0, 0, 0, 0, 0,
            0,    0,    0, 0, 0, 0, 0,
            0,    0,    0, 0, 0, 0, ((i - 15) << 4) + 15,
            0,    0,    0, 0, 0, 0, 0,
            0,    0,
        };
        gdeWriteCommandStream(0x32, lut, 30);
      } else if (i <= 45) {
        uint8_t lut[30] = {
            0x88, 0x88, 0x88, 0, 0, 0,    0,        0, 0, 0, 0, 0, 0, 0, 0,
            0,    0,    0,    0, 0, 0xff, (i - 30), 0, 0, 0, 0, 0, 0, 0, 0,
        };
        gdeWriteCommandStream(0x32, lut, 30);
      } else if (i <= 60) {
        uint8_t lut[30] = {
            0x88, 0x88, 0x88, 0x88, 0, 0, 0, 0, 0, 0,    0,
            0,    0,    0,    0,    0, 0, 0, 0, 0, 0xff, ((i - 45) << 4) + 15,
            0,    0,    0,    0,    0, 0, 0, 0,
        };
        gdeWriteCommandStream(0x32, lut, 30);
      }

      /* update display */
      updateDisplayPartial(y_start, y_end + 1);
      gdeBusyWait();
    }
  }

  gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
  gdeWriteCommand_p1(0x3b, 0x08); // 62us per line

  // wait for random keypress
  uint32_t buttons_down = 0;
  while ((buttons_down & 0x7f) == 0)
    xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY);
}
