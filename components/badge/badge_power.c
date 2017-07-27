#include <sdkconfig.h>

#ifdef CONFIG_SHA_BADGE_POWER_DEBUG
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif // CONFIG_SHA_BADGE_POWER_DEBUG

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <esp_log.h>
#include <driver/adc.h>
#include <driver/gpio.h>

#include "badge_pins.h"
#include "badge_fxl6408.h"
#include "badge_mpr121.h"
#include "badge_power.h"

static const char *TAG = "badge_power";

int
badge_battery_volt_sense(void)
{
#ifdef ADC1_CHAN_VBAT_SENSE
	int val = adc1_get_voltage(ADC1_CHAN_VBAT_SENSE);
	if (val == -1)
		return -1;

	return (val * 22 * 9 * 32) >> 12;
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

	return (val * 22 * 9 * 32) >> 12;
#else
	return -1;
#endif // ADC1_CHAN_VUSB_SENSE
}

bool
badge_battery_charge_status(void)
{
#ifdef FXL6408_PIN_NUM_CHRGSTAT
	return ((badge_fxl6408_get_input() >> FXL6408_PIN_NUM_CHRGSTAT) & 1) == 0;
#elif defined(MPR121_PIN_NUM_CHRGSTAT)
	return badge_mpr121_get_gpio_level(MPR121_PIN_NUM_CHRGSTAT) == 0;
#else
	return false;
#endif
}

// shared power
static int badge_power_leds_sdcard = 0; // bit 0 = leds, bit 1 = sd-card

static esp_err_t
badge_power_sdcard_leds_enable(void)
{
	ESP_LOGD(TAG, "enabling power to sdcard and leds.");

	esp_err_t ret;
#ifdef FXL6408_PIN_NUM_LEDS
	ret = badge_fxl6408_set_output_state(FXL6408_PIN_NUM_LEDS, 1);
#elif defined(MPR121_PIN_NUM_LEDS)
	ret = badge_mpr121_set_gpio_level(MPR121_PIN_NUM_LEDS, 1);
#else
	ret = ESP_OK;
#endif

	if (ret != ESP_OK)
	{
		ESP_LOGW(TAG, "failed to enable power.");
	}

	return ret;
}

static esp_err_t
badge_power_sdcard_leds_disable(void)
{
#ifdef PIN_NUM_SD_CLK
	// configure led and sdcard pins as "high-impedance"
	gpio_config_t io_conf = {
		.intr_type    = GPIO_INTR_DISABLE,
		.mode         = GPIO_MODE_INPUT,
		.pin_bit_mask = (0
#ifdef PIN_NUM_LEDS
			| (1LL << PIN_NUM_LEDS)
#endif // PIN_NUM_LEDS
			| (1LL << PIN_NUM_SD_CLK)
			| (1LL << PIN_NUM_SD_CMD)
			| (1LL << PIN_NUM_SD_DATA_0)
#ifdef PIN_NUM_SD_DATA_1
			| (1LL << PIN_NUM_SD_DATA_1)
			| (1LL << PIN_NUM_SD_DATA_2)
#endif // PIN_NUM_SD_DATA_1
			| (1LL << PIN_NUM_SD_DATA_3)
			),
		.pull_down_en = 0,
		.pull_up_en   = 0,
	};
	esp_err_t res = gpio_config(&io_conf);
	if (res != ESP_OK)
		return res;
#endif // PIN_NUM_SD_CLK

	ESP_LOGD(TAG, "disabling power to sdcard and leds.");

	int ret;
#ifdef FXL6408_PIN_NUM_LEDS
	ret = badge_fxl6408_set_output_state(FXL6408_PIN_NUM_LEDS, 0);
#elif defined(MPR121_PIN_NUM_LEDS)
	ret = badge_mpr121_set_gpio_level(MPR121_PIN_NUM_LEDS, 0);
#else
	ret = ESP_OK;
#endif

	if (ret != ESP_OK)
	{
		ESP_LOGW(TAG, "failed to disable power.");
	}

	return ret;
}

esp_err_t
badge_power_leds_enable(void)
{
	if (badge_power_leds_sdcard & 1)
		return ESP_OK;

	if (badge_power_leds_sdcard)
	{
		badge_power_leds_sdcard |= 1;
		return ESP_OK;
	}

	esp_err_t ret = badge_power_sdcard_leds_enable();

	if (ret == ESP_OK)
		badge_power_leds_sdcard = 1;

	return ret;
}

esp_err_t
badge_power_leds_disable(void)
{
	if ((badge_power_leds_sdcard & 1) == 0)
		return ESP_OK;

	if (badge_power_leds_sdcard != 1)
	{
		badge_power_leds_sdcard &= ~1;
		return ESP_OK;
	}

	esp_err_t ret = badge_power_sdcard_leds_disable();

	if (ret == ESP_OK)
		badge_power_leds_sdcard = 0;

	return ret;
}

esp_err_t
badge_power_sdcard_enable(void)
{
	if (badge_power_leds_sdcard & 2)
		return ESP_OK;

	if (badge_power_leds_sdcard)
	{
		badge_power_leds_sdcard |= 2;
		return ESP_OK;
	}

	esp_err_t ret = badge_power_sdcard_leds_enable();

	if (ret == ESP_OK)
		badge_power_leds_sdcard = 2;

	return ret;
}

esp_err_t
badge_power_sdcard_disable(void)
{
	if ((badge_power_leds_sdcard & 2) == 0)
		return ESP_OK;

	if (badge_power_leds_sdcard != 2)
	{
		badge_power_leds_sdcard &= ~2;
		return ESP_OK;
	}

	esp_err_t ret = badge_power_sdcard_leds_disable();

	if (ret == ESP_OK)
		badge_power_leds_sdcard = 0;

	return ret;
}

esp_err_t
badge_power_init(void)
{
	static bool badge_power_init_done = false;

	if (badge_power_init_done)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	esp_err_t res;

	// configure adc width
#if defined(ADC1_CHAN_VBAT_SENSE) || defined(ADC1_CHAN_VUSB_SENSE)
	res = adc1_config_width(ADC_WIDTH_12Bit);
	assert( res == ESP_OK );
#endif // defined(ADC1_CHAN_VBAT_SENSE) || defined(ADC1_CHAN_VUSB_SENSE)

	// configure vbat-sense
#ifdef ADC1_CHAN_VBAT_SENSE
	// When VDD_A is 3.3V:
	// 6dB attenuation (ADC_ATTEN_6db) gives full-scale voltage 2.2V
	res = adc1_config_channel_atten(ADC1_CHAN_VBAT_SENSE, ADC_ATTEN_6db);
	assert( res == ESP_OK );
#endif // ADC1_CHAN_VBAT_SENSE

	// configure vusb-sense
#ifdef ADC1_CHAN_VUSB_SENSE
	// When VDD_A is 3.3V:
	// 6dB attenuation (ADC_ATTEN_6db) gives full-scale voltage 2.2V
	res = adc1_config_channel_atten(ADC1_CHAN_VUSB_SENSE, ADC_ATTEN_6db);
	assert( res == ESP_OK );
#endif // ADC1_CHAN_VUSB_SENSE

#if defined(FXL6408_PIN_NUM_CHRGSTAT) || defined(FXL6408_PIN_NUM_LEDS)
	res = badge_fxl6408_init();
	if (res != ESP_OK)
		return res;
#endif

#if defined(MPR121_PIN_NUM_CHRGSTAT) || defined(MPR121_PIN_NUM_LEDS)
	res = badge_mpr121_init();
	if (res != ESP_OK)
		return res;
#endif

	// configure charge-stat pin
#ifdef FXL6408_PIN_NUM_CHRGSTAT
	res = badge_fxl6408_set_io_direction(FXL6408_PIN_NUM_CHRGSTAT, 0);
	if (res != ESP_OK)
		return res;
	res = badge_fxl6408_set_input_default_state(FXL6408_PIN_NUM_CHRGSTAT, 0);
	if (res != ESP_OK)
		return res;
	res = badge_fxl6408_set_pull_enable(FXL6408_PIN_NUM_CHRGSTAT, 0);
	if (res != ESP_OK)
		return res;
	res = badge_fxl6408_set_interrupt_enable(FXL6408_PIN_NUM_CHRGSTAT, 0);
	if (res != ESP_OK)
		return res;
#elif defined(MPR121_PIN_NUM_CHRGSTAT)
	res = badge_mpr121_configure_gpio(MPR121_PIN_NUM_CHRGSTAT, MPR121_INPUT);
	if (res != ESP_OK)
		return res;
#endif

	// configure power to the leds and sd-card
#ifdef FXL6408_PIN_NUM_LEDS
	res = badge_fxl6408_set_output_state(FXL6408_PIN_NUM_LEDS, 0);
	if (res != ESP_OK)
		return res;
	res = badge_fxl6408_set_output_high_z(FXL6408_PIN_NUM_LEDS, 0);
	if (res != ESP_OK)
		return res;
	res = badge_fxl6408_set_io_direction(FXL6408_PIN_NUM_LEDS, 1);
	if (res != ESP_OK)
		return res;
#elif defined(MPR121_PIN_NUM_LEDS)
	res = badge_mpr121_configure_gpio(MPR121_PIN_NUM_LEDS, MPR121_OUTPUT);
	if (res != ESP_OK)
		return res;
#endif

	badge_power_init_done = true;

	ESP_LOGD(TAG, "init done");

	return ESP_OK;
}
