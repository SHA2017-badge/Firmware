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
	static uint32_t button_id[8] = {
		BADGE_BUTTON_A,
		BADGE_BUTTON_B,
		BADGE_BUTTON_START,
		BADGE_BUTTON_SELECT,
		BADGE_BUTTON_DOWN,
		BADGE_BUTTON_RIGHT,
		BADGE_BUTTON_UP,
		BADGE_BUTTON_LEFT,
	};

	while (1)
	{
		memset(screen_buf, 0xff, sizeof(screen_buf));
		uint32_t data[8*5];

		int res = badge_mpr121_get_electrode_data(data);

		int i;
		for (i=0; i<8; i++)
		{
			int flags = 0;
			if ((badge_input_button_state >> button_id[i]) & 1)
				flags |= FONT_INVERT;

			draw_font(screen_buf, 2, 16 + 10*i, 36, names[i], (FONT_FULL_WIDTH | FONT_INVERT) ^ flags);
			if (res != -1) {
				int x, y;
				int d = (data[i] >> 2) & 255;
				for (y=18+10*i; y<20+10*i; y++)
				{
					for (x=40; x<40+d; x++)
					{
						screen_buf[y*(296/8) + (x/8)] &= ~( 1 << (x&7) );
					}
				}
				d = (data[i+8] >> 2) & 255;
				for (y=20+10*i; y<22+10*i; y++)
				{
					for (x=40; x<40+d; x++)
					{
						screen_buf[y*(296/8) + (x/8)] &= ~( 1 << (x&7) );
					}
				}
				d = data[i+8] - data[i+16]; // touch
				if (d < 0) d = 0;
				d >>= 2;
				d &= 255;
				for (y=22+10*i; y<24+10*i; y++)
				{
					for (x=40+d-1; x<40+d+1; x++)
					{
						screen_buf[y*(296/8) + (x/8)] &= ~( 1 << (x&7) );
					}
				}
				d = data[i+8] - data[i+24]; // release
				if (d < 0) d = 0;
				d >>= 2;
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
		badge_eink_display(screen_buf, (3 << DISPLAY_FLAG_LUT_BIT));

		// wait 0.1 second
		badge_input_get_event(100);
	}
}

#endif // I2C_MPR121_ADDR
