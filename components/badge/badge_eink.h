#ifndef BADGE_EINK_H
#define BADGE_EINK_H

#include <stdint.h>

#define BADGE_EINK_WIDTH  296
#define BADGE_EINK_HEIGHT 128

extern void badge_eink_init(void);

/* badge_eink_update 'lut' settings */
#define BADGE_EINK_LUT_CUSTOM  -1
#define BADGE_EINK_LUT_FULL     0
#define BADGE_EINK_LUT_NORMAL   1
#define BADGE_EINK_LUT_FASTER   2
#define BADGE_EINK_LUT_FASTEST  3
#define BADGE_EINK_LUT_DEFAULT  BADGE_EINK_LUT_FULL
#define BADGE_EINK_LUT_MAX      BADGE_EINK_LUT_FASTEST

struct badge_eink_update {
	int lut;
	const uint8_t *lut_custom;
	int reg_0x3a;
	int reg_0x3b;
	int y_start;
	int y_end;
};

// default config for convenience
extern const struct badge_eink_update eink_upd_default;

extern void badge_eink_update(const struct badge_eink_update *upd_conf);

/* badge_eink_display 'mode' settings */
// bitmapped flags:
#define DISPLAY_FLAG_GREYSCALE  1
#define DISPLAY_FLAG_ROTATE_180 2
#define DISPLAY_FLAG_NO_UPDATE  4
// fields and sizes:
#define DISPLAY_FLAG_LUT_BIT    8
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
 */
extern void badge_eink_display(const uint8_t *img, int mode);


/* And some more low-level methods; only use them if you know what you're doing. :-) */

extern void badge_eink_set_ram_area(uint8_t x_start, uint8_t x_end,
		uint16_t y_start, uint16_t y_end);
extern void badge_eink_set_ram_pointer(uint8_t x_addr, uint16_t y_addr);

#endif // BADGE_EINK_H
