#include "gdeh029a1.h"
#include "driver/gpio.h"
#include "soc/io_mux_reg.h"
#include "soc/spi_reg.h"
#include <gde.h>
#include <pins.h>
#include <stdbool.h>

// GDEH029A1
// SSD1608

#define DISP_SIZE_X 128
#define DISP_SIZE_Y 296

#define DISP_SIZE_X_B ((DISP_SIZE_X + 7) >> 3)

/* 01 - driver output control */
const unsigned char GDOControl[] = {0x01, 0x27, 0x01, 0x00}; // for 2.9inch
/* 0C - booster soft start control */
const unsigned char softstart[] = {0x0c, 0xd7, 0xd6, 0x9d};
/* 21 - display update control 1 */
const unsigned char Rambypass[] = {0x21, 0x8f};        // Display update
/* 22 - display update control 2 */
const unsigned char MAsequency[] = {0x22, 0xf0};       // clock
/* 03 - gate driving voltage control */
const unsigned char GDVol[] = {0x03, 0x00};            // Gate voltage +15V/-15V
/* 04 - source driving voltage control */
const unsigned char SDVol[] = {0x04, 0x0a};            // Source voltage +15V/-15V
/* 2C - write VCOM register */
const unsigned char VCOMVol[] = {0x2c, 0xa8};          // VCOM 7c
/* F0 - ??? */
const unsigned char BOOSTERFB[] = {0xf0, 0x1f};        // Source voltage +15V/-15V
/* 3A - set dummy line period */
const unsigned char DummyLine[] = {0x3a, 0x1a};        // 4 dummy line per gate
/* 3B - set gate line width */
const unsigned char Gatetime[] = {0x3b, 0x08};         // 2us per line
/* 3C - border waveform control */
const unsigned char BorderWavefrom[] = {0x3c, 0x33};   // Border
/* 11 - data entry mode setting */
const unsigned char RamDataEntryMode[] = {0x11, 0x01}; // Ram data entry mode

/* LUT data without the last 2 bytes */
const unsigned char LUTDefault_part[30] = {
	/* VS */
    0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* TP */
    0x13, 0x14, 0x44, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char LUTDefault_full[30] = {
	/* VS */
    0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 0x66, 0x69,
    0x69, 0x59, 0x58, 0x99, 0x99, 0x88, 0x00, 0x00, 0x00, 0x00,
	/* TP */
    0xF8, 0xB4, 0x13, 0x51, 0x35, 0x51, 0x51, 0x19, 0x01, 0x00,
};

void writeLUT(bool fast) {
  unsigned char i;
  writeCommand(0x32); // write LUT register
  for (i = 0; i < 30; i++) {
    if (fast) {
      writeData(LUTDefault_part[i]);
    } else {
      writeData(LUTDefault_full[i]);
    }
  }
}

void initDisplay(bool fast) {
  resetDisplay();
  writeStream(GDOControl,
              sizeof(GDOControl)); // Pannel configuration, Gate selection
  writeStream(softstart, sizeof(softstart)); // X decrease, Y decrease
  writeStream(VCOMVol, sizeof(VCOMVol));     // VCOM setting
  writeStream(DummyLine, sizeof(DummyLine)); // dummy line per gate
  writeStream(Gatetime, sizeof(Gatetime));   // Gage time setting
  writeStream(RamDataEntryMode,
              sizeof(RamDataEntryMode));          // X increase, Y decrease
  setRamArea(0, DISP_SIZE_X_B-1, DISP_SIZE_Y-1, 0); // X-source area,Y-gage area
  setRamPointer(0x00, 0xC7);                // set ram

  writeLUT(fast);
}

void displayImage(const unsigned char *picture, bool partial) {
  setRamPointer(0, DISP_SIZE_Y-1); // set ram
  writeDispRam(DISP_SIZE_X, DISP_SIZE_Y, picture);
  if (partial) {
    updateDisplayPartial();
    setRamPointer(0, DISP_SIZE_Y-1); // set ram
    writeDispRam(DISP_SIZE_X, DISP_SIZE_Y, picture);
  } else {
    updateDisplay();
  }
}

void setRamArea(unsigned char Xstart, unsigned char Xend,
                unsigned short Ystart, unsigned short Yend) {
  unsigned char RamAreaX[3]; // X start and end
  unsigned char RamAreaY[5]; // Y start and end
  RamAreaX[0] = 0x44;        // command
  RamAreaX[1] = Xstart;
  RamAreaX[2] = Xend;
  RamAreaY[0] = 0x45; // command
  RamAreaY[1] = Ystart & 0xff;
  RamAreaY[2] = Ystart >> 8;
  RamAreaY[3] = Yend & 0xff;
  RamAreaY[4] = Yend >> 8;
  writeStream(RamAreaX, sizeof(RamAreaX));
  writeStream(RamAreaY, sizeof(RamAreaY));
}

void setRamPointer(unsigned char addrX, unsigned short addrY) {
  unsigned char RamPointerX[2]; // default (0,0)
  unsigned char RamPointerY[3];
  RamPointerX[0] = 0x4e;
  RamPointerX[1] = addrX;
  RamPointerY[0] = 0x4f;
  RamPointerY[1] = addrY & 0xff;
  RamPointerY[2] = addrY >> 8;
  writeStream(RamPointerX, sizeof(RamPointerX));
  writeStream(RamPointerY, sizeof(RamPointerY));
}

void updateDisplay(void) {
  writeCMD_p1(0x22, 0xc7);
  writeCommand(0x20);
  writeCommand(0xff);
}

void updateDisplayPartial(void) {
  writeCMD_p1(0x22, 0x04);
  // writeCMD_p1(0x22,0x08);
  writeCommand(0x20);
  writeCommand(0xff);
}

void powerOn(void) {
  writeCMD_p1(0x22, 0xc0);
  writeCommand(0x20);
  //	writeCommand(0xff);
}

void powerOff(void) {
  writeCMD_p1(0x22, 0xc3);
  writeCommand(0x20);
  //  writeCommand(0xff);
}

void partialDisplay(unsigned char RAM_XST, unsigned char RAM_XEND,
                    unsigned short RAM_YST, unsigned short RAM_YEND) {
  setRamArea(RAM_XST, RAM_XEND, RAM_YST, RAM_YEND);  /*set w h*/
  setRamPointer(RAM_XST, RAM_YST); /*set orginal*/
}

// Currently unused functions

void writeRam(void) { writeCommand(0x24); }

void enableChargepump(void) {
  writeCMD_p1(0xf0, 0x8f);
  writeCMD_p1(0x22, 0xc0);
  writeCommand(0x20);
  writeCommand(0xff);
}

void disableChargepump(void) {
  writeCMD_p1(0x22, 0xf0);
  writeCommand(0x20);
  writeCommand(0xff);
}
