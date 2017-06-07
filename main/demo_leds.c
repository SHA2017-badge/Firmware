#include "sdkconfig.h"

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <badge_input.h>
#include <badge_pins.h>
#include <badge_leds.h>

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

void demo_leds(void)
{
	uint8_t rgbw[6*4] = {
		 L1,  L2,  L3, L2,
		 L2,  L3,  L2, L1,
		 L3,  L2,  L1, L0,
		 L2,  L1,  L0, L1,
		 L1,  L0,  L1, L2,
		 L0,  L1,  L2, L3,
	};
	int8_t dir[6*4] = {
		+1, +1, -1, -1,
		+1, -1, -1, -1,
		-1, -1, -1, +1,
		-1, -1, +1, +1,
		-1, +1, +1, +1,
		+1, +1, +1, -1,
	};

	while (1) {
		// exit on random keypress
		uint32_t buttons_down = 0;
		if (xQueueReceive(badge_input_queue, &buttons_down, 10 / portTICK_RATE_MS))
		{
			if (buttons_down & 0xffff)
				break;
		}

		badge_leds_set_state(rgbw);

		int i;
		for (i=0; i<6*4; i++) {
			rgbw[i] += dir[i];
			if (rgbw[i] == L0)
				dir[i] = +1;
			if (rgbw[i] == L3)
				dir[i] = -1;
		}
	}

	memset(rgbw, 0, sizeof(rgbw));
	badge_leds_set_state(rgbw);
}
#endif // PIN_NUM_LEDS
