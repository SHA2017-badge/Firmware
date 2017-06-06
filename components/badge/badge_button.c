#include "sdkconfig.h"

#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>

#include "badge_input.h"
#include "badge_button.h"

uint32_t badge_button_conv[40] = { 0 };
int badge_button_old_state[40] = { 0 };

void
badge_button_handler(void *arg)
{
	uint32_t gpio_num = (uint32_t) arg;

	int new_state = gpio_get_level(gpio_num);
	if (new_state == 0 && badge_button_old_state[gpio_num] != 0)
	{
		uint32_t button_down = badge_button_conv[gpio_num];
		xQueueSendFromISR(badge_input_queue, &button_down, NULL);
	}
	badge_button_old_state[gpio_num] = new_state;
}

void
badge_button_add(int gpio_num, uint32_t button_id)
{
	badge_button_conv[gpio_num] = button_id;
	badge_button_old_state[gpio_num] = -1;

	gpio_isr_handler_add(gpio_num, badge_button_handler, (void*) gpio_num);

	// configure the gpio pin for input
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_ANYEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = 1LL << gpio_num;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);
}
