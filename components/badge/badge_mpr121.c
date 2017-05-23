#include <stdbool.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"

#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "badge_pins.h"
#include "badge_i2c.h"
#include "badge_mpr121.h"

#ifdef I2C_MPR121_ADDR

// define BADGE_MPR121_DEBUG

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
	esp_err_t ret = badge_i2c_read_reg(I2C_MPR121_ADDR, reg, &value);

	if (ret == ESP_OK) {
#ifdef BADGE_MPR121_DEBUG
		ets_printf("i2c read reg(0x%02x): 0x%02x\n", reg, value);
#endif // BADGE_MPR121_DEBUG
		return value;
	} else {
		ets_printf("i2c read reg(0x%02x): error %d\n", reg, ret);
		return -1;
	}
}

static inline int
badge_mpr121_write_reg(uint8_t reg, uint8_t value)
{
	esp_err_t ret = badge_i2c_write_reg(I2C_MPR121_ADDR, reg, value);

	if (ret == ESP_OK) {
#ifdef BADGE_MPR121_DEBUG
		ets_printf("i2c write reg(0x%02x, 0x%02x): ok\n", reg, value);
#endif // BADGE_MPR121_DEBUG
		return 0;
	} else {
		ets_printf("i2c write reg(0x%02x, 0x%02x): error %d\n", reg, value, ret);
		return -1;
	}
}

void
badge_mpr121_intr_task(void *arg)
{
	// we cannot use I2C in the interrupt handler, so we
	// create an extra thread for this..

	while (1)
	{
		if (xSemaphoreTake(badge_mpr121_intr_trigger, portMAX_DELAY))
		{
			int ints = badge_mpr121_get_interrupt_status();
			// NOTE: if ints = -1, then all handlers will trigger.

			int i;
			for (i=0; i<8; i++)
			{
				if (ints & (1 << i))
				{
					xSemaphoreTake(badge_mpr121_mux, portMAX_DELAY);
					badge_mpr121_intr_t handler = badge_mpr121_handlers[i];
					void *arg = badge_mpr121_arg[i];
					xSemaphoreGive(badge_mpr121_mux);

					if (handler != NULL)
						handler(arg);
				}
			}
		}
	}
}

void
badge_mpr121_intr_handler(void *arg)
{

	int gpio_state = gpio_get_level(PIN_NUM_MPR121_INT);
#ifdef BADGE_MPR121_DEBUG
	static int gpio_last_state = -1;
	if (gpio_last_state != gpio_state)
	{
		if (gpio_state == 1)
			ets_printf("I2C Int down\n");
		else if (gpio_state == 0)
			ets_printf("I2C Int up\n");
	}
	gpio_last_state = gpio_state;
#endif // BADGE_MPR121_DEBUG

	if (gpio_state == 0)
	{
		xSemaphoreGiveFromISR(badge_mpr121_intr_trigger, NULL);
	}
}

void
badge_mpr121_init(void)
{
	badge_mpr121_mux = xSemaphoreCreateMutex();
	badge_mpr121_intr_trigger = xSemaphoreCreateBinary();
	gpio_isr_handler_add(PIN_NUM_MPR121_INT, badge_mpr121_intr_handler, NULL);
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_ANYEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = 1LL << PIN_NUM_MPR121_INT;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);

	badge_mpr121_write_reg(0x80, 0x63); // soft reset
	badge_mpr121_write_reg(0x7b, 0x33); // auto-config
	badge_mpr121_write_reg(0x7d, 200); // auto-config
	badge_mpr121_write_reg(0x7e, 100); // auto-config
	badge_mpr121_write_reg(0x7f, 180); // auto-config
	badge_mpr121_write_reg(0x5e, 0x08); // enable run-mode

	xTaskCreate(&badge_mpr121_intr_task, "MPR121 interrupt task", 4096, NULL, 10, NULL);

	badge_mpr121_intr_handler(NULL);
}

void
badge_mpr121_set_interrupt_handler(uint8_t pin, badge_mpr121_intr_t handler, void *arg)
{
	xSemaphoreTake(badge_mpr121_mux, portMAX_DELAY);

	badge_mpr121_handlers[pin] = handler;
	badge_mpr121_arg[pin] = arg;

	xSemaphoreGive(badge_mpr121_mux);
}

int
badge_mpr121_get_interrupt_status(void)
{
	int r0 = badge_mpr121_read_reg(0x00);
	int r1 = badge_mpr121_read_reg(0x01);
	if (r0 == -1 || r1 == -1)
		return -1;
	return (r1 << 8) | r0;
}

#endif // I2C_MPR121_ADDR
