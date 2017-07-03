#include "sdkconfig.h"

#include <stdbool.h>

#include <driver/gpio.h>

#include "badge_base.h"

void
badge_base_init(void)
{
	static bool badge_base_init_done = false;

	if (badge_base_init_done)
		return;

	// install isr-service, so we can register interrupt-handlers per
	// gpio pin.
	gpio_install_isr_service(0);

	badge_base_init_done = true;
}
