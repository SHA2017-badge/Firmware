#include <string.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "event_queue.h"

#include "badge_pins.h"
#include "badge_leds.h"

#ifdef PIN_NUM_LEDS
void demo_leds(void)
{
	uint8_t rgbw[6*4] = {
		 21,  42,  63, 1,
		 42,  63,  42, 1,
		 63,  42,  21, 1,
		 42,  21,   0, 1,
		 21,   0,  21, 1,
		  0,  21,  42, 1,
	};
	int8_t dir[6*4] = {
		+1, +1, -1, 0,
		+1, -1, -1, 0,
		-1, -1, -1, 0,
		-1, -1, +1, 0,
		-1, +1, +1, 0,
		+1, +1, +1, 0,
	};

	while (1) {
		// exit on random keypress
		uint32_t buttons_down = 0;
		if (xQueueReceive(evt_queue, &buttons_down, 10 / portTICK_RATE_MS))
		{
			if (buttons_down & 0xffff)
				break;
		}

		badge_leds_set_state(rgbw);

		int i;
		for (i=0; i<6*4; i++) {
			rgbw[i] += dir[i];
			if (rgbw[i] == 0)
				dir[i] = +1;
			if (rgbw[i] == 63)
				dir[i] = -1;
		}
	}

	memset(rgbw, 0, sizeof(rgbw));
	badge_leds_set_state(rgbw);
}
#endif // PIN_NUM_LEDS
