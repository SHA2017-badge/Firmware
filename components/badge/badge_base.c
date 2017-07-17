#include <sdkconfig.h>

#include <stdbool.h>

#include <esp_log.h>
#include <driver/gpio.h>

#include "badge_base.h"

static const char *TAG = "badge_base";

esp_err_t
badge_base_init(void)
{
	static bool badge_base_init_done = false;

	if (badge_base_init_done)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	// install isr-service, so we can register interrupt-handlers per
	// gpio pin.
	esp_err_t res = gpio_install_isr_service(0);
	if (res == ESP_FAIL)
	{
		ESP_LOGW(TAG, "Failed to install gpio isr service. Ignoring this.");
		res = ESP_OK;
	}

	if (res != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to install gpio isr service: %d", res);
		return res;
	}

	badge_base_init_done = true;

	ESP_LOGD(TAG, "init done");

	return ESP_OK;
}
