#include "sdkconfig.h"

#ifdef CONFIG_SHA_BADGE_EINK_DEPG0290B1
#include <stdbool.h>
#include <stdint.h>

#include "gde.h"
#include "depg0290b1.h"

// actually 'DEPG0290B01'

#include "font_16px.h"
#include "font_8px.h"

/* LUT data without the last 2 bytes */
const uint8_t LUTDefault_full[70] = {
	// Voltages and other settings? Timing?
	0xA0, 0x90, 0x50, 0x00, 0x00, 0x00, 0x00,
	0x50, 0x90, 0xA0, 0x00, 0x00, 0x00, 0x00,
	0xA0, 0x90, 0x50, 0x00, 0x00, 0x00, 0x00,
	0x50, 0x90, 0xA0, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	// Update program
	//
	// Top three lines are the main program (bottom 4 have unknown function)
	// Line 1: Negative image
	// Line 2: White/Black flashing
	// Line 3: Positive image
	//
	// Line construction
	// First two bytes denote Intensity (range 0x00 to 0x0F)
	// Second two bytes denote lenght of each 'pulse' (range 0x00 to 0xFF)
	// Last byte denotes number of repeats (0 = line runs 1 time, range 0x00 to 0xFF)
	// If you don't want a line to do anything, set all bytes to 0x00.
	// This way you can make a quick update cycle between two screens.
	// Maybe not as pretty/crisp but nice and fast is also awesome!

	// Negative image
	// first two bytes negative image, length white pulse (0-FF), length black pulse (0-FF), last byte repeats

	0x0F, 0x0F, 0x00, 0x00, 0x00,

	// White or black flash
	// white flash intensity, black flash intensity, length white pulse (0-FF), length black pulse (0-FF), repeats

	0x0F, 0x0F, 0x00, 0x00, 0x02,

	// Positive image
	// first byte or second byte positive image (don't know why you need both), rest same as above

	0x0F, 0x0F, 0x00, 0x00, 0x00,

	// Unknown what lines below actually do.
	// They seem to be programs to, but have no visible effect on dislay.
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
};

const uint8_t LUTDefault_part[70] = {
	// This is the standard LUT we received with the display
	0xA0, 0x90, 0x50, 0x00, 0x00, 0x00, 0x00,
	0x50, 0x90, 0xA0, 0x00, 0x00, 0x00, 0x00,
	0xA0, 0x90, 0x50, 0x00, 0x00, 0x00, 0x00,
	0x50, 0x90, 0xA0, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0F, 0x0F, 0x00, 0x00, 0x00,
	0x0F, 0x0F, 0x00, 0x00, 0x02,
	0x0F, 0x0F, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
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
  gdeWriteCommandStream(0x32, LUT_data[lut_idx], 70);
}

void initDisplay(void) {
  // Hardware reset
  gdeReset();
  gdeBusyWait();

  // Software reset
  gdeWriteCommand(0x12);
  gdeBusyWait();

  // Set analog block control
  gdeWriteCommand_p1(0x74, 0x54);

  // Set digital block control
  gdeWriteCommand_p1(0x7E, 0x3B);

  // Set display size and driver output control
  gdeWriteCommand_p3(0x01, 0x27, 0x01, 0x00);

  // Ram data entry mode
  // Adress counter is updated in Y direction, Y increment, X increment
  gdeWriteCommand_p1(0x11, 0x03);

  // Set RAM X address (00h to 0Fh)
  gdeWriteCommand_p2(0x44, 0x00, 0x0F);

  // Set RAM Y address (0127h to 0000h)
  gdeWriteCommand_p4(0x45, 0x00, 0x00, 0x27, 0x01);

  // Set border waveform for VBD (see datasheet)
  gdeWriteCommand_p1(0x3C, 0x01);

  // SET VOLTAGE AND LOAD LUT_MAX

  // Set VCOM value
  gdeWriteCommand_p1(0x2C, 0x26);

  // Gate voltage setting (17h = 20 Volt, ranges from 10v to 21v)
  gdeWriteCommand_p1(0x03, 0x17);

  // Source voltage setting (15volt, 0 volt and -15 volt)
  gdeWriteCommand_p3(0x04, 0x41, 0x00, 0x32);

  // Frame setting 50hz
  gdeWriteCommand_p1(0x3A, 0x30);
  gdeWriteCommand_p1(0x3B, 0x0A);

  writeLUT(LUT_DEFAULT);
}

void drawImage(const uint8_t *picture) {
  setRamArea(0, DISP_SIZE_X_B - 1, DISP_SIZE_Y - 1, 0);
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
  //gdeWriteCommand_p3(0x01, (DISP_SIZE_Y - 1) & 0xff, (DISP_SIZE_Y - 1) >> 8,
  //                   0x00);
  // gdeWriteCommand_p2(0x0f, 0, 0);

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

#endif // CONFIG_SHA_BADGE_EINK_DEPG0290B1
