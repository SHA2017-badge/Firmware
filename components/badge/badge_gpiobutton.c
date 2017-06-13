#include "sdkconfig.h"

//include <freertos/FreeRTOS.h>
#include <driver/gpio.h>

#include "badge_input.h"
#include "badge_gpiobutton.h"

uint32_t badge_gpiobutton_conv[40] = { 0 };
int badge_gpiobutton_old_state[40] = { 0 };

void
badge_gpiobutton_handler(void *arg)
{
	uint32_t gpio_num = (uint32_t) arg;

	int new_state = gpio_get_level(gpio_num);
	if (new_state != badge_gpiobutton_old_state[gpio_num])
	{
		uint32_t button_id = badge_gpiobutton_conv[gpio_num];
		badge_input_add_event(button_id, new_state == 0 ? EVENT_BUTTON_PRESSED : EVENT_BUTTON_RELEASED, IN_ISR);
	}
	badge_gpiobutton_old_state[gpio_num] = new_state;
}

void
badge_gpiobutton_add(int gpio_num, uint32_t button_id)
{
	badge_gpiobutton_conv[gpio_num] = button_id;
	badge_gpiobutton_old_state[gpio_num] = 1; // released

	gpio_isr_handler_add(gpio_num, badge_gpiobutton_handler, (void*) gpio_num);

	// configure the gpio pin for input
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_ANYEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = 1LL << gpio_num;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);
}
