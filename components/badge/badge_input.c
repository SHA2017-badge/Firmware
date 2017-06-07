#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <esp_event.h>

#include "badge_input.h"

xQueueHandle badge_input_queue = NULL;

void
badge_input_init(void)
{
	badge_input_queue = xQueueCreate(10, sizeof(uint32_t));
}
