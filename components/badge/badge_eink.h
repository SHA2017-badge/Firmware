/** @file badge_eink.h */
#ifndef BADGE_EINK_H
#define BADGE_EINK_H

#include <stdint.h>
#include <esp_err.h>

/** the width of the eink display */
#define BADGE_EINK_WIDTH  296

/** the height of the eink display */
#define BADGE_EINK_HEIGHT 128

/** Initialize the eink display
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_eink_init(void);

/** badge_eink_update 'lut' settings */
enum badge_eink_lut
{
	BADGE_EINK_LUT_CUSTOM  = -1,
	BADGE_EINK_LUT_FULL    =  0,
	BADGE_EINK_LUT_NORMAL  =  1,
	BADGE_EINK_LUT_FASTER  =  2,
	BADGE_EINK_LUT_FASTEST =  3,
	BADGE_EINK_LUT_DEFAULT = BADGE_EINK_LUT_FULL,
	BADGE_EINK_LUT_MAX     = BADGE_EINK_LUT_FASTEST,
};

/** config-settings structure */
struct badge_eink_update {
	/** lut index */
	int lut;
	/** optional lut flags */
	int lut_flags;
	/** the raw lut data if BADGE_EINK_LUT_CUSTOM is selected */
	const struct badge_eink_lut_entry *lut_custom;
	/** raw setting for the number of dummy lines */
	int reg_0x3a;
	/** raw setting for the time per line */
	int reg_0x3b;
	/** the start column for partial-screen-updates */
	int y_start;
	/** the end column for partial-screen-updates */
	int y_end;
};

/** default config for convenience */
extern const struct badge_eink_update eink_upd_default;

/** refresh the eink display with given config-settings
 * @param upd_conf the config-settings to use
 */
extern void badge_eink_update(const struct badge_eink_update *upd_conf);

/* badge_eink_display 'mode' settings */
// bitmapped flags:
#define DISPLAY_FLAG_GREYSCALE  1
#define DISPLAY_FLAG_ROTATE_180 2
#define DISPLAY_FLAG_NO_UPDATE  4
#define DISPLAY_FLAG_FULL_UPDATE 8
// fields and sizes:
#define DISPLAY_FLAG_LUT_BIT    8
#define DISPLAY_FLAG_LUT_SIZE   4
#define DISPLAY_FLAG_LUT(x) ((1+(x)) << DISPLAY_FLAG_LUT_BIT)

extern void badge_eink_display_one_layer(const uint8_t *img, int flags);
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

extern void badge_eink_deep_sleep(void);
extern void badge_eink_wakeup(void);

#endif // BADGE_EINK_H
