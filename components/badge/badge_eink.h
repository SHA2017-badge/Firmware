#ifndef BADGE_EINK_H
#define BADGE_EINK_H

#include <stdint.h>

#define DISPLAY_FLAG_GREYSCALE  1
#define DISPLAY_FLAG_ROTATE_180 2
#define DISPLAY_FLAG_LUT_BIT    2
#define DISPLAY_FLAG_LUT_SIZE   4

/*
 * display image on badge
 *
 * img is 1 byte per pixel; from top-left corner in horizontal
 * direction first (296 pixels).
 *
 * mode is still work in progress. think of:
 * - fast update
 * - slow/full update
 * - greyscale update
 *
 */
extern void badge_eink_display(const uint8_t *img, int mode);

#endif // BADGE_EINK_H
