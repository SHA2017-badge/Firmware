#include "sdkconfig.h"

#include <badge_pins.h>
#ifdef I2C_MPR121_ADDR
#include <string.h>

#include <rom/ets_sys.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <badge_eink.h>
#include <badge_input.h>
#include <badge_button.h>
#include <badge_mpr121.h>

#include <font.h>

#include "demo_mpr121.h"

// re-use screen_buf from main.c
extern uint8_t screen_buf[296*16];

void
demoMpr121(void)
{
	static const char *names[8] = {
		"A",
		"B",
		"Start",
		"Select",
		"Down",
		"Right",
		"Up",
		"Left",
	};

	while (1)
	{
		memset(screen_buf, 0xff, sizeof(screen_buf));
		struct badge_mpr121_touch_info info;

		int res = badge_mpr121_get_touch_info(&info);

		int i;
		for (i=0; i<8; i++)
		{
			int flags = 0;
			if ((info.touch_state >> i) & 1)
				flags |= FONT_INVERT;

			draw_font(screen_buf, 2, 16 + 10*i, 36, names[i], (FONT_FULL_WIDTH | FONT_INVERT) ^ flags);
			if (res == ESP_OK) {
				int x, y;
				int d = (info.data[i] >> 2) & 255;
				for (y=18+10*i; y<20+10*i; y++)
				{
					for (x=40; x<40+d; x++)
					{
						screen_buf[y*(296/8) + (x/8)] &= ~( 1 << (x&7) );
					}
				}
				d = info.baseline[i] & 255;
				for (y=20+10*i; y<22+10*i; y++)
				{
					for (x=40; x<40+d; x++)
					{
						screen_buf[y*(296/8) + (x/8)] &= ~( 1 << (x&7) );
					}
				}
				d = info.baseline[i] - (info.touch[i] >> 2);
				if (d < 0) d = 0;
				d &= 255;
				for (y=22+10*i; y<24+10*i; y++)
				{
					for (x=40+d-1; x<40+d+1; x++)
					{
						screen_buf[y*(296/8) + (x/8)] &= ~( 1 << (x&7) );
					}
				}
				d = info.baseline[i] - (info.release[i] >> 2);
				if (d < 0) d = 0;
				d &= 255;
				for (y=16+10*i; y<18+10*i; y++)
				{
					for (x=40+d-1; x<40+d+1; x++)
					{
						screen_buf[y*(296/8) + (x/8)] &= ~( 1 << (x&7) );
					}
				}
			}
		}

		/* update display */
		badge_eink_display(screen_buf, DISPLAY_FLAG_LUT(2));

		// wait 0.1 second
		badge_input_get_event(100);
	}
}

#endif // I2C_MPR121_ADDR
