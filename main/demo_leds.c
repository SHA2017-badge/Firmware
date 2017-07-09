#include "sdkconfig.h"

#include <string.h>

#include <badge_eink.h>
#include <badge_input.h>
#include <badge_pins.h>
#include <badge_leds.h>

#include <font.h>

#ifdef PIN_NUM_LEDS

#if 0
 #define L0  0
 #define L1 21
 #define L2 42
 #define L3 63
#else
 #define L0  0
 #define L1 10
 #define L2 20
 #define L3 30
#endif

// re-use screen_buf from main.c
extern uint8_t screen_buf[296*16];

void
demo_leds(void)
{
	memset(screen_buf, 0xff, sizeof(screen_buf));
	draw_font(screen_buf, 6, 16, 284, "testing leds. colors should be:",
		FONT_INVERT);
	draw_font(screen_buf, 6, 26, 284, "<red>,<green>,<blue>,<white>,<dimmed white>,<black>",
		FONT_INVERT);
	badge_eink_display(screen_buf, DISPLAY_FLAG_LUT(2));

	{
		uint8_t grbw[6*4] = {
			 0,  0,  0,  0,
			 0,  0,  0, 10,
			 0,  0,  0, 40,
			 0,  0, 40,  0,
			40,  0,  0,  0,
			 0, 40,  0,  0,
		}; // show R, G, B, W
		badge_leds_send_data(grbw, sizeof(grbw));
		if (badge_input_get_event(10000) != 0)
		{
			badge_leds_disable();
			return;
		}
	}

	memset(screen_buf, 0xff, sizeof(screen_buf));
	draw_font(screen_buf, 6, 16, 264, "now doing random updates",
		FONT_INVERT);
	badge_eink_display(screen_buf, DISPLAY_FLAG_LUT(2));

	uint8_t grbw[6*4] = {
		 L0,  L1,  L2, L3,
		 L1,  L0,  L1, L2,
		 L2,  L1,  L0, L1,
		 L3,  L2,  L1, L0,
		 L2,  L3,  L2, L1,
		 L1,  L2,  L3, L2,
	};

	int8_t dir[6*4] = {
		+1, +1, +1, -1,
		-1, +1, +1, +1,
		-1, -1, +1, +1,
		-1, -1, -1, +1,
		+1, -1, -1, -1,
		+1, +1, -1, -1,
	};

	while (1) {
		// exit on random keypress
		if (badge_input_get_event(10) != 0)
			break;

		badge_leds_send_data(grbw, sizeof(grbw));

		int i;
		for (i=0; i<6*4; i++) {
			grbw[i] += dir[i];
			if (grbw[i] == L0)
				dir[i] = +1;
			if (grbw[i] == L3)
				dir[i] = -1;
		}
	}

	memset(screen_buf, 0xff, sizeof(screen_buf));
	draw_font(screen_buf, 6, 16, 264, "key pressed. disabling leds",
		FONT_INVERT);
	badge_eink_display(screen_buf, DISPLAY_FLAG_LUT(2));

	badge_leds_disable();

	memset(screen_buf, 0xff, sizeof(screen_buf));
	draw_font(screen_buf, 6, 16, 264, "leds are disabled.",
		FONT_INVERT);
	badge_eink_display(screen_buf, DISPLAY_FLAG_LUT(2));
}
#endif // PIN_NUM_LEDS
