#include <stdbool.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"

#include "rom/ets_sys.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "badge_pins.h"
#include "badge_i2c.h"
#include "badge_portexp.h"
#include "badge_mpr121.h"

#ifdef PIN_NUM_LEDS

spi_device_handle_t badge_leds_spi = NULL;

int
badge_leds_enable(void)
{
#ifdef PORTEXP_PIN_NUM_LEDS
	return badge_portexp_set_output_state(PORTEXP_PIN_NUM_LEDS, 1);
#elif defined(MPR121_PIN_NUM_LEDS)
	return badge_mpr121_set_gpio_level(MPR121_PIN_NUM_LEDS, 1);
#endif
	return -1;
}

int
badge_leds_disable(void)
{
#ifdef PORTEXP_PIN_NUM_LEDS
	return badge_portexp_set_output_state(PORTEXP_PIN_NUM_LEDS, 0);
#elif defined(MPR121_PIN_NUM_LEDS)
	return badge_mpr121_set_gpio_level(MPR121_PIN_NUM_LEDS, 0);
#endif
	return -1;
}

uint8_t *badge_leds_buf = NULL;
int badge_leds_buf_len = 0;

int
badge_leds_send_data(uint8_t *data, int len)
{
	static const uint8_t conv[4] = { 0x11, 0x13, 0x31, 0x33 };

	int res = badge_leds_enable();
	if (res == -1)
		return -1;

	if (badge_leds_buf_len < len * 4 + 3)
	{
		if (badge_leds_buf != NULL)
			free(badge_leds_buf);
		badge_leds_buf_len = 0;
		badge_leds_buf = malloc(len * 4 + 3);
		if (badge_leds_buf == NULL)
			return -1; // out of memory
	}

	// 3 * 24 us 'reset'
	int pos=0;
	badge_leds_buf[pos++] = 0;
	badge_leds_buf[pos++] = 0;
	badge_leds_buf[pos++] = 0;

	int i;
	for (i=0; i<len; i++)
	{
		int v = rgbw[i];
#ifdef CONFIG_SHA_BADGE_LEDS_WS2812
		// the WS2812 doesn't have a white led; evenly distribute over other leds.
		if (i < 6*4) // only do conversion for the internal leds
		{
			if ((i & 3) == 3)
				continue; // skip the white pixel
			int w = ((i|3) < len) ? rgbw[i|3] : 0;
			v += w;
			if (v > 255)
				v = 255;
		}
#endif // CONFIG_SHA_BADGE_LEDS_WS2812

		badge_leds_buf[pos++] = conv[(v>>6)&3];
		badge_leds_buf[pos++] = conv[(v>>4)&3];
		badge_leds_buf[pos++] = conv[(v>>2)&3];
		badge_leds_buf[pos++] = conv[(v>>0)&3];
	}

	spi_transaction_t t;
	memset(&t, 0, sizeof(t));
	t.length = pos*8;
	t.tx_buffer = badge_leds_buf;

	esp_err_t ret = spi_device_transmit(badge_leds_spi, &t);
	return (ret == ESP_OK) ? 0 : -1;
}

int
badge_leds_set_state(uint8_t *rgbw)
{
	uint8_t buf[6*4];

	int pos=0;
	bool all_zero = true;
	int i;
	for (i=5; i>=0; i--)
	{
		int r = rgbw[i*4+0];
		int g = rgbw[i*4+1];
		int b = rgbw[i*4+2];
		int w = rgbw[i*4+3];
		buf[pos++] = g;
		buf[pos++] = r;
		buf[pos++] = b;
		buf[pos++] = w;
		if (r|g|b|w)
			all_zero = false;
	}

	if (all_zero)
	{
		return badge_leds_disable();
	}

	return badge_leds_send_data(buf, sizeof(buf));
}

void
badge_leds_init(void)
{
	// enable power to led-bar
#ifdef PORTEXP_PIN_NUM_LEDS
	badge_portexp_set_output_state(PORTEXP_PIN_NUM_LEDS, 0);
	badge_portexp_set_output_high_z(PORTEXP_PIN_NUM_LEDS, 0);
	badge_portexp_set_io_direction(PORTEXP_PIN_NUM_LEDS, 1);
#elif defined(MPR121_PIN_NUM_LEDS)
	badge_mpr121_configure_gpio(MPR121_PIN_NUM_LEDS, MPR121_OUTPUT);
#endif

	spi_bus_config_t buscfg = {
		.mosi_io_num   = PIN_NUM_LEDS,
		.miso_io_num   = -1,  // -1 = unused
		.sclk_io_num   = -1,  // -1 = unused
		.quadwp_io_num = -1,  // -1 = unused
		.quadhd_io_num = -1,  // -1 = unused
	};
	esp_err_t ret = spi_bus_initialize(HSPI_HOST, &buscfg, 1);
	assert( ret == ESP_OK );

	spi_device_interface_config_t devcfg = {
//		.clock_speed_hz = 3333333, // 3.33 Mhz -- too fast?
		.clock_speed_hz = 3200000, // 3.2 Mhz -- works.. :-)
		.mode           = 0,
		.spics_io_num   = -1,
		.queue_size     = 7,
	};
	ret = spi_bus_add_device(HSPI_HOST, &devcfg, &badge_leds_spi);
	assert( ret == ESP_OK );
}

#endif // PIN_NUM_LEDS
