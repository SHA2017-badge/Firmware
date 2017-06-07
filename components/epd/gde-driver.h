#ifndef EPD_GDE_DRIVER_H
#define EPD_GDE_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

// low-level display, 90 degrees rotated
#define DISP_SIZE_X 128
#define DISP_SIZE_Y 296
#define DISP_SIZE_X_B ((DISP_SIZE_X + 7) >> 3)

extern void initDisplay(void);

extern void setRamArea(unsigned char Xstart, unsigned char Xend,
                       unsigned short Ystart, unsigned short Yend);
extern void setRamPointer(unsigned char addrX, unsigned short addrY);

#endif
