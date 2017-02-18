#include <stdbool.h>
#include <stdint.h>

#include "gde.h"
#include "gdeh029a1.h"

// GDEH029A1
// SSD1608

#include "font_16px.h"
#include "font_8px.h"

/* LUT data without the last 2 bytes */
const uint8_t LUTDefault_full[30] = {
    /* VS */
    0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 0x66, 0x69, 0x69, 0x59,
    0x58, 0x99, 0x99, 0x88, 0x00, 0x00, 0x00, 0x00,
    /* TP */
    0xF8, 0xB4, 0x13, 0x51, 0x35, 0x51, 0x51, 0x19, 0x01, 0x00,
};

const uint8_t LUTDefault_part[30] = {
    /* VS */
    0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* TP */
    0x13, 0x14, 0x44, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const uint8_t LUTDefault_faster[30] = {
    /* VS */
    0x10, 0x18, 0x18, 0x18, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* TP */
    0x11, 0x11, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const uint8_t LUTDefault_fastest[30] = {
    /* VS */
    0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* TP */
    0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const uint8_t *LUT_data[LUT_MAX + 1] = {
    LUTDefault_full, LUTDefault_part, LUTDefault_faster, LUTDefault_fastest,
};

void writeLUT(int lut_idx) {
  gdeWriteCommandStream(0x32, LUT_data[lut_idx], 30);
}

void initDisplay(int lut_idx) {
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
  // 3A: set dummy line period
  gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
  // 3B: set gate line width
  gdeWriteCommand_p1(0x3b, 0x08); // 2us per line
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

  // configure LUT.
  // The device should have a hardcoded list, but reading the
  // temperature sensor or loading the LUT data from non-volatile
  // memory doesn't seem to work.
  writeLUT(lut_idx);
}

void drawImage(const uint8_t *picture) {
  setRamArea(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
  setRamPointer(0, 0);
  gdeWriteCommandStream(0x24, picture, DISP_SIZE_X_B * DISP_SIZE_Y);
}

int drawText(int x, int y, int y_len, const char *text, uint8_t flags) {
  // select font definitions
  const uint8_t *font_data;
  const uint8_t *font_width;
  int font_FIRST, font_LAST, font_WIDTH, font_HEIGHT;

  if (flags & FONT_16PX) {
    font_data = font_16px_data;
    font_width = font_16px_width;
    font_FIRST = FONT_16PX_FIRST;
    font_LAST = FONT_16PX_LAST;
    font_WIDTH = FONT_16PX_WIDTH;
    font_HEIGHT = FONT_16PX_HEIGHT;
  } else {
    font_data = font_8px_data;
    font_width = font_8px_width;
    font_FIRST = FONT_8PX_FIRST;
    font_LAST = FONT_8PX_LAST;
    font_WIDTH = FONT_8PX_WIDTH;
    font_HEIGHT = FONT_8PX_HEIGHT;
  }

  if (x < 0)
    return 0;
  if (x > DISP_SIZE_X_B - font_HEIGHT)
    return 0;

  if (y < 0)
    return 0;
  if (y >= DISP_SIZE_Y)
    return 0;

  if (y_len <= 0)
    y_len += DISP_SIZE_Y - y;
  if (y_len <= 0)
    return 0;
  if (y_len > DISP_SIZE_Y - y)
    y_len = DISP_SIZE_Y - y;

  int y_max = y_len + y;
  setRamArea(x, x + font_HEIGHT - 1, y, y_max - 1);
  setRamPointer(x, y);

  gdeWriteCommandInit(0x24); // write RAM

  uint8_t ch_ul = 0x00;
  if (flags & FONT_UNDERLINE_1)
    ch_ul |= 0x80;
  if (flags & FONT_UNDERLINE_2)
    ch_ul |= 0x40;

  int numChars = 0;
  while (*text != 0 && y < y_max) {
    int ch = *text;
    if (ch < font_FIRST || ch > font_LAST)
      ch = 0;
    else
      ch -= font_FIRST;

    uint8_t ch_width;
    unsigned int f_index = ch * (font_WIDTH * font_HEIGHT);
    if (flags & FONT_MONOSPACE) {
      ch_width = font_WIDTH;
    } else {
      ch_width = font_width[ch];
      f_index += (ch_width >> 4) * font_HEIGHT;
      ch_width &= 0x0f;
    }

    if (y + ch_width >= y_max)
      break; // not enough space for full character

    while (ch_width > 0 && y < y_max) {
      int ch_x;
      for (ch_x = 0; ch_x < font_HEIGHT; ch_x++) {
        uint8_t ch = font_data[f_index++];
        if (flags & FONT_INVERT)
          ch = ~ch;
        gdeWriteByte(ch ^ (ch_x == 0 ? ch_ul : 0));
      }
      y++;
      ch_width--;
    }
    text = &text[1];
    numChars++;
  }

  if (flags & FONT_FULL_WIDTH) {
    uint8_t ch = 0;
    if (flags & FONT_INVERT)
      ch = ~ch;
    while (y < y_max) {
      int ch_x;
      for (ch_x = 0; ch_x < font_HEIGHT; ch_x++)
        gdeWriteByte(ch ^ (ch_x == 0 ? ch_ul : 0));
      y++;
    }
  }

  gdeWriteCommandEnd();

  return numChars;
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

void updateDisplay(void) {
  // enforce full screen update
  gdeWriteCommand_p3(0x01, (DISP_SIZE_Y - 1) & 0xff, (DISP_SIZE_Y - 1) >> 8,
                     0x00);
  gdeWriteCommand_p2(0x0f, 0, 0);

  //	gdeWriteCommand_p1(0x22, 0xc7);
  gdeWriteCommand_p1(0x22, 0xc7);
  //	gdeWriteCommand_p1(0x22, 0xff);
  // 80 - enable clock signal
  // 40 - enable CP
  // 20 - load temperature value
  // 10 - load LUT
  // 08 - initial display
  // 04 - pattern display
  // 02 - disable CP
  // 01 - disable clock signal
  gdeWriteCommand(0x20);
}

void updateDisplayPartial(uint16_t yStart, uint16_t yEnd) {
  // NOTE: partial screen updates work, but you need the full
  //       LUT waveform. but still a lot of ghosting..
  uint16_t yLen = yEnd - yStart;
  gdeWriteCommand_p3(0x01, yLen & 0xff, yLen >> 8, 0x00);
  gdeWriteCommand_p2(0x0f, yStart & 0xff, yStart >> 8);
  gdeWriteCommand_p1(0x22, 0xc7);
  gdeWriteCommand(0x20);
}

/* Currently unused (and untested) functions: */
/*
void
updateDisplayPartial(void)
{
        gdeWriteCommand_p1(0x22, 0x04);
//	gdeWriteCommand_p1(0x22, 0x08);
        gdeWriteCommand(0x20);
}

void
powerOn(void)
{
        gdeWriteCommand_p1(0x22, 0xc0);
        gdeWriteCommand(0x20);
}

void
powerOff(void)
{
        gdeWriteCommand_p1(0x22, 0xc3);
        gdeWriteCommand(0x20);
}

void
partialDisplay(uint8_t RAM_XST, uint8_t RAM_XEND,
               uint16_t RAM_YST, uint16_t RAM_YEND)
{
        setRamArea(RAM_XST, RAM_XEND, RAM_YST, RAM_YEND);
        setRamPointer(RAM_XST, RAM_YST);
}

void
writeRam(void)
{
        gdeWriteCommand(0x24);
}

void
enableChargepump(void)
{
        gdeWriteCommand_p1(0xf0, 0x8f); // undefined register?
        gdeWriteCommand_p1(0x22, 0xc0);
        gdeWriteCommand(0x20);
}

void
disableChargepump(void)
{
        gdeWriteCommand_p1(0x22, 0xf0);
        gdeWriteCommand(0x20);
}
*/
