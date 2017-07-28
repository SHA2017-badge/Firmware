#include <sdkconfig.h>

#ifdef CONFIG_SHA_BADGE_FXL6408_DEBUG
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif // CONFIG_SHA_BADGE_FXL6408_DEBUG

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <rom/ets_sys.h>
#include <esp_log.h>
#include <driver/gpio.h>

#include "badge_pins.h"
#include "badge_base.h"
#include "badge_i2c.h"
#include "badge_fxl6408.h"

#ifdef I2C_FXL6408_ADDR

static const char *TAG = "badge_fxl6408";

// use bit 8 to mark unknown current state due to failed write.
struct badge_fxl6408_state_t {
	uint16_t io_direction;         // default is 0x00
	uint16_t output_state;         // default is 0x00
	uint16_t output_high_z;        // default is 0xff
	uint16_t input_default_state;  // default is 0x00
	uint16_t pull_enable;          // default is 0xff
	uint16_t pull_down_up;         // default is 0x00
	uint16_t interrupt_mask;       // default is 0x00
};

// mutex for accessing badge_fxl6408_state, badge_fxl6408_handlers, etc..
static xSemaphoreHandle badge_fxl6408_mux = NULL;

// semaphore to trigger port-expander interrupt handling
static xSemaphoreHandle badge_fxl6408_intr_trigger = NULL;

// port-expander state
static struct badge_fxl6408_state_t badge_fxl6408_state;

// handlers per port-expander port.
static badge_fxl6408_intr_t badge_fxl6408_handlers[8] = { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
static void * badge_fxl6408_arg[8] = { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

static inline int
badge_fxl6408_read_reg(uint8_t reg)
{
	uint8_t value;
	esp_err_t res = badge_i2c_read_reg(I2C_FXL6408_ADDR, reg, &value, 1);

	if (res != ESP_OK) {
		ESP_LOGE(TAG, "i2c read reg(0x%02x): error %d", reg, res);
		return -1;
	}

	ESP_LOGD(TAG, "i2c read reg(0x%02x): 0x%02x", reg, value);

	return value;
}

static inline esp_err_t
badge_fxl6408_write_reg(uint8_t reg, uint8_t value)
{
	esp_err_t res = badge_i2c_write_reg(I2C_FXL6408_ADDR, reg, value);

	if (res != ESP_OK) {
		ESP_LOGE(TAG, "i2c write reg(0x%02x, 0x%02x): error %d", reg, value, res);
		return res;
	}

	ESP_LOGD(TAG, "i2c write reg(0x%02x, 0x%02x): ok", reg, value);

	return ESP_OK;
}

void
badge_fxl6408_intr_task(void *arg)
{
	// we cannot use I2C in the interrupt handler, so we
	// create an extra thread for this..

	while (1)
	{
		if (xSemaphoreTake(badge_fxl6408_intr_trigger, portMAX_DELAY))
		{
			int ints = badge_fxl6408_get_interrupt_status();
			// NOTE: if ints = -1, then all handlers will trigger.

			int i;
			for (i=0; i<8; i++)
			{
				if (ints & (1 << i))
				{
					xSemaphoreTake(badge_fxl6408_mux, portMAX_DELAY);
					badge_fxl6408_intr_t handler = badge_fxl6408_handlers[i];
					void *arg = badge_fxl6408_arg[i];
					xSemaphoreGive(badge_fxl6408_mux);

					if (handler != NULL)
						handler(arg);
				}
			}
		}
	}
}

void
badge_fxl6408_intr_handler(void *arg)
{ /* in interrupt handler */
	int gpio_state = gpio_get_level(PIN_NUM_FXL6408_INT);

#ifdef CONFIG_SHA_BADGE_FXL6408_DEBUG
	static int gpio_last_state = -1;
	if (gpio_state != -1 && gpio_last_state != gpio_state)
	{
		ets_printf("badge_fxl6408: I2C Int %s\n", gpio_state == 0 ? "up" : "down");
	}
	gpio_last_state = gpio_state;
#endif // CONFIG_SHA_BADGE_FXL6408_DEBUG

	if (gpio_state == 0)
	{
		xSemaphoreGiveFromISR(badge_fxl6408_intr_trigger, NULL);
	}
}

esp_err_t
badge_fxl6408_init(void)
{
	static bool badge_fxl6408_init_done = false;

	if (badge_fxl6408_init_done)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	esp_err_t res = badge_base_init();
	if (res != ESP_OK)
		return res;

	res = badge_i2c_init();
	if (res != ESP_OK)
		return res;

	badge_fxl6408_mux = xSemaphoreCreateMutex();
	if (badge_fxl6408_mux == NULL)
		return ESP_ERR_NO_MEM;

	badge_fxl6408_intr_trigger = xSemaphoreCreateBinary();
	if (badge_fxl6408_intr_trigger == NULL)
		return ESP_ERR_NO_MEM;

	gpio_isr_handler_add(PIN_NUM_FXL6408_INT, badge_fxl6408_intr_handler, NULL);
	gpio_config_t io_conf = {
		.intr_type    = GPIO_INTR_ANYEDGE,
		.mode         = GPIO_MODE_INPUT,
		.pin_bit_mask = 1LL << PIN_NUM_FXL6408_INT,
		.pull_down_en = 0,
		.pull_up_en   = 1,
	};
	res = gpio_config(&io_conf);
	if (res != ESP_OK)
		return res;

//	badge_fxl6408_write_reg(0x01, 0x01); // sw reset
	badge_fxl6408_write_reg(0x03, 0x00);
	badge_fxl6408_write_reg(0x05, 0x00);
	badge_fxl6408_write_reg(0x07, 0xff);
	badge_fxl6408_write_reg(0x09, 0x00);
	badge_fxl6408_write_reg(0x0b, 0xff);
	badge_fxl6408_write_reg(0x0d, 0x00);
	badge_fxl6408_write_reg(0x11, 0xff);
	badge_fxl6408_write_reg(0x13, 0x00);
	struct badge_fxl6408_state_t init_state = {
		.io_direction        = 0x00,
		.output_state        = 0x00,
		.output_high_z       = 0xff,
		.input_default_state = 0x00,
		.pull_enable         = 0xff,
		.pull_down_up        = 0x00,
		.interrupt_mask      = 0xff,
	};
	memcpy(&badge_fxl6408_state, &init_state, sizeof(init_state));

	xTaskCreate(&badge_fxl6408_intr_task, "port-expander interrupt task", 4096, NULL, 10, NULL);

	// it seems that we need to read some registers to start interrupt handling.. (?)
	badge_fxl6408_read_reg(0x01);
	badge_fxl6408_read_reg(0x03);
	badge_fxl6408_read_reg(0x05);
	badge_fxl6408_read_reg(0x07);
	badge_fxl6408_read_reg(0x09);
	badge_fxl6408_read_reg(0x0b);
	badge_fxl6408_read_reg(0x0d);
	badge_fxl6408_read_reg(0x0f);
	badge_fxl6408_read_reg(0x11);
	badge_fxl6408_read_reg(0x13);

	badge_fxl6408_intr_handler(NULL);

	badge_fxl6408_init_done = true;

	ESP_LOGD(TAG, "init done");

	return ESP_OK;
}

esp_err_t
badge_fxl6408_set_io_direction(uint8_t pin, uint8_t direction)
{
	xSemaphoreTake(badge_fxl6408_mux, portMAX_DELAY);

	uint8_t value = badge_fxl6408_state.io_direction;
	if (direction)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	esp_err_t res = ESP_OK;
	if (badge_fxl6408_state.io_direction != value)
	{
		badge_fxl6408_state.io_direction = value;
		res = badge_fxl6408_write_reg(0x03, value);
		if (res != ESP_OK)
			badge_fxl6408_state.io_direction |= 0x100;
	}

	xSemaphoreGive(badge_fxl6408_mux);

	return res;
}

esp_err_t
badge_fxl6408_set_output_state(uint8_t pin, uint8_t state)
{
	xSemaphoreTake(badge_fxl6408_mux, portMAX_DELAY);

	uint8_t value = badge_fxl6408_state.output_state;
	if (state)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	esp_err_t res = ESP_OK;
	if (badge_fxl6408_state.output_state != value)
	{
		badge_fxl6408_state.output_state = value;
		res = badge_fxl6408_write_reg(0x05, value);
		if (res != ESP_OK)
			badge_fxl6408_state.output_state |= 0x100;
	}

	xSemaphoreGive(badge_fxl6408_mux);

	return res;
}

esp_err_t
badge_fxl6408_set_output_high_z(uint8_t pin, uint8_t high_z)
{
	xSemaphoreTake(badge_fxl6408_mux, portMAX_DELAY);

	uint8_t value = badge_fxl6408_state.output_high_z;
	if (high_z)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	esp_err_t res = ESP_OK;
	if (badge_fxl6408_state.output_high_z != value)
	{
		badge_fxl6408_state.output_high_z = value;
		res = badge_fxl6408_write_reg(0x07, value);
		if (res != ESP_OK)
			badge_fxl6408_state.output_high_z |= 0x100;
	}

	xSemaphoreGive(badge_fxl6408_mux);

	return res;
}

esp_err_t
badge_fxl6408_set_input_default_state(uint8_t pin, uint8_t state)
{
	xSemaphoreTake(badge_fxl6408_mux, portMAX_DELAY);

	uint8_t value = badge_fxl6408_state.input_default_state;
	if (state)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	esp_err_t res = ESP_OK;
	if (badge_fxl6408_state.input_default_state != value)
	{
		badge_fxl6408_state.input_default_state = value;
		res = badge_fxl6408_write_reg(0x09, value);
		if (res != ESP_OK)
			badge_fxl6408_state.input_default_state |= 0x100;
	}

	xSemaphoreGive(badge_fxl6408_mux);

	return res;
}

esp_err_t
badge_fxl6408_set_pull_enable(uint8_t pin, uint8_t enable)
{
	xSemaphoreTake(badge_fxl6408_mux, portMAX_DELAY);

	uint8_t value = badge_fxl6408_state.pull_enable;
	if (enable)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	esp_err_t res = ESP_OK;
	if (badge_fxl6408_state.pull_enable != value)
	{
		badge_fxl6408_state.pull_enable = value;
		res = badge_fxl6408_write_reg(0x0b, value);
		if (res != ESP_OK)
			badge_fxl6408_state.pull_enable |= 0x100;
	}

	xSemaphoreGive(badge_fxl6408_mux);

	return res;
}

esp_err_t
badge_fxl6408_set_pull_down_up(uint8_t pin, uint8_t up)
{
	xSemaphoreTake(badge_fxl6408_mux, portMAX_DELAY);

	uint8_t value = badge_fxl6408_state.pull_down_up;
	if (up)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	esp_err_t res = ESP_OK;
	if (badge_fxl6408_state.pull_down_up != value)
	{
		badge_fxl6408_state.pull_down_up = value;
		res = badge_fxl6408_write_reg(0x0d, value);
		if (res != ESP_OK)
			badge_fxl6408_state.pull_down_up |= 0x100;
	}

	xSemaphoreGive(badge_fxl6408_mux);

	return res;
}

esp_err_t
badge_fxl6408_set_interrupt_enable(uint8_t pin, uint8_t enable)
{
	xSemaphoreTake(badge_fxl6408_mux, portMAX_DELAY);

	uint8_t value = badge_fxl6408_state.interrupt_mask;
	if (enable)
		value &= ~(1 << pin);
	else
		value |= 1 << pin;

	esp_err_t res = ESP_OK;
	if (badge_fxl6408_state.interrupt_mask != value)
	{
		badge_fxl6408_state.interrupt_mask = value;
		res = badge_fxl6408_write_reg(0x11, value);
		if (res != ESP_OK)
			badge_fxl6408_state.interrupt_mask |= 0x100;
	}

	xSemaphoreGive(badge_fxl6408_mux);

	return res;
}

void
badge_fxl6408_set_interrupt_handler(uint8_t pin, badge_fxl6408_intr_t handler, void *arg)
{
	xSemaphoreTake(badge_fxl6408_mux, portMAX_DELAY);

	badge_fxl6408_handlers[pin] = handler;
	badge_fxl6408_arg[pin] = arg;

	xSemaphoreGive(badge_fxl6408_mux);
}

int
badge_fxl6408_get_input(void)
{
	return badge_fxl6408_read_reg(0x0f);
}

int
badge_fxl6408_get_interrupt_status(void)
{
	return badge_fxl6408_read_reg(0x13);
}

#endif // I2C_FXL6408_ADDR
