#ifndef BADGE_EINK_H
#define BADGE_EINK_H

#include <stdint.h>

#define BADGE_EINK_WIDTH  296
#define BADGE_EINK_HEIGHT 128

#define DISPLAY_FLAG_GREYSCALE  1
#define DISPLAY_FLAG_ROTATE_180 2
#define DISPLAY_FLAG_NO_UPDATE  4
#define DISPLAY_FLAG_LUT_BIT    3
#define DISPLAY_FLAG_LUT_SIZE   4

extern void badge_eink_init(void);

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
