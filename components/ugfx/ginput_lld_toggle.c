/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "gfx.h"

#if (GFX_USE_GINPUT && GINPUT_NEED_TOGGLE)
#include "../../../src/ginput/ginput_driver_toggle.h"

#include <freertos/FreeRTOS.h>
#include <esp_event.h>

#include <badge_input.h>
#include <badge_pins.h>
#include <badge_button.h>

#include "ginput_lld_toggle_config.h"

GINPUT_TOGGLE_DECLARE_STRUCTURE();

void
ginput_toggle_interrupt_handler(void *arg) {
#ifdef PIN_NUM_BUTTON_A
	badge_button_handler(arg);
#endif /* PIN_NUM_BUTTON_A */

#ifdef I2C_TOUCHPAD_ADDR
	badge_touch_intr_handler(arg);
#endif /* I2C_TOUCHPAD_ADDR */

	ginputToggleWakeupI();
}

void
register_button_interrupt_handler(int gpio_num) {
	gpio_isr_handler_add(gpio_num, ginput_toggle_interrupt_handler, (void*) gpio_num);

	// configure the gpio pin for input
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_ANYEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = 1LL << gpio_num;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);
}

void
ginput_lld_toggle_init(const GToggleConfig *ptc)
{
	ets_printf("ginput_lld_toggle: init()\n");
#ifdef PIN_NUM_BUTTON_A
	register_button_interrupt_handler(PIN_NUM_BUTTON_A);
	register_button_interrupt_handler(PIN_NUM_BUTTON_B);
	register_button_interrupt_handler(PIN_NUM_BUTTON_MID);
	register_button_interrupt_handler(PIN_NUM_BUTTON_UP);
	register_button_interrupt_handler(PIN_NUM_BUTTON_DOWN);
	register_button_interrupt_handler(PIN_NUM_BUTTON_LEFT);
	register_button_interrupt_handler(PIN_NUM_BUTTON_RIGHT);
#endif /* PIN_NUM_BUTTON_A */

#ifdef I2C_TOUCHPAD_ADDR
	badge_portexp_set_interrupt_handler(PORTEXP_PIN_NUM_TOUCH, ginput_toggle_interrupt_handler, NULL);
#endif // I2C_TOUCHPAD_ADDR
}

unsigned
ginput_lld_toggle_getbits(const GToggleConfig *ptc)
{
	ets_printf("ginput_lld_toggle: getbits()\n");
	uint32_t result = 0;
    uint32_t button_down = 0;
		// No delay, because we'll be triggered by an interrupt so we know there should be something to read
    while (xQueueReceive(badge_input_queue, &button_down, 0))
	{
		ets_printf("ginput_lld_toggle: button %d pressed\n", button_down);
		result |= 1 << button_down;
	}
	ets_printf("ginput_lld_toggle: no more buttons pressed, final bitmask: %d\n", result);
	return result;
}

#endif /*  GFX_USE_GINPUT && GINPUT_NEED_TOGGLE */
