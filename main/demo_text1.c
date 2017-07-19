#include "sdkconfig.h"

#include <string.h>

#include <badge_eink.h>
#include <badge_eink_fb.h>
#include <badge_input.h>

#include <font.h>

void
demoText1(void) {
	esp_err_t err = badge_eink_fb_init();
	assert( err == ESP_OK );

	/* draw test pattern */
	{
		int y;
		for (y=0; y<BADGE_EINK_HEIGHT; y++)
		{
			memset(&badge_eink_fb[y * (BADGE_EINK_WIDTH/8)], (y&1) ? 0x55 : 0xaa, (BADGE_EINK_WIDTH/8));
		}

		badge_eink_display(badge_eink_fb, DISPLAY_FLAG_LUT(0));
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
			draw_font(badge_eink_fb, 16, row, BADGE_EINK_WIDTH-32, &line_1[pos], FONT_INVERT | FONT_FULL_WIDTH);
		if (num == 0)
			break;
		pos += num;
		row += 8;
	}
	draw_font(badge_eink_fb, 16, row, BADGE_EINK_WIDTH-32, "", FONT_INVERT | FONT_FULL_WIDTH);

	row += 8;

	const char *line_2 =
		"Multiple make functions can be combined into one. For "
		"example: to build the app & bootloader using 5 jobs in "
		"parallel, then flash everything, and then display "
		"serial output from the ESP32 run:";
	pos = 0;
	while (line_2[pos]) {
		int num =
			draw_font(badge_eink_fb, 16, row, BADGE_EINK_WIDTH-32, &line_2[pos], FONT_INVERT | FONT_FULL_WIDTH);
		if (num == 0)
			break;
		pos += num;
		row += 8;
	}

	// try monospace
	draw_font(badge_eink_fb, 0, 120, BADGE_EINK_WIDTH, " Just a status line. Wifi status: not connected",
			FONT_FULL_WIDTH | FONT_MONOSPACE);

	badge_eink_display(badge_eink_fb, DISPLAY_FLAG_LUT(0));

	// wait for random keypress
	badge_input_get_event(-1);
}
