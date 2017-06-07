#include "sdkconfig.h"

#include <driver/gpio.h>

#include "badge_pins.h"
#include "badge_input.h"
#include "badge_button.h"
#include "badge_i2c.h"
#include "badge_portexp.h"
#include "badge_mpr121.h"
#include "badge_touch.h"
#include "badge_power.h"
#include "badge_leds.h"
#include "badge_eink.h"

#ifdef I2C_TOUCHPAD_ADDR
void
touch_event_handler(int event)
{
	// convert into button queue event
	if (((event >> 16) & 0x0f) == 0x0) { // button down event
		static const int conv[12] = {
			-1,
			-1,
			BADGE_BUTTON_LEFT,
			BADGE_BUTTON_UP,
			BADGE_BUTTON_RIGHT,
			BADGE_BUTTON_DOWN,
			-1,
			BADGE_BUTTON_SELECT,
			BADGE_BUTTON_START,
			BADGE_BUTTON_B,
			-1,
			BADGE_BUTTON_A,
		};
		if (((event >> 8) & 0xff) < 12) {
			int id = conv[(event >> 8) & 0xff];
			if (id != -1)
			{
				uint32_t button_down = id;
				xQueueSend(badge_input_queue, &button_down, 0);
			}
		}
	}
}
#endif // I2C_TOUCHPAD_ADDR

#ifdef I2C_MPR121_ADDR
void
mpr121_event_handler(void *b)
{
	uint32_t button_down = (int) b;
	xQueueSend(badge_input_queue, &button_down, 0);
}
#endif // I2C_MPR121_ADDR

void
badge_init(void)
{
	// install isr-service, so we can register interrupt-handlers per
	// gpio pin.
	gpio_install_isr_service(0);

	// configure input queue
	badge_input_init();

	// configure buttons directly connected to gpio pins
#ifdef PIN_NUM_BUTTON_A
	badge_button_add(PIN_NUM_BUTTON_A    , BADGE_BUTTON_A);
	badge_button_add(PIN_NUM_BUTTON_B    , BADGE_BUTTON_B);
	badge_button_add(PIN_NUM_BUTTON_MID  , BADGE_BUTTON_MID);
	badge_button_add(PIN_NUM_BUTTON_UP   , BADGE_BUTTON_UP);
	badge_button_add(PIN_NUM_BUTTON_DOWN , BADGE_BUTTON_DOWN);
	badge_button_add(PIN_NUM_BUTTON_LEFT , BADGE_BUTTON_LEFT);
	badge_button_add(PIN_NUM_BUTTON_RIGHT, BADGE_BUTTON_RIGHT);
#else
	badge_button_add(PIN_NUM_BUTTON_FLASH, BADGE_BUTTON_FLASH);
#endif // ! PIN_NUM_BUTTON_A

	// configure the i2c bus to the port-expander and touch-controller or to the mpr121
#ifdef PIN_NUM_I2C_CLK
	badge_i2c_init();
#endif // PIN_NUM_I2C_CLK

#ifdef I2C_PORTEXP_ADDR
	badge_portexp_init();
#endif // I2C_PORTEXP_ADDR

#ifdef I2C_TOUCHPAD_ADDR
	badge_touch_init();
	badge_touch_set_event_handler(touch_event_handler);
#endif // I2C_TOUCHPAD_ADDR

#ifdef I2C_MPR121_ADDR
	badge_mpr121_init();
	badge_mpr121_set_interrupt_handler(MPR121_PIN_NUM_A     , mpr121_event_handler, (void*) (BADGE_BUTTON_A));
	badge_mpr121_set_interrupt_handler(MPR121_PIN_NUM_B     , mpr121_event_handler, (void*) (BADGE_BUTTON_B));
	badge_mpr121_set_interrupt_handler(MPR121_PIN_NUM_START , mpr121_event_handler, (void*) (BADGE_BUTTON_START));
	badge_mpr121_set_interrupt_handler(MPR121_PIN_NUM_SELECT, mpr121_event_handler, (void*) (BADGE_BUTTON_SELECT));
	badge_mpr121_set_interrupt_handler(MPR121_PIN_NUM_DOWN  , mpr121_event_handler, (void*) (BADGE_BUTTON_DOWN));
	badge_mpr121_set_interrupt_handler(MPR121_PIN_NUM_RIGHT , mpr121_event_handler, (void*) (BADGE_BUTTON_RIGHT));
	badge_mpr121_set_interrupt_handler(MPR121_PIN_NUM_UP    , mpr121_event_handler, (void*) (BADGE_BUTTON_UP));
	badge_mpr121_set_interrupt_handler(MPR121_PIN_NUM_LEFT  , mpr121_event_handler, (void*) (BADGE_BUTTON_LEFT));
#endif // I2C_MPR121_ADDR

	// configure the voltage measuring for charging-info feedback
	badge_power_init();

	// configure the led-strip on top of the badge
#ifdef PIN_NUM_LEDS
	badge_leds_init();
#endif // PIN_NUM_LEDS

	// configure eink display
	badge_eink_init();
}
