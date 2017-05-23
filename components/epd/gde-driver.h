#ifndef EPD_GDE_DRIVER_H
#define EPD_GDE_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#define DISP_SIZE_X 128
#define DISP_SIZE_Y 296

#define DISP_SIZE_X_B ((DISP_SIZE_X + 7) >> 3)

#define LUT_DEFAULT 0
#define LUT_FULL 0
#define LUT_PART 1
#define LUT_FASTER 2
#define LUT_FASTEST 3
#define LUT_MAX 3
extern void initDisplay(void);
extern void writeLUT(int lut_idx);

extern void drawImage(const unsigned char *picture);

#define FONT_16PX 0x01
#define FONT_INVERT 0x02
#define FONT_FULL_WIDTH 0x04
#define FONT_MONOSPACE 0x08
#define FONT_UNDERLINE_1 0x10
#define FONT_UNDERLINE_2 0x20
extern int drawText(int x, int y, int y_len, const char *text, uint8_t flags);

extern void setRamArea(unsigned char Xstart, unsigned char Xend,
                       unsigned short Ystart, unsigned short Yend);
extern void setRamPointer(unsigned char addrX, unsigned short addrY);
extern void updateDisplay(void);
extern void updateDisplayPartial(unsigned short yStart, unsigned short yEnd);

#endif
