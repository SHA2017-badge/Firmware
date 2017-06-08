#include "sdkconfig.h"

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <esp_event.h>

#include <badge_eink.h>
#include <badge_input.h>

#include <font.h>

// re-use screen_buf from main.c
extern uint8_t screen_buf[296*16];

void
demoText1(void) {
	/* draw test pattern */
	{
		int y;
		for (y=0; y<BADGE_EINK_HEIGHT; y++)
		{
			memset(&screen_buf[y * (BADGE_EINK_WIDTH/8)], (y&1) ? 0x55 : 0xaa, (BADGE_EINK_WIDTH/8));
		}

		badge_eink_display(screen_buf, (1 << DISPLAY_FLAG_LUT_BIT));
	}

	/* draw text with 8px font */
	const char *line_1 =
		"esp-idf supports compiling multiple files in parallel, "
		"so all of the above commands can be run as `make -jN` "
		"where `N` is the number of parallel make processes to "
		"run (generally N should be equal to or one more than "
		"the number of CPU cores in your system.)";

	int pos = 0;
	int row = 8;
	while (line_1[pos]) {
		int num =
			draw_font(screen_buf, 16, row, BADGE_EINK_WIDTH-32, &line_1[pos], FONT_INVERT | FONT_FULL_WIDTH);
		if (num == 0)
			break;
		pos += num;
		row += 8;
	}
	draw_font(screen_buf, 16, row, BADGE_EINK_WIDTH-32, "", FONT_INVERT | FONT_FULL_WIDTH);

	row += 8;

	const char *line_2 =
		"Multiple make functions can be combined into one. For "
		"example: to build the app & bootloader using 5 jobs in "
		"parallel, then flash everything, and then display "
		"serial output from the ESP32 run:";
	pos = 0;
	while (line_2[pos]) {
		int num =
			draw_font(screen_buf, 16, row, BADGE_EINK_WIDTH-32, &line_2[pos], FONT_INVERT | FONT_FULL_WIDTH);
		if (num == 0)
			break;
		pos += num;
		row += 8;
	}

	// try monospace
	draw_font(screen_buf, 0, 120, BADGE_EINK_WIDTH, " Just a status line. Wifi status: not connected",
			FONT_FULL_WIDTH | FONT_MONOSPACE);

	badge_eink_display(screen_buf, (1 << DISPLAY_FLAG_LUT_BIT));

	// wait for random keypress
	uint32_t buttons_down = 0;
	while ((buttons_down & 0xffff) == 0)
		xQueueReceive(badge_input_queue, &buttons_down, portMAX_DELAY);
}
