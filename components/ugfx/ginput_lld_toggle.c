/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include <sdkconfig.h>

#include "gfx.h"

#if (GFX_USE_GINPUT && GINPUT_NEED_TOGGLE)
#include "../../../src/ginput/ginput_driver_toggle.h"

#include <badge_input.h>

#include "ginput_lld_toggle_config.h"

GINPUT_TOGGLE_DECLARE_STRUCTURE();

void
ginput_lld_toggle_init(const GToggleConfig *ptc)
{
#ifdef CONFIG_SHA_BADGE_UGFX_GINPUT_DEBUG
	ets_printf("ginput_lld_toggle: init()\n");
#endif // CONFIG_SHA_BADGE_UGFX_GINPUT_DEBUG

	badge_input_notify = ginputToggleWakeupI;
}

unsigned
ginput_lld_toggle_getbits(const GToggleConfig *ptc)
{
#ifdef CONFIG_SHA_BADGE_UGFX_GINPUT_DEBUG
	ets_printf("ginput_lld_toggle: getbits()\n");
#endif // CONFIG_SHA_BADGE_UGFX_GINPUT_DEBUG

	// drain input queue
	while (badge_input_get_event(0) != 0);

	// pass on button state
	return badge_input_button_state;
}

#endif /*  GFX_USE_GINPUT && GINPUT_NEED_TOGGLE */
