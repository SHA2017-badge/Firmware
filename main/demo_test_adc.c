#include "sdkconfig.h"

#include <string.h>

#include <rom/ets_sys.h>
#include <driver/adc.h>

#include <badge_eink.h>
#include <badge_eink_fb.h>
#include <badge_input.h>

#include <font.h>

void
demoTestAdc(void) {
	int channel;
	int adc_mask = 0xff;

	adc1_config_width(ADC_WIDTH_12Bit);
	for (channel = 0; channel < ADC1_CHANNEL_MAX; channel++)
	{
		if (adc_mask & (1 << channel))
		{
			adc1_config_channel_atten(channel, ADC_ATTEN_0db);
		}
	}

	while (1)
	{
		memset(badge_eink_fb, 0xff, sizeof(badge_eink_fb));

		for (channel = 0; channel < ADC1_CHANNEL_MAX; channel++)
		{
#define TEXTLEN 32
			char text[TEXTLEN];
			int val;
			if (adc_mask & (1 << channel))
				val = adc1_get_voltage(channel);
			else
				val = -1;
			snprintf(text, TEXTLEN, "ADC channel %d: %d", channel, val);
			draw_font(badge_eink_fb, 16, 8+8*channel, BADGE_EINK_WIDTH-32, text,
					FONT_FULL_WIDTH | FONT_MONOSPACE | FONT_INVERT);
			ets_printf("%s\n", text);
		}

		/* update display */
		badge_eink_display(badge_eink_fb, DISPLAY_FLAG_LUT(0));

		// wait 1 second
		if (badge_input_get_event(1000) != 0)
			return;
	}
}
