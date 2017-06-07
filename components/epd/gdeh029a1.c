#include "sdkconfig.h"

#ifdef CONFIG_SHA_BADGE_EINK_GDEH029A1

#include <stdbool.h>
#include <stdint.h>

#include "gde.h"
#include "gde-driver.h"

// GDEH029A1
// SSD1608

void initDisplay(void) {
  gdeReset();

  // 01: driver output control
  gdeWriteCommand_p3(0x01, (DISP_SIZE_Y - 1) & 0xff, (DISP_SIZE_Y - 1) >> 8,
                     0x00); // DISP_SIZE_Y lines, no interlacing
  // 03: gate driving voltage control (VGH/VGL)
  //	gdeWriteCommand_p1(0x03, 0xea); // +22V/-20V (POR)
  //	gdeWriteCommand_p1(0x03, 0x00); // +15V/-15V (in original source, but
  //not used)
  // 04: source driving voltage control (VSH/VSL)
  //	gdeWriteCommand_p1(0x04, 0x0a); // +15V/-15V (POR) (in original source,
  //but not used)
  // 0C: booster soft start control
  gdeWriteCommand_p3(0x0c, 0xd7, 0xd6, 0x9d);
  // 2C: write VCOM register
  gdeWriteCommand_p1(0x2c, 0xa8); // VCOM 7c
  // 3C: border waveform control
  //	gdeWriteCommand_p1(0x3c, 0x71); // POR
  //	gdeWriteCommand_p1(0x3c, 0x33); // FIXME: check value (in original
  //source, but not used)
  // F0: ???
  //	gdeWriteCommand_p1(0xf0, 0x1f); // +15V/-15V ?? (in original source, but
  //not used)

  // 11: data entry mode setting
  gdeWriteCommand_p1(0x11, 0x03); // X inc, Y inc
  // 21: display update control 1
  //	gdeWriteCommand_p1(0x21, 0x8f); // (in original source, but not used)
  // 22: display update control 2
  //	gdeWriteCommand_p1(0x22, 0xf0); // (in original source, but not used)
}

#endif // CONFIG_SHA_BADGE_EINK_GDEH029A1
