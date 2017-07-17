#include <sdkconfig.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <rom/ets_sys.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/spi_master.h>

#include "badge_pins.h"
#include "badge_i2c.h"
#include "badge_portexp.h"
#include "badge_mpr121.h"
#include "badge_vibrator.h"

static const char *TAG = "badge_vibrator";

#if defined(PORTEXP_PIN_NUM_VIBRATOR) || defined(MPR121_PIN_NUM_VIBRATOR)

static int
badge_vibrator_on(void)
{
#ifdef PORTEXP_PIN_NUM_VIBRATOR
	return badge_portexp_set_output_state(PORTEXP_PIN_NUM_VIBRATOR, 1);
#elif defined(MPR121_PIN_NUM_VIBRATOR)
	return badge_mpr121_set_gpio_level(MPR121_PIN_NUM_VIBRATOR, 1);
#endif
}

static int
badge_vibrator_off(void)
{
#ifdef PORTEXP_PIN_NUM_VIBRATOR
	return badge_portexp_set_output_state(PORTEXP_PIN_NUM_VIBRATOR, 0);
#elif defined(MPR121_PIN_NUM_VIBRATOR)
	return badge_mpr121_set_gpio_level(MPR121_PIN_NUM_VIBRATOR, 0);
#endif
}

void
badge_vibrator_activate(uint32_t pattern)
{
	while (pattern != 0)
	{
		if ((pattern & 1) == 0)
			badge_vibrator_off();
		else
			badge_vibrator_on();
		pattern >>= 1;
		ets_delay_us(200000);
	}
	badge_vibrator_off();
}

esp_err_t
badge_vibrator_init(void)
{
	static bool badge_vibrator_init_done = false;
	esp_err_t res;

	if (badge_vibrator_init_done)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	// configure vibrator pin
#ifdef PORTEXP_PIN_NUM_VIBRATOR
	res = badge_portexp_init();
	if (res != ESP_OK)
		return res;
	res = badge_portexp_set_output_state(PORTEXP_PIN_NUM_VIBRATOR, 0);
	if (res != ESP_OK)
		return res;
	res = badge_portexp_set_output_high_z(PORTEXP_PIN_NUM_VIBRATOR, 0);
	if (res != ESP_OK)
		return res;
	res = badge_portexp_set_io_direction(PORTEXP_PIN_NUM_VIBRATOR, 1);
	if (res != ESP_OK)
		return res;
#elif defined(MPR121_PIN_NUM_VIBRATOR)
	res = badge_mpr121_init();
	if (res != ESP_OK)
		return res;
	res = badge_mpr121_configure_gpio(MPR121_PIN_NUM_VIBRATOR, MPR121_OUTPUT);
	if (res != ESP_OK)
		return res;
#endif

	badge_vibrator_init_done = true;

	return ESP_OK;
}

#endif // defined(PORTEXP_PIN_NUM_VIBRATOR) || defined(MPR121_PIN_NUM_VIBRATOR)
