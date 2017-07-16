#include <sdkconfig.h>

#include <stdbool.h>

#include <driver/gpio.h>

#include "badge_base.h"

esp_err_t
badge_base_init(void)
{
	static bool badge_base_init_done = false;

	if (badge_base_init_done)
		return ESP_OK;

	// install isr-service, so we can register interrupt-handlers per
	// gpio pin.
	esp_err_t res = gpio_install_isr_service(0);
	if (res != ESP_OK)
		return res;

	badge_base_init_done = true;

	return ESP_OK;
}
