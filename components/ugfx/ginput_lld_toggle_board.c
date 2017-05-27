#include "gfx.h"

#if (GFX_USE_GINPUT && GINPUT_NEED_TOGGLE)

#include "ginput/ginput_driver_toggle.h"

GINPUT_TOGGLE_DECLARE_STRUCTURE();

void ginput_badge_toggle_init(const GToggleConfig *ptc) {
  ets_printf("Initializing badge toggles\n");
}

unsigned ginput_badge_toggle_getbits(const GToggleConfig *ptc) {
  ets_printf("Getting toggle bits\n");
  return 0;
}

#endif /* GFX_USE_GINPUT && GINPUT_NEED_TOGGLE */
