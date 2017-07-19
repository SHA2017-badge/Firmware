#include <sdkconfig.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <rom/ets_sys.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/gpio.h>

#include "badge_pins.h"
#include "badge_base.h"
#include "badge_i2c.h"
#include "badge_portexp.h"

#ifdef I2C_PORTEXP_ADDR

static const char *TAG = "badge_portexp";

// use bit 8 to mark unknown current state due to failed write.
struct badge_portexp_state_t {
	uint16_t io_direction;         // default is 0x00
	uint16_t output_state;         // default is 0x00
	uint16_t output_high_z;        // default is 0xff
	uint16_t input_default_state;  // default is 0x00
	uint16_t pull_enable;          // default is 0xff
	uint16_t pull_down_up;         // default is 0x00
	uint16_t interrupt_mask;       // default is 0x00
};

// mutex for accessing badge_portexp_state, badge_portexp_handlers, etc..
static xSemaphoreHandle badge_portexp_mux = NULL;

// semaphore to trigger port-expander interrupt handling
static xSemaphoreHandle badge_portexp_intr_trigger = NULL;

// port-expander state
static struct badge_portexp_state_t badge_portexp_state;

// handlers per port-expander port.
static badge_portexp_intr_t badge_portexp_handlers[8] = { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
static void * badge_portexp_arg[8] = { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

static inline int
badge_portexp_read_reg(uint8_t reg)
{
	uint8_t value;
	esp_err_t res = badge_i2c_read_reg(I2C_PORTEXP_ADDR, reg, &value, 1);

	if (res != ESP_OK) {
		ets_printf("badge_portexp: i2c read reg(0x%02x): error %d\n", reg, res);
		return -1;
	}

#ifdef CONFIG_SHA_BADGE_PORTEXP_DEBUG
	ets_printf("badge_portexp: i2c read reg(0x%02x): 0x%02x\n", reg, value);
#endif // CONFIG_SHA_BADGE_PORTEXP_DEBUG

	return value;
}

static inline esp_err_t
badge_portexp_write_reg(uint8_t reg, uint8_t value)
{
	esp_err_t res = badge_i2c_write_reg(I2C_PORTEXP_ADDR, reg, value);

	if (res != ESP_OK) {
		ets_printf("badge_portexp: i2c write reg(0x%02x, 0x%02x): error %d\n", reg, value, res);
		return res;
	}

#ifdef CONFIG_SHA_BADGE_PORTEXP_DEBUG
	ets_printf("badge_portexp: i2c write reg(0x%02x, 0x%02x): ok\n", reg, value);
#endif // CONFIG_SHA_BADGE_PORTEXP_DEBUG

	return ESP_OK;
}

void
badge_portexp_intr_task(void *arg)
{
	// we cannot use I2C in the interrupt handler, so we
	// create an extra thread for this..

	while (1)
	{
		if (xSemaphoreTake(badge_portexp_intr_trigger, portMAX_DELAY))
		{
			int ints = badge_portexp_get_interrupt_status();
			// NOTE: if ints = -1, then all handlers will trigger.

			int i;
			for (i=0; i<8; i++)
			{
				if (ints & (1 << i))
				{
					xSemaphoreTake(badge_portexp_mux, portMAX_DELAY);
					badge_portexp_intr_t handler = badge_portexp_handlers[i];
					void *arg = badge_portexp_arg[i];
					xSemaphoreGive(badge_portexp_mux);

					if (handler != NULL)
						handler(arg);
				}
			}
		}
	}
}

void
badge_portexp_intr_handler(void *arg)
{

	int gpio_state = gpio_get_level(PIN_NUM_PORTEXP_INT);
#ifdef CONFIG_SHA_BADGE_PORTEXP_DEBUG
	static int gpio_last_state = -1;
	if (gpio_last_state != gpio_state)
	{
		if (gpio_state == 1)
			ets_printf("badge_portexp: I2C Int down\n");
		else if (gpio_state == 0)
			ets_printf("badge_portexp: I2C Int up\n");
	}
	gpio_last_state = gpio_state;
#endif // CONFIG_SHA_BADGE_PORTEXP_DEBUG

	if (gpio_state == 0)
	{
		xSemaphoreGiveFromISR(badge_portexp_intr_trigger, NULL);
	}
}

esp_err_t
badge_portexp_init(void)
{
	static bool badge_portexp_init_done = false;

	if (badge_portexp_init_done)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	esp_err_t res = badge_base_init();
	if (res != ESP_OK)
		return res;

	res = badge_i2c_init();
	if (res != ESP_OK)
		return res;

	badge_portexp_mux = xSemaphoreCreateMutex();
	if (badge_portexp_mux == NULL)
		return ESP_ERR_NO_MEM;

	badge_portexp_intr_trigger = xSemaphoreCreateBinary();
	if (badge_portexp_intr_trigger == NULL)
		return ESP_ERR_NO_MEM;

	gpio_isr_handler_add(PIN_NUM_PORTEXP_INT, badge_portexp_intr_handler, NULL);
	gpio_config_t io_conf = {
		.intr_type    = GPIO_INTR_ANYEDGE,
		.mode         = GPIO_MODE_INPUT,
		.pin_bit_mask = 1LL << PIN_NUM_PORTEXP_INT,
		.pull_down_en = 0,
		.pull_up_en   = 1,
	};
	res = gpio_config(&io_conf);
	if (res != ESP_OK)
		return res;

//	badge_portexp_write_reg(0x01, 0x01); // sw reset
	badge_portexp_write_reg(0x03, 0x00);
	badge_portexp_write_reg(0x05, 0x00);
	badge_portexp_write_reg(0x07, 0xff);
	badge_portexp_write_reg(0x09, 0x00);
	badge_portexp_write_reg(0x0b, 0xff);
	badge_portexp_write_reg(0x0d, 0x00);
	badge_portexp_write_reg(0x11, 0xff);
	badge_portexp_write_reg(0x13, 0x00);
	struct badge_portexp_state_t init_state = {
		.io_direction        = 0x00,
		.output_state        = 0x00,
		.output_high_z       = 0xff,
		.input_default_state = 0x00,
		.pull_enable         = 0xff,
		.pull_down_up        = 0x00,
		.interrupt_mask      = 0xff,
	};
	memcpy(&badge_portexp_state, &init_state, sizeof(init_state));

	xTaskCreatePinnedToCore(&badge_portexp_intr_task, "port-expander interrupt task", 4096, NULL, 10, NULL, 0);

	// it seems that we need to read some registers to start interrupt handling.. (?)
	badge_portexp_read_reg(0x01);
	badge_portexp_read_reg(0x03);
	badge_portexp_read_reg(0x05);
	badge_portexp_read_reg(0x07);
	badge_portexp_read_reg(0x09);
	badge_portexp_read_reg(0x0b);
	badge_portexp_read_reg(0x0d);
	badge_portexp_read_reg(0x0f);
	badge_portexp_read_reg(0x11);
	badge_portexp_read_reg(0x13);

	badge_portexp_intr_handler(NULL);

	badge_portexp_init_done = true;

	ESP_LOGD(TAG, "init done");

	return ESP_OK;
}

esp_err_t
badge_portexp_set_io_direction(uint8_t pin, uint8_t direction)
{
	xSemaphoreTake(badge_portexp_mux, portMAX_DELAY);

	uint8_t value = badge_portexp_state.io_direction;
	if (direction)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	esp_err_t res = ESP_OK;
	if (badge_portexp_state.io_direction != value)
	{
		badge_portexp_state.io_direction = value;
		res = badge_portexp_write_reg(0x03, value);
		if (res != ESP_OK)
			badge_portexp_state.io_direction |= 0x100;
	}

	xSemaphoreGive(badge_portexp_mux);

	return res;
}

esp_err_t
badge_portexp_set_output_state(uint8_t pin, uint8_t state)
{
	xSemaphoreTake(badge_portexp_mux, portMAX_DELAY);

	uint8_t value = badge_portexp_state.output_state;
	if (state)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	esp_err_t res = ESP_OK;
	if (badge_portexp_state.output_state != value)
	{
		badge_portexp_state.output_state = value;
		res = badge_portexp_write_reg(0x05, value);
		if (res != ESP_OK)
			badge_portexp_state.output_state |= 0x100;
	}

	xSemaphoreGive(badge_portexp_mux);

	return res;
}

esp_err_t
badge_portexp_set_output_high_z(uint8_t pin, uint8_t high_z)
{
	xSemaphoreTake(badge_portexp_mux, portMAX_DELAY);

	uint8_t value = badge_portexp_state.output_high_z;
	if (high_z)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	esp_err_t res = ESP_OK;
	if (badge_portexp_state.output_high_z != value)
	{
		badge_portexp_state.output_high_z = value;
		res = badge_portexp_write_reg(0x07, value);
		if (res != ESP_OK)
			badge_portexp_state.output_high_z |= 0x100;
	}

	xSemaphoreGive(badge_portexp_mux);

	return res;
}

esp_err_t
badge_portexp_set_input_default_state(uint8_t pin, uint8_t state)
{
	xSemaphoreTake(badge_portexp_mux, portMAX_DELAY);

	uint8_t value = badge_portexp_state.input_default_state;
	if (state)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	esp_err_t res = ESP_OK;
	if (badge_portexp_state.input_default_state != value)
	{
		badge_portexp_state.input_default_state = value;
		res = badge_portexp_write_reg(0x09, value);
		if (res != ESP_OK)
			badge_portexp_state.input_default_state |= 0x100;
	}

	xSemaphoreGive(badge_portexp_mux);

	return res;
}

esp_err_t
badge_portexp_set_pull_enable(uint8_t pin, uint8_t enable)
{
	xSemaphoreTake(badge_portexp_mux, portMAX_DELAY);

	uint8_t value = badge_portexp_state.pull_enable;
	if (enable)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	esp_err_t res = ESP_OK;
	if (badge_portexp_state.pull_enable != value)
	{
		badge_portexp_state.pull_enable = value;
		res = badge_portexp_write_reg(0x0b, value);
		if (res != ESP_OK)
			badge_portexp_state.pull_enable |= 0x100;
	}

	xSemaphoreGive(badge_portexp_mux);

	return res;
}

esp_err_t
badge_portexp_set_pull_down_up(uint8_t pin, uint8_t up)
{
	xSemaphoreTake(badge_portexp_mux, portMAX_DELAY);

	uint8_t value = badge_portexp_state.pull_down_up;
	if (up)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	esp_err_t res = ESP_OK;
	if (badge_portexp_state.pull_down_up != value)
	{
		badge_portexp_state.pull_down_up = value;
		res = badge_portexp_write_reg(0x0d, value);
		if (res != ESP_OK)
			badge_portexp_state.pull_down_up |= 0x100;
	}

	xSemaphoreGive(badge_portexp_mux);

	return res;
}

esp_err_t
badge_portexp_set_interrupt_enable(uint8_t pin, uint8_t enable)
{
	xSemaphoreTake(badge_portexp_mux, portMAX_DELAY);

	uint8_t value = badge_portexp_state.interrupt_mask;
	if (enable)
		value &= ~(1 << pin);
	else
		value |= 1 << pin;

	esp_err_t res = ESP_OK;
	if (badge_portexp_state.interrupt_mask != value)
	{
		badge_portexp_state.interrupt_mask = value;
		res = badge_portexp_write_reg(0x11, value);
		if (res != ESP_OK)
			badge_portexp_state.interrupt_mask |= 0x100;
	}

	xSemaphoreGive(badge_portexp_mux);

	return res;
}

void
badge_portexp_set_interrupt_handler(uint8_t pin, badge_portexp_intr_t handler, void *arg)
{
	xSemaphoreTake(badge_portexp_mux, portMAX_DELAY);

	badge_portexp_handlers[pin] = handler;
	badge_portexp_arg[pin] = arg;

	xSemaphoreGive(badge_portexp_mux);
}

int
badge_portexp_get_input(void)
{
	return badge_portexp_read_reg(0x0f);
}

int
badge_portexp_get_interrupt_status(void)
{
	return badge_portexp_read_reg(0x13);
}

#endif // I2C_PORTEXP_ADDR
