#include <stdbool.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>

#include <driver/adc.h>

#include "sdkconfig.h"

#include "badge_pins.h"
#include "badge_portexp.h"
#include "badge_mpr121.h"
#include "badge_power.h"

int
badge_battery_volt_sense(void)
{
#ifdef ADC1_CHAN_VBAT_SENSE
	int val = adc1_get_voltage(ADC1_CHAN_VBAT_SENSE);
	if (val == -1)
		return -1;

	return (val * 220 * 32) >> 12;
#else
	return -1;
#endif // ADC1_CHAN_VBAT_SENSE
}

int
badge_usb_volt_sense(void)
{
#ifdef ADC1_CHAN_VUSB_SENSE
	int val = adc1_get_voltage(ADC1_CHAN_VUSB_SENSE);
	if (val == -1)
		return -1;

	return (val * 220 * 32) >> 12;
#else
	return -1;
#endif // ADC1_CHAN_VUSB_SENSE
}

bool
badge_battery_charge_status(void)
{
#ifdef PORTEXP_PIN_NUM_CHRGSTAT
	return ((badge_portexp_get_input() >> PORTEXP_PIN_NUM_CHRGSTAT) & 1) == 0;
#elif defined(MPR121_PIN_NUM_CHRGSTAT)
	return badge_mpr121_get_gpio_level(MPR121_PIN_NUM_CHRGSTAT) == 0;
#else
	return false;
#endif
}

// shared power
static int badge_power_leds_sdcard = 0; // bit 0 = leds, bit 1 = sd-card

int
badge_power_leds_enable(void)
{
	if (badge_power_leds_sdcard & 1)
		return 0;

	if (badge_power_leds_sdcard)
	{
		badge_power_leds_sdcard |= 1;
		return 0;
	}

	int ret;
#ifdef PORTEXP_PIN_NUM_LEDS
	ret = badge_portexp_set_output_state(PORTEXP_PIN_NUM_LEDS, 1);
#elif defined(MPR121_PIN_NUM_LEDS)
	ret = badge_mpr121_set_gpio_level(MPR121_PIN_NUM_LEDS, 1);
#else
	ret = -1;
#endif

	if (ret == 0)
		badge_power_leds_sdcard = 1;

	return ret;
}

int
badge_power_leds_disable(void)
{
	if ((badge_power_leds_sdcard & 1) == 0)
		return 0;

	if (badge_power_leds_sdcard != 1)
	{
		badge_power_leds_sdcard &= ~1;
		return 0;
	}

	int ret;
#ifdef PORTEXP_PIN_NUM_LEDS
	ret = badge_portexp_set_output_state(PORTEXP_PIN_NUM_LEDS, 0);
#elif defined(MPR121_PIN_NUM_LEDS)
	ret = badge_mpr121_set_gpio_level(MPR121_PIN_NUM_LEDS, 0);
#else
	ret = -1;
#endif

	if (ret == 0)
		badge_power_leds_sdcard = 0;

	return ret;
}

int
badge_power_sdcard_enable(void)
{
	if (badge_power_leds_sdcard)
	{
		badge_power_leds_sdcard |= 2;
		return 0;
	}

	int ret;
#ifdef PORTEXP_PIN_NUM_LEDS
	ret = badge_portexp_set_output_state(PORTEXP_PIN_NUM_LEDS, 1);
#elif defined(MPR121_PIN_NUM_LEDS)
	ret = badge_mpr121_set_gpio_level(MPR121_PIN_NUM_LEDS, 1);
#else
	ret = -1;
#endif

	if (ret == 0)
		badge_power_leds_sdcard = 2;

	return ret;
}

int
badge_power_sdcard_disable(void)
{
	if ((badge_power_leds_sdcard & 2) == 0)
		return 0;

	if (badge_power_leds_sdcard != 2)
	{
		badge_power_leds_sdcard &= ~2;
		return 0;
	}

	int ret;
#ifdef PORTEXP_PIN_NUM_LEDS
	ret = badge_portexp_set_output_state(PORTEXP_PIN_NUM_LEDS, 0);
#elif defined(MPR121_PIN_NUM_LEDS)
	ret = badge_mpr121_set_gpio_level(MPR121_PIN_NUM_LEDS, 0);
#else
	ret = -1;
#endif

	if (ret == 0)
		badge_power_leds_sdcard = 0;

	return ret;
}

void
badge_power_init(void)
{
	static bool badge_power_init_done = false;

	if (badge_power_init_done)
		return;

	// configure adc width
#if defined(ADC1_CHAN_VBAT_SENSE) || defined(ADC1_CHAN_VUSB_SENSE)
	adc1_config_width(ADC_WIDTH_12Bit);
#endif // defined(ADC1_CHAN_VBAT_SENSE) || defined(ADC1_CHAN_VUSB_SENSE)

	// configure vbat-sense
#ifdef ADC1_CHAN_VBAT_SENSE
	// When VDD_A is 3.3V:
	// 6dB attenuation (ADC_ATTEN_6db) gives full-scale voltage 2.2V
	adc1_config_channel_atten(ADC1_CHAN_VBAT_SENSE, ADC_ATTEN_6db);
#endif // ADC1_CHAN_VBAT_SENSE

	// configure vusb-sense
#ifdef ADC1_CHAN_VUSB_SENSE
	// When VDD_A is 3.3V:
	// 6dB attenuation (ADC_ATTEN_6db) gives full-scale voltage 2.2V
	adc1_config_channel_atten(ADC1_CHAN_VUSB_SENSE, ADC_ATTEN_6db);
#endif // ADC1_CHAN_VUSB_SENSE

	// configure charge-stat pin
#ifdef PORTEXP_PIN_NUM_CHRGSTAT
	badge_portexp_set_io_direction(PORTEXP_PIN_NUM_CHRGSTAT, 0);
	badge_portexp_set_input_default_state(PORTEXP_PIN_NUM_CHRGSTAT, 0);
	badge_portexp_set_pull_enable(PORTEXP_PIN_NUM_CHRGSTAT, 0);
	badge_portexp_set_interrupt_enable(PORTEXP_PIN_NUM_CHRGSTAT, 0);
#elif defined(MPR121_PIN_NUM_CHRGSTAT)
	badge_mpr121_configure_gpio(MPR121_PIN_NUM_CHRGSTAT, MPR121_INPUT);
#endif

	// configure power to the leds and sd-card
#ifdef PORTEXP_PIN_NUM_LEDS
	badge_portexp_set_output_state(PORTEXP_PIN_NUM_LEDS, 0);
	badge_portexp_set_output_high_z(PORTEXP_PIN_NUM_LEDS, 0);
	badge_portexp_set_io_direction(PORTEXP_PIN_NUM_LEDS, 1);
#elif defined(MPR121_PIN_NUM_LEDS)
	badge_mpr121_configure_gpio(MPR121_PIN_NUM_LEDS, MPR121_OUTPUT);
#endif

	badge_power_init_done = true;
}
