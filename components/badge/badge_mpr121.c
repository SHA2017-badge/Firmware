#include <sdkconfig.h>

#ifdef CONFIG_SHA_BADGE_MPR121_DEBUG
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif // CONFIG_SHA_BADGE_MPR121_DEBUG

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/gpio.h>

#include "badge_pins.h"
#include "badge_base.h"
#include "badge_i2c.h"
#include "badge_mpr121.h"

#ifdef I2C_MPR121_ADDR

#define MPR121_BASELINE_0   0x1E
#define MPR121_MHDR         0x2B
#define MPR121_NHDR         0x2C
#define MPR121_NCLR         0x2D
#define MPR121_FDLR         0x2E
#define MPR121_MHDF         0x2F
#define MPR121_NHDF         0x30
#define MPR121_NCLF         0x31
#define MPR121_FDLF         0x32
#define MPR121_NHDT         0x33
#define MPR121_NCLT         0x34
#define MPR121_FDLT         0x35

#define MPR121_TOUCHTH_0    0x41
#define MPR121_RELEASETH_0  0x42
#define MPR121_DEBOUNCE     0x5B
#define MPR121_CONFIG1      0x5C
#define MPR121_CONFIG2      0x5D

static const char *TAG = "badge_mpr121";

// mutex for accessing badge_mpr121_state, badge_mpr121_handlers, etc..
xSemaphoreHandle badge_mpr121_mux = NULL;

// semaphore to trigger MPR121 interrupt handling
xSemaphoreHandle badge_mpr121_intr_trigger = NULL;

// handlers per MPR121 port.
badge_mpr121_intr_t badge_mpr121_handlers[12] = { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
void* badge_mpr121_arg[12] = { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

static inline int
badge_mpr121_read_reg(uint8_t reg)
{
	uint8_t value;
	esp_err_t res = badge_i2c_read_reg(I2C_MPR121_ADDR, reg, &value, 1);

	if (res != ESP_OK) {
		ESP_LOGE(TAG, "i2c read reg(0x%02x): error %d", reg, res);
		return -1;
	}

	ESP_LOGD(TAG, "i2c read reg(0x%02x): 0x%02x", reg, value);

	return value;
}

static inline esp_err_t
badge_mpr121_read_regs(uint8_t reg, uint8_t *data, size_t data_len)
{
	esp_err_t res = badge_i2c_read_reg(I2C_MPR121_ADDR, reg, data, data_len);

	if (res != ESP_OK) {
		ESP_LOGE(TAG, "i2c read regs(0x%02x, %d): error %d", reg, data_len, res);
		return res;
	}

#ifdef CONFIG_SHA_BADGE_MPR121_DEBUG
	{
		ESP_LOGD(TAG, "i2c read regs(0x%02x, %d):", reg, data_len);
		char line[16*3 + 1];
		char *lptr = line;
		int i;
		for (i=0; i<data_len; i++)
		{
			sprintf(lptr, " %02x", data[i]);
			lptr = &lptr[3];
			if ((i % 16) == 15 || i == data_len-1)
			{
				ESP_LOGD(TAG, "%s", line);
				lptr = line;
			}
		}
	}
#endif // CONFIG_SHA_BADGE_MPR121_DEBUG

	return res;
}

static inline esp_err_t
badge_mpr121_write_reg(uint8_t reg, uint8_t value)
{
	esp_err_t res = badge_i2c_write_reg(I2C_MPR121_ADDR, reg, value);

	if (res != ESP_OK) {
		ESP_LOGE(TAG, "i2c write reg(0x%02x, 0x%02x): error %d", reg, value, res);
		return res;
	}

	ESP_LOGD(TAG, "i2c write reg(0x%02x, 0x%02x): ok", reg, value);

	return res;
}

void
badge_mpr121_intr_task(void *arg)
{
	// we cannot use I2C in the interrupt handler, so we
	// create an extra thread for this..

	int old_state = 0;
	while (1)
	{
		if (xSemaphoreTake(badge_mpr121_intr_trigger, portMAX_DELAY))
		{
			int state;
			while (1)
			{
				state = badge_mpr121_get_interrupt_status();
				if (state != -1)
					break;

				ESP_LOGE(TAG, "failed to read status registers.");
				vTaskDelay(1000 / portTICK_PERIOD_MS);
			}

			int i;
			for (i=0; i<8; i++)
			{
				if ((state & (1 << i)) != (old_state & (1 << i)))
				{
					xSemaphoreTake(badge_mpr121_mux, portMAX_DELAY);
					badge_mpr121_intr_t handler = badge_mpr121_handlers[i];
					void *arg = badge_mpr121_arg[i];
					xSemaphoreGive(badge_mpr121_mux);

					if (handler != NULL)
						handler(arg, (state & (1 << i)) != 0);
				}
			}

			if (state & 0x8000)
			{
				ESP_LOGE(TAG, "over-current detected!");
				vTaskDelay(1000 / portTICK_PERIOD_MS);

				// clear OVCF by writing a 1
				esp_err_t res = badge_mpr121_write_reg(0x01, state >> 8);
				if (res != ESP_OK)
				{
					ESP_LOGE(TAG, "failed to reset over-current.");
				}

				// enable run-mode, set base-line tracking
				res = badge_mpr121_write_reg(0x5e, 0x88);
				if (res != ESP_OK)
				{
					ESP_LOGE(TAG, "failed to re-enable touch.");
				}
			}

			old_state = state;
		}
	}
}

void
badge_mpr121_intr_handler(void *arg)
{ /* in interrupt handler */
	int gpio_state = gpio_get_level(PIN_NUM_MPR121_INT);

#ifdef CONFIG_SHA_BADGE_MPR121_DEBUG
	static int gpio_last_state = -1;
	if (gpio_state != -1 && gpio_last_state != gpio_state)
	{
		ets_printf("badge_mpr121: I2C Int %s\n", gpio_state == 0 ? "up" : "down");
	}
	gpio_last_state = gpio_state;
#endif // CONFIG_SHA_BADGE_MPR121_DEBUG

	if (gpio_state == 0)
	{
		xSemaphoreGiveFromISR(badge_mpr121_intr_trigger, NULL);
	}
}

esp_err_t
badge_mpr121_configure(const uint32_t *baseline, bool strict)
{
	static const uint8_t conf[2*15] = {
		// soft reset
		0x80, 0x63,

		// set baseline filters
		MPR121_MHDR, 0x01,
		MPR121_NHDR, 0x01,
		MPR121_NCLR, 0x0E,
		MPR121_FDLR, 0x00,

		MPR121_MHDF, 0x01,
		MPR121_NHDF, 0x05,
		MPR121_NCLF, 0x01,
		MPR121_FDLF, 0x00,

		MPR121_NHDT, 0x00,
		MPR121_NCLT, 0x00,
		MPR121_FDLT, 0x00,

		MPR121_DEBOUNCE, 0x00,
		MPR121_CONFIG1, 0x10,  // default, 16µA charge current
		MPR121_CONFIG2, 0x20,  // 0x5µs encoding, 1ms period
	};
	esp_err_t res;

	ESP_LOGD(TAG, "configure called");

	int i;
	for (i=0; i<sizeof(conf); i += 2)
	{
		res = badge_mpr121_write_reg(conf[i], conf[i+1]);
		if (res != ESP_OK)
			return res;
	}

	// set thresholds
	for (i=0; i<8; i++)
	{
		if (baseline != NULL)
		{
			res = badge_mpr121_write_reg(MPR121_BASELINE_0 + i, baseline[i] >> 2); // baseline
			if (res != ESP_OK)
				return res;
		}

		if (strict)
		{
			res = badge_mpr121_write_reg(MPR121_TOUCHTH_0   + 2*i, 24); // touch
			if (res != ESP_OK)
				return res;

			res = badge_mpr121_write_reg(MPR121_RELEASETH_0 + 2*i, 12); // release
			if (res != ESP_OK)
				return res;
		}
		else
		{
			res = badge_mpr121_write_reg(MPR121_TOUCHTH_0   + 2*i, 48); // touch
			if (res != ESP_OK)
				return res;

			res = badge_mpr121_write_reg(MPR121_RELEASETH_0 + 2*i, 24); // release
			if (res != ESP_OK)
				return res;
		}
	}

	if (baseline == NULL)
	{
		// enable run-mode, set base-line tracking
		res = badge_mpr121_write_reg(0x5e, 0x88);
		if (res != ESP_OK)
			return res;
	}
	else
	{
		// enable run-mode, disable base-line tracking
		res = badge_mpr121_write_reg(0x5e, 0x48);
		if (res != ESP_OK)
			return res;
	}

	ESP_LOGD(TAG, "configure done");

	return ESP_OK;
}

esp_err_t
badge_mpr121_init(void)
{
	static bool badge_mpr121_init_done = false;

	if (badge_mpr121_init_done)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	esp_err_t res = badge_base_init();
	if (res != ESP_OK)
		return res;

	res = badge_i2c_init();
	if (res != ESP_OK)
		return res;

	badge_mpr121_mux = xSemaphoreCreateMutex();
	if (badge_mpr121_mux == NULL)
		return ESP_ERR_NO_MEM;

	badge_mpr121_intr_trigger = xSemaphoreCreateBinary();
	if (badge_mpr121_intr_trigger == NULL)
		return ESP_ERR_NO_MEM;

	res = gpio_isr_handler_add(PIN_NUM_MPR121_INT, badge_mpr121_intr_handler, NULL);
	if (res != ESP_OK)
		return res;

	gpio_config_t io_conf = {
		.intr_type    = GPIO_INTR_ANYEDGE,
		.mode         = GPIO_MODE_INPUT,
		.pin_bit_mask = 1LL << PIN_NUM_MPR121_INT,
		.pull_down_en = 0,
		.pull_up_en   = 1,
	};
	res = gpio_config(&io_conf);
	if (res != ESP_OK)
		return res;

	xTaskCreate(&badge_mpr121_intr_task, "MPR121 interrupt task", 4096, NULL, 10, NULL);

	xSemaphoreGive(badge_mpr121_intr_trigger);

	badge_mpr121_init_done = true;

	ESP_LOGD(TAG, "init done");

	return ESP_OK;
}

void
badge_mpr121_set_interrupt_handler(uint8_t pin, badge_mpr121_intr_t handler, void *arg)
{
	if (badge_mpr121_mux == NULL)
	{ // allow setting handlers when badge_mpr121 is not initialized yet.
		badge_mpr121_handlers[pin] = handler;
		badge_mpr121_arg[pin] = arg;
	}
	else
	{
		xSemaphoreTake(badge_mpr121_mux, portMAX_DELAY);

		badge_mpr121_handlers[pin] = handler;
		badge_mpr121_arg[pin] = arg;

		xSemaphoreGive(badge_mpr121_mux);
	}
}

int
badge_mpr121_get_interrupt_status(void)
{
	uint16_t value;
	esp_err_t res = badge_mpr121_read_regs(0x00, (uint8_t *) &value, 2);
	if (res != ESP_OK)
		return -1;

	return value;
}

esp_err_t
badge_mpr121_get_touch_info(struct badge_mpr121_touch_info *info)
{
	int value = badge_mpr121_read_reg(0x00);
	if (value == -1)
		return ESP_FAIL; // need more-specific error?
	info->touch_state = value;

	uint16_t data[8];
	esp_err_t res = badge_mpr121_read_regs(0x04, (uint8_t *) &data, 16);
	if (res != ESP_OK)
		return res;

	uint8_t baseline[8];
	res = badge_mpr121_read_regs(0x1e, baseline, 8);
	if (res != ESP_OK)
		return res;

	uint8_t touch_release[16];
	res = badge_mpr121_read_regs(0x41, touch_release, 16);
	if (res != ESP_OK)
		return res;

	int i;
	for (i=0; i<8; i++)
	{
		info->data[i] = data[i];
		info->baseline[i] = baseline[i];
		info->touch[i] = touch_release[i*2+0];
		info->release[i] = touch_release[i*2+1];
	}

	return ESP_OK;
}

int mpr121_gpio_bit_out[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };

int
badge_mpr121_configure_gpio(int pin, enum badge_mpr121_gpio_config config)
{
	if (pin < 4 || pin >= 12)
		return -1;

	pin -= 4;
	int bit_set = 1 << pin;
	int bit_rst = bit_set ^ 0xff;

	mpr121_gpio_bit_out[pin] = -1;

	// set control 0: 0
	int value = badge_mpr121_read_reg(0x73);
	if (value == -1)
		return -1;

	if ((config & 1) == 0)
		value &= bit_rst;
	else
		value |= bit_set;

	esp_err_t res = badge_mpr121_write_reg(0x73, value);
	if (res != ESP_OK)
		return -1;

	// set control 1: 0
	value = badge_mpr121_read_reg(0x74);
	if (value == -1)
		return -1;

	if ((config & 2) == 0)
		value &= bit_rst;
	else
		value |= bit_set;

	res = badge_mpr121_write_reg(0x74, value);
	if (res != ESP_OK)
		return -1;

	// set data: 0 = low
	value = badge_mpr121_read_reg(0x75);
	if (value == -1)
		return -1;

	// always reset data out bit
	value &= bit_rst;

	res = badge_mpr121_write_reg(0x75, value);
	if (res != ESP_OK)
		return -1;

	// set direction: 1 = output
	value = badge_mpr121_read_reg(0x76);
	if (value == -1)
		return -1;

	if ((config & 4) == 0)
		value &= bit_rst;
	else
		value |= bit_set;

	res = badge_mpr121_write_reg(0x76, value);
	if (res != ESP_OK)
		return -1;

	// enable gpio pin: 1 = enable
	value = badge_mpr121_read_reg(0x77);
	if (value == -1)
		return -1;

	if ((config & 8) == 0)
		value &= bit_rst;
	else
		value |= bit_set;

	res = badge_mpr121_write_reg(0x77, value);
	if (res != ESP_OK)
		return -1;

	return 0;
}

int
badge_mpr121_get_gpio_level(int pin)
{
	if (pin < 4 || pin >= 12)
		return -1;

	pin &= 7;

	// read data from status register
	int value = badge_mpr121_read_reg(pin < 4 ? 0x01: 0x00);
	if (value == -1)
		return -1;

	return (value >> pin) & 1;
}

int
badge_mpr121_set_gpio_level(int pin, int value)
{
	if (pin < 4 || pin >= 12)
		return ESP_ERR_INVALID_ARG;

	pin -= 4;
	if (mpr121_gpio_bit_out[pin] == value)
		return ESP_OK;

	mpr121_gpio_bit_out[pin] = -1;
	int bit_set = 1 << pin;
	if (value == 0)
	{
		int res = badge_mpr121_write_reg(0x79, bit_set); // clear bit
		if (res == ESP_OK)
			mpr121_gpio_bit_out[pin] = 0;
		return res;
	}
	else
	{
		int res = badge_mpr121_write_reg(0x78, bit_set); // set bit
		if (res == ESP_OK)
			mpr121_gpio_bit_out[pin] = 1;
		return res;
	}
}

#endif // I2C_MPR121_ADDR
