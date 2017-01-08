#ifndef EPD_GDE_DRIVER_H
#define EPD_GDE_DRIVER_H
#define GDEH029A1

#include <stdbool.h>
#include <stdint.h>

void writeLUT(bool fast);
void initDisplay(bool fast);

void drawImage(const unsigned char *picture);
int drawText(int x, int y, int y_len, const char *text, bool invert, bool fill);

void setRamArea(unsigned char Xstart, unsigned char Xend,
                unsigned short Ystart, unsigned short Yend);
void setRamPointer(unsigned char addrX, unsigned short addrY);
void partialDisplay(unsigned char RAM_XST, unsigned char RAM_XEND,
                    unsigned short RAM_YST, unsigned short RAM_YEND);
void updateDisplay(void);
void updateDisplayPartial(unsigned short yStart, unsigned short yEnd);
//void updateDisplayPartial(void);
void writeRam(void);
void powerOff(void);
void powerOn(void);

#endif
