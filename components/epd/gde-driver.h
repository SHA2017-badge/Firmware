#ifndef EPD_GDE_DRIVER_H
#define EPD_GDE_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

// low-level display, 90 degrees rotated
#define DISP_SIZE_X 128
#define DISP_SIZE_Y 296
#define DISP_SIZE_X_B ((DISP_SIZE_X + 7) >> 3)

#define LUT_DEFAULT 0
#define LUT_FULL 0
#define LUT_PART 1
#define LUT_FASTER 2
#define LUT_FASTEST 3
#define LUT_MAX 3

/* pre-defined lookup-table display-updates. size depends on model. */
extern const uint8_t badge_eink_lut_full[];     // full, includes inverting
extern const uint8_t badge_eink_lut_normal[];   // full, no inversion
extern const uint8_t badge_eink_lut_faster[];   // full, no inversion, needs 2 updates for full update
extern const uint8_t badge_eink_lut_fastest[];  // full, no inversion, needs 4 updates for full update

extern void initDisplay(void);

extern void setRamArea(unsigned char Xstart, unsigned char Xend,
                       unsigned short Ystart, unsigned short Yend);
extern void setRamPointer(unsigned char addrX, unsigned short addrY);

#endif
