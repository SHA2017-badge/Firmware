#include <sdkconfig.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <esp_log.h>

#include "badge_pins.h"
#include "badge_portexp.h"
#include "badge_mpr121.h"
#include "badge_sdcard.h"

static const char *TAG = "badge_sdcard";

#if defined(PORTEXP_PIN_NUM_SD_CD) || defined(MPR121_PIN_NUM_SD_CD)
bool
badge_sdcard_detected(void)
{
#ifdef PORTEXP_PIN_NUM_SD_CD
	return (badge_portexp_get_input() >> PORTEXP_PIN_NUM_SD_CD) & 1;
#elif defined(MPR121_PIN_NUM_SD_CD)
	return badge_mpr121_get_gpio_level(MPR121_PIN_NUM_SD_CD);
#endif
}
#endif // defined(PORTEXP_PIN_NUM_SD_CD) || defined(MPR121_PIN_NUM_SD_CD)

esp_err_t
badge_sdcard_init(void)
{
	static bool badge_sdcard_init_done = false;
	esp_err_t res;

	if (badge_sdcard_init_done)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	// configure charge-stat pin
#ifdef PORTEXP_PIN_NUM_SD_CD
	res = badge_portexp_init();
	if (res != ESP_OK)
		return res;
	res = badge_portexp_set_io_direction(PORTEXP_PIN_NUM_SD_CD, 0);
	if (res != ESP_OK)
		return res;
	res = badge_portexp_set_input_default_state(PORTEXP_PIN_NUM_SD_CD, 0);
	if (res != ESP_OK)
		return res;
	res = badge_portexp_set_pull_enable(PORTEXP_PIN_NUM_SD_CD, 0);
	if (res != ESP_OK)
		return res;
	res = badge_portexp_set_interrupt_enable(PORTEXP_PIN_NUM_SD_CD, 0);
	if (res != ESP_OK)
		return res;
#elif defined(MPR121_PIN_NUM_SD_CD)
	res = badge_mpr121_init();
	if (res != ESP_OK)
		return res;
	res = badge_mpr121_configure_gpio(MPR121_PIN_NUM_SD_CD, MPR121_INPUT);
	if (res != ESP_OK)
		return res;
#endif

	badge_sdcard_init_done = true;

	return ESP_OK;
}
