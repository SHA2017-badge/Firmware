#include "sdkconfig.h"

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <esp_event.h>
#include <driver/adc.h>

#include <badge_eink.h>
#include <badge_input.h>

#include <font.h>

// re-use screen_buf from main.c
extern uint8_t screen_buf[296*16];

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
		memset(screen_buf, 0xff, sizeof(screen_buf));

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
			draw_font(screen_buf, 16, 8+8*channel, BADGE_EINK_WIDTH-32, text,
					FONT_FULL_WIDTH | FONT_MONOSPACE | FONT_INVERT);
			ets_printf("%s\n", text);
		}

		/* update display */
		badge_eink_display(screen_buf, (1 << DISPLAY_FLAG_LUT_BIT));

		uint32_t buttons_down = 0;
		if (xQueueReceive(badge_input_queue, &buttons_down, portMAX_DELAY))
			if ((buttons_down & 0xffff) != 0)
				return;
	}
}
