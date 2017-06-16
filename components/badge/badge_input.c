#include <sdkconfig.h>

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
#ifdef CONFIG_SHA_BADGE_INPUT_DEBUG
	const char *button_name[11] = {
		"(null)",
		"UP",
		"DOWN",
		"LEFT",
		"RIGHT",
		"MID",
		"A",
		"B",
		"SELECT",
		"START",
		"FLASH",
	};
	ets_printf("badge_input: Button %s %s.\n", button_name[button_id < 11 ? button_id : 0], pressed ? "pressed" : "released");
#endif // CONFIG_SHA_BADGE_INPUT_DEBUG
	if (pressed)
	{
		if (in_isr)
			xQueueSendFromISR(badge_input_queue, &button_id, NULL);
		else
			xQueueSend(badge_input_queue, &button_id, 0);
	}
	if (badge_input_notify != NULL)
		badge_input_notify();
}

uint32_t
badge_input_get_event(int timeout)
{
	int xqueuetimeout = (timeout == -1) ? portMAX_DELAY : timeout / portTICK_RATE_MS;
	uint32_t button_id;
	if (xQueueReceive(badge_input_queue, &button_id, xqueuetimeout)) {
		return button_id;
	}
	return 0;
}
