#include "sdkconfig.h"

#include <badge_eink.h>
#include <badge_input.h>

#include "imgv2_hacking.h"

void
demoGreyscaleImg4(void)
{
	badge_eink_display(imgv2_hacking, DISPLAY_FLAG_GREYSCALE);

	// wait for random keypress
	badge_input_get_event(-1);
}
