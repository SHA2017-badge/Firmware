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

#include "ginput_lld_toggle_config.h"

GINPUT_TOGGLE_DECLARE_STRUCTURE();

void
ginput_lld_toggle_init(const GToggleConfig *ptc)
{
	ets_printf("ginput_lld_toggle: init()\n");
}

unsigned
ginput_lld_toggle_getbits(const GToggleConfig *ptc)
{
	ets_printf("ginput_lld_toggle: getbits()\n");
    uint32_t button_down = 0;
    if (xQueueReceive(badge_input_queue, &button_down, 1000 / portTICK_RATE_MS))
	{
		ets_printf("ginput_lld_toggle: button %d pressed\n", button_down);
		return 1 << button_down;
	}
	ets_printf("ginput_lld_toggle: no buttons pressed (timeout)\n");
	return 0;
}

#endif /*  GFX_USE_GINPUT && GINPUT_NEED_TOGGLE */