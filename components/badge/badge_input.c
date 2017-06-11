#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <esp_event.h>

#include "badge_input.h"

xQueueHandle badge_input_queue = NULL;
void (*badge_input_notify)(void);

void
badge_input_init(void)
{
	badge_input_queue = xQueueCreate(10, sizeof(uint32_t));
}

void
badge_input_add_event(uint32_t button_id, bool pressed, bool in_isr)
{
	if (pressed)
	{
		if (in_isr)
			xQueueSendFromISR(badge_input_queue, &button_id, NULL);
		else
			xQueueSend(badge_input_queue, &button_id, 0);

		if (badge_input_notify != NULL)
			badge_input_notify();
	}
}
