#include "sdkconfig.h"

#ifdef CONFIG_SHA_BADGE_EINK_GDEH029A1

#include <stdbool.h>
#include <stdint.h>

#include "gde.h"
#include "gde-driver.h"

// GDEH029A1
// SSD1608

/* LUT data without the last 2 bytes */

// full, includes inverting
const uint8_t badge_eink_lut_full[30] = {
	/* VS */
	0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 0x66, 0x69,
	0x69, 0x59, 0x58, 0x99, 0x99, 0x88, 0x00, 0x00, 0x00, 0x00,
	/* TP */
	0xF8, 0xB4, 0x13, 0x51, 0x35, 0x51, 0x51, 0x19, 0x01, 0x00,
};

// full, no inversion
const uint8_t badge_eink_lut_normal[30] = {
    /* VS */
	0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* TP */
    0x13, 0x14, 0x44, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// full, no inversion, needs 2 updates for full update
const uint8_t badge_eink_lut_faster[30] = {
    /* VS */
	0x10, 0x18, 0x18, 0x18, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* TP */
    0x11, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// full, no inversion, needs 4 updates for full update
const uint8_t badge_eink_lut_fastest[30] = {
	/* VS */
	0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* TP */
	0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

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

void setRamArea(uint8_t Xstart, uint8_t Xend, uint16_t Ystart, uint16_t Yend) {
  // set RAM X - address Start / End position
  gdeWriteCommand_p2(0x44, Xstart, Xend);
  // set RAM Y - address Start / End position
  gdeWriteCommand_p4(0x45, Ystart & 0xff, Ystart >> 8, Yend & 0xff, Yend >> 8);
}

void setRamPointer(uint8_t addrX, uint16_t addrY) {
  // set RAM X address counter
  gdeWriteCommand_p1(0x4e, addrX);
  // set RAM Y address counter
  gdeWriteCommand_p2(0x4f, addrY & 0xff, addrY >> 8);
}

#endif // CONFIG_SHA_BADGE_EINK_GDEH029A1
