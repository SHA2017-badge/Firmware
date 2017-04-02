#include <string.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "event_queue.h"

#include "badge_leds.h"

#ifdef CONFIG_SHA_BADGE_V2
void demo_leds(void)
{
	uint8_t rgb[6*3] = {
		 21,  42,  63,
		 42,  63,  42,
		 63,  42,  21,
		 42,  21,   0,
		 21,   0,  21,
		  0,  21,  42,
	};
	int8_t dir[6*3] = {
		+1, +1, -1,
		+1, -1, -1,
		-1, -1, -1,
		-1, -1, +1,
		-1, +1, +1,
		+1, +1, +1,
	};

	while (1) {
		// exit on random keypress
		uint32_t buttons_down = 0;
		if (xQueueReceive(evt_queue, &buttons_down, 10 / portTICK_RATE_MS))
		{
			if (buttons_down & 0xffff)
				break;
		}

		badge_leds_set_state(rgb);

		int i;
		for (i=0; i<6*3; i++) {
			rgb[i] += dir[i];
			if (rgb[i] == 0)
				dir[i] = +1;
			if (rgb[i] == 63)
				dir[i] = -1;
		}
	}

	memset(rgb, 0, sizeof(rgb));
	badge_leds_set_state(rgb);
}
#endif // CONFIG_SHA_BADGE_V2
