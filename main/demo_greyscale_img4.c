#include "sdkconfig.h"

#include <freertos/FreeRTOS.h>
#include <esp_event.h>

#include <badge_eink.h>
#include <badge_input.h>

#include "imgv2_hacking.h"

void
demoGreyscaleImg4(void)
{
	badge_eink_display(imgv2_hacking, DISPLAY_FLAG_GREYSCALE);

	// wait for random keypress
	uint32_t buttons_down = 0;
	while ((buttons_down & 0xffff) == 0)
		xQueueReceive(badge_input_queue, &buttons_down, portMAX_DELAY);
}
