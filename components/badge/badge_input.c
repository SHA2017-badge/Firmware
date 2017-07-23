#include <sdkconfig.h>

#ifdef CONFIG_SHA_BADGE_INPUT_DEBUG
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif // CONFIG_SHA_BADGE_INPUT_DEBUG

#include <esp_event.h>
#include <esp_log.h>

#include "badge_input.h"

static const char *TAG = "badge_input";

xQueueHandle badge_input_queue = NULL;
void (*badge_input_notify)(void);
uint32_t badge_input_button_state = 0;

esp_err_t
badge_input_init(void)
{
	static bool badge_input_init_done = false;

	if (badge_input_init_done)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	badge_input_queue = xQueueCreate(10, sizeof(uint32_t));
	if (badge_input_queue == NULL)
		return ESP_ERR_NO_MEM;

	badge_input_init_done = true;

	ESP_LOGD(TAG, "init done");

	return ESP_OK;
}

static const char *badge_input_button_name[11] = {
	"(null)",
	"UP",
	"DOWN",
	"LEFT",
	"RIGHT",
	"(null)",
	"A",
	"B",
	"SELECT",
	"START",
	"FLASH",
};

void
badge_input_add_event(uint32_t button_id, bool pressed, bool in_isr)
{ /* maybe in interrupt handler */
#ifdef CONFIG_SHA_BADGE_INPUT_DEBUG
	ets_printf("badge_input: Button %s %s.\n",
			badge_input_button_name[button_id < 11 ? button_id : 0],
			pressed ? "pressed" : "released");
#endif // CONFIG_SHA_BADGE_INPUT_DEBUG

	if (pressed)
	{
		badge_input_button_state |= 1 << button_id;
		if (in_isr)
		{
			if (xQueueSendFromISR(badge_input_queue, &button_id, NULL) != pdTRUE)
			{
				ets_printf("badge_input: input queue full.\n");
			}
		}
		else
		{
			if (xQueueSend(badge_input_queue, &button_id, 0) != pdTRUE)
			{
				ets_printf("badge_input: input queue full.\n");
			}
		}
	}
	else
	{
		badge_input_button_state &= ~(1 << button_id);
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
