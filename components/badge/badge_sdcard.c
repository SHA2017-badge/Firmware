#include <stdbool.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"

#include "badge_pins.h"
#include "badge_portexp.h"
#include "badge_mpr121.h"
#include "badge_sdcard.h"

#if defined(PORTEXP_PIN_NUM_SD_CD) || defined(MPR121_PIN_NUM_SD_CD)
bool
badge_sdcard_detected(void)
{
#ifdef PORTEXP_PIN_NUM_SD_CD
	return (badge_portexp_get_input() >> PORTEXP_PIN_NUM_SD_CD) & 1;
#elif defined(MPR121_PIN_NUM_SD_CD)
	return badge_mpr121_get_gpio_level(MPR121_PIN_NUM_SD_CD);
#endif
}
#endif // defined(PORTEXP_PIN_NUM_SD_CD) || defined(MPR121_PIN_NUM_SD_CD)

void
badge_sdcard_init(void)
{
	// configure charge-stat pin
#ifdef PORTEXP_PIN_NUM_SD_CD
	badge_portexp_set_io_direction(PORTEXP_PIN_NUM_SD_CD, 0);
	badge_portexp_set_input_default_state(PORTEXP_PIN_NUM_SD_CD, 0);
	badge_portexp_set_pull_enable(PORTEXP_PIN_NUM_SD_CD, 0);
	badge_portexp_set_interrupt_enable(PORTEXP_PIN_NUM_SD_CD, 0);
#elif defined(MPR121_PIN_NUM_SD_CD)
	badge_mpr121_configure_gpio(MPR121_PIN_NUM_SD_CD, MPR121_INPUT);
#endif
}
