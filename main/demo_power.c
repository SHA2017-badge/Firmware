#include "sdkconfig.h"

#include <stdio.h>
#include <string.h>

#include <badge_pins.h>
#include <badge_eink.h>
#include <badge_eink_fb.h>
#include <badge_power.h>
#include <badge_input.h>

#include <font.h>

void
demoPower(void) {
	char text[32];

	bool bat_cs = false;
	int v_bat = -1;
	int v_usb = -1;

	while (1)
	{
#if defined(PORTEXP_PIN_NUM_CHRGSTAT) || defined(MPR121_PIN_NUM_CHRGSTAT)
		bool new_bat_cs = badge_battery_charge_status();
#else
		bool new_bat_cs = false;
#endif
#ifdef ADC1_CHAN_VBAT_SENSE
		int new_v_bat = badge_battery_volt_sense();
#else
		int new_v_bat = 0;
#endif
#ifdef ADC1_CHAN_VBAT_SENSE
		int new_v_usb = badge_usb_volt_sense();
#else
		int new_v_usb = 0;
#endif

		if (bat_cs != new_bat_cs ||
			v_bat != new_v_bat || v_usb != new_v_usb)
		{
			bat_cs = new_bat_cs;
			v_bat = new_v_bat;
			v_usb = new_v_usb;

			memset(badge_eink_fb, 0xff, sizeof(badge_eink_fb));

			snprintf(text, sizeof(text), "Is charging: %s", bat_cs ? "true" : "false");
			draw_font(badge_eink_fb, 16,  8, BADGE_EINK_WIDTH-32, text,
					FONT_MONOSPACE | FONT_INVERT);

			snprintf(text, sizeof(text), "Vusb       : %d.%03d V", v_usb / 1000, v_usb % 1000);
			draw_font(badge_eink_fb, 16, 16, BADGE_EINK_WIDTH-32, text,
					FONT_MONOSPACE | FONT_INVERT);

			snprintf(text, sizeof(text), "Vbat       : %d.%03d V", v_bat / 1000, v_bat % 1000);
			draw_font(badge_eink_fb, 16, 24, BADGE_EINK_WIDTH-32, text,
					FONT_MONOSPACE | FONT_INVERT);

			/* update display */
			badge_eink_display(badge_eink_fb, DISPLAY_FLAG_LUT(2));
		}

		// wait 1 second
		if (badge_input_get_event(1000) != 0)
			return;
	}
}
