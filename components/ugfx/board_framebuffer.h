/*
 * GDISP framebuffer driver for our eink display, to bridge to ugfx.
 */

/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include <stdlib.h>
#include "badge_eink.h"
#include "esp_system.h"

// Set this to your frame buffer pixel format.
#ifndef GDISP_LLD_PIXELFORMAT
	#define GDISP_LLD_PIXELFORMAT		GDISP_PIXELFORMAT_MONO
#endif

// Uncomment this if your frame buffer device requires flushing
#define GDISP_HARDWARE_FLUSH		TRUE

// For now we simply keep the current state of the framebuffer in memory, and
// encode it and send it to the badge_eink driver on flush
uint8_t* framebuffer;
uint8_t* target_buffer;

#ifdef GDISP_DRIVER_VMT

	static void board_init(GDisplay *g, fbInfo *fbi) {
		badge_eink_init();

		// Careful: this is never freed, so don't initialize the board multiple times!
		framebuffer = malloc(BADGE_EINK_WIDTH * BADGE_EINK_HEIGHT);
		target_buffer = malloc(BADGE_EINK_WIDTH * BADGE_EINK_HEIGHT / 8);

    ets_printf("Initializing eink ugfx driver!\n");
		ets_printf("sizeof(COLOR_TYPE): %d", sizeof(COLOR_TYPE));

		g->g.Width = BADGE_EINK_WIDTH;
		g->g.Height = BADGE_EINK_HEIGHT;
		g->g.Backlight = 100;
		g->g.Contrast = 50;
		fbi->linelen = g->g.Width;
		fbi->pixels = framebuffer;
	}

	#if GDISP_HARDWARE_FLUSH
		static void board_flush(GDisplay *g) {
			(void) g;
			ets_printf("Flushing framebuffer!\n");
			ets_printf("First byte: %d\n", framebuffer[0]);
			for (int i = 0; i < BADGE_EINK_WIDTH * BADGE_EINK_HEIGHT / 8; i++) {
				target_buffer[i] =
					framebuffer[i * 8 + 7] << 7 |
					framebuffer[i * 8 + 6] << 6 |
					framebuffer[i * 8 + 5] << 5 |
					framebuffer[i * 8 + 4] << 4 |
					framebuffer[i * 8 + 3] << 3 |
					framebuffer[i * 8 + 2] << 2 |
					framebuffer[i * 8 + 1] << 1 |
					framebuffer[i * 8];
			}
			badge_eink_display(target_buffer, 3 << DISPLAY_FLAG_LUT_BIT);
			ets_printf("Flushed framebuffer!\n");
		}
	#endif

	#if GDISP_NEED_CONTROL
		static void board_backlight(GDisplay *g, uint8_t percent) {
			// TODO: Can be an empty function if your hardware doesn't support this
			(void) g;
			(void) percent;
		}

		static void board_contrast(GDisplay *g, uint8_t percent) {
			// TODO: Can be an empty function if your hardware doesn't support this
			(void) g;
			(void) percent;
		}

		static void board_power(GDisplay *g, powermode_t pwr) {
			// TODO: Can be an empty function if your hardware doesn't support this
			(void) g;
			(void) pwr;
		}
	#endif

#endif /* GDISP_LLD_BOARD_IMPLEMENTATION */
