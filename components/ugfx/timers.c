#include "gfx.h"

// https://community.ugfx.io/topic/99-using-ugfx-on-bare-metal-with-coocox-coide/#comment-785
systemticks_t gfxSystemTicks(void)
{
	return 0;
}

systemticks_t gfxMillisecondsToTicks(delaytime_t ms)
{
	return ms;
}
