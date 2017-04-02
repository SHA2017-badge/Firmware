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

#ifdef PIN_NUM_I2C_CLOCK

#define PORTEXP_PIN_NUM_LEDS 2

spi_device_handle_t badge_leds_spi = NULL;

int
badge_leds_set_state(uint8_t *rgb)
{
	const uint8_t conv[4] = { 0x11, 0x13, 0x31, 0x33 };
	uint8_t data[75];
	int i,j;
	j=0;
	// 3 * 24 us 'reset'
	data[j++] = 0;
	data[j++] = 0;
	data[j++] = 0;
	int k=0;
	for (i=5; i>=0; i--) {
		int r = rgb[i*3+0];
		int g = rgb[i*3+1];
		int b = rgb[i*3+2];
		if (r || g || b)
			k++;
		data[j++] = conv[(g>>6)&3];
		data[j++] = conv[(g>>4)&3];
		data[j++] = conv[(g>>2)&3];
		data[j++] = conv[(g>>0)&3];
		data[j++] = conv[(r>>6)&3];
		data[j++] = conv[(r>>4)&3];
		data[j++] = conv[(r>>2)&3];
		data[j++] = conv[(r>>0)&3];
		data[j++] = conv[(b>>6)&3];
		data[j++] = conv[(b>>4)&3];
		data[j++] = conv[(b>>2)&3];
		data[j++] = conv[(b>>0)&3];
	}

	if (k == 0)
	{
		return badge_portexp_set_output_state(PORTEXP_PIN_NUM_LEDS, 0);
	}
	else
	{
		int res = badge_portexp_set_output_state(PORTEXP_PIN_NUM_LEDS, 1);
		if (res == -1)
			return -1;

		spi_transaction_t t;
		memset(&t, 0, sizeof(t));
		t.length = j*8;
		t.tx_buffer = data;

		esp_err_t ret = spi_device_transmit(badge_leds_spi, &t);
		return (ret == ESP_OK) ? 0 : -1;
	}
}

void
badge_leds_init(void)
{
	// enable power to led-bar
	badge_portexp_set_output_state(PORTEXP_PIN_NUM_LEDS, 0);
	badge_portexp_set_output_high_z(PORTEXP_PIN_NUM_LEDS, 0);
	badge_portexp_set_io_direction(PORTEXP_PIN_NUM_LEDS, 1);

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

#endif // PIN_NUM_I2C_CLOCK
