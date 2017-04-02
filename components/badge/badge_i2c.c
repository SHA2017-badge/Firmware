#include <stdbool.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>
#include "driver/i2c.h"

#include "sdkconfig.h"

#include "badge_pins.h"
#include "badge_i2c.h"

#ifdef PIN_NUM_I2C_CLOCK

//define BADGE_I2C_DEBUG

#define I2C_MASTER_NUM             I2C_NUM_1
#define I2C_PORTEXP_ID             0x44
#define I2C_TOUCHPAD_ID            0x78
#define I2C_MASTER_FREQ_HZ         100000
//define I2C_MASTER_FREQ_HZ        400000
#define I2C_MASTER_TX_BUF_DISABLE  0
#define I2C_MASTER_RX_BUF_DISABLE  0

#define WRITE_BIT      I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT       I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave */
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL        0x0     /*!< I2C ack value */
#define NACK_VAL       0x1     /*!< I2C nack value */

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

// mutex for accessing the I2C bus
xSemaphoreHandle badge_i2c_mux = NULL;

// mutex for accessing badge_portexp_state, badge_portexp_handlers, etc..
xSemaphoreHandle badge_portexp_mux = NULL;

// semaphore to trigger port-expander interrupt handling
xSemaphoreHandle badge_portexp_intr_trigger = NULL;

// port-expander state
struct badge_portexp_state_t badge_portexp_state;

// handlers per port-expander port.
gpio_isr_t badge_portexp_handlers[8] = { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
void* badge_portexp_arg[8] = { NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

extern xQueueHandle evt_queue; // hack

static inline int
badge_portexp_read_reg(uint8_t reg)
{
	uint8_t value;

	xSemaphoreTake(badge_i2c_mux, portMAX_DELAY);

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ( I2C_PORTEXP_ID << 1 ) | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ( I2C_PORTEXP_ID << 1 ) | READ_BIT, ACK_CHECK_EN);
	i2c_master_read_byte(cmd, &value, NACK_VAL);
	i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	xSemaphoreGive(badge_i2c_mux);

	if (ret == ESP_OK) {
#ifdef BADGE_I2C_DEBUG
		ets_printf("i2c read reg(0x%02x): 0x%02x\n", reg, value);
#endif // BADGE_I2C_DEBUG
		return value;
	} else {
		ets_printf("i2c read reg(0x%02x): error %d\n", reg, ret);
		return -1;
	}
}

static inline int
badge_portexp_write_reg(uint8_t reg, uint8_t value)
{
	xSemaphoreTake(badge_i2c_mux, portMAX_DELAY);

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ( I2C_PORTEXP_ID << 1 ) | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, value, ACK_CHECK_EN);
	i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	xSemaphoreGive(badge_i2c_mux);

	if (ret == ESP_OK) {
#ifdef BADGE_I2C_DEBUG
		ets_printf("i2c write reg(0x%02x, 0x%02x): ok\n", reg, value);
#endif // BADGE_I2C_DEBUG
		return 0;
	} else {
		ets_printf("i2c write reg(0x%02x, 0x%02x): error %d\n", reg, value, ret);
		return -1;
	}
}

static inline int
badge_touch_read_event(void)
{
	uint8_t buf[3];

	xSemaphoreTake(badge_i2c_mux, portMAX_DELAY);

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ( I2C_TOUCHPAD_ID << 1 ) | READ_BIT, ACK_CHECK_EN);
	i2c_master_read(cmd, buf, 3, ACK_VAL);
	i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	xSemaphoreGive(badge_i2c_mux);

	if (ret == ESP_OK) {
#ifdef BADGE_I2C_DEBUG
		ets_printf("event: 0x%02x, 0x%02x, 0x%02x\n", buf[0], buf[1], buf[2]);
#endif // BADGE_I2C_DEBUG
		return (buf[0] << 16) | (buf[1] << 8) | (buf[2]);
	} else {
		ets_printf("i2c master read (touch-controller): error %d\n", ret);
		return -1;
	}
}

void
badge_touch_intr_handler(void *arg)
{
	while (1)
	{
		// read touch-controller interrupt line
		int x = badge_portexp_get_input();
		if (x == -1) // error
			continue; // retry..

		if (x & (1 << 3)) // no events waiting
			break;

		// event waiting
		int event = badge_touch_read_event();

		if (event == -1) // error
			continue;

		// FIXME: should handle this with a callback.
		// convert into button queue event
		if (((event >> 16) & 0x0f) == 0x0) { // button down event
			static const int conv[12] =
				{ -1, -1, 5, 3, 6, 4, -1, 0, 2, 1, -1, 0 };
			if (((event >> 8) & 0xff) < 12) {
				int id = conv[(event >> 8) & 0xff];
				if (id != -1)
				{
					uint32_t buttons_down = 1<<id;
					xQueueSendFromISR(evt_queue, &buttons_down, NULL);
				}
			}
		}
	}
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
					gpio_isr_t handler = badge_portexp_handlers[i];
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

	int gpio_state = gpio_get_level(PIN_NUM_I2C_INT);
#ifdef BADGE_I2C_DEBUG
	static int gpio_last_state = -1;
	if (gpio_last_state != gpio_state)
	{
		if (gpio_state == 1)
			ets_printf("I2C Int down\n");
		else if (gpio_state == 0)
			ets_printf("I2C Int up\n");
	}
	gpio_last_state = gpio_state;
#endif // BADGE_I2C_DEBUG

	if (gpio_state == 0)
	{
		xSemaphoreGiveFromISR(badge_portexp_intr_trigger, NULL);
	}
}

void
badge_touch_init(void)
{
	badge_portexp_set_input_default_state(3, 1);
	badge_portexp_set_interrupt_enable(3, 1);
	badge_portexp_set_interrupt_handler(3, badge_touch_intr_handler, NULL);
}

void
badge_portexp_init(void)
{
	badge_portexp_mux = xSemaphoreCreateMutex();
	badge_portexp_intr_trigger = xSemaphoreCreateBinary();
	gpio_isr_handler_add(PIN_NUM_I2C_INT, badge_portexp_intr_handler, NULL);
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_ANYEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = 1LL << PIN_NUM_I2C_INT;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);

//	badge_portexp_write_reg(0x01, 0x01); // sw reset
	badge_portexp_write_reg(0x03, 0x04);
	badge_portexp_write_reg(0x05, 0x04);
	badge_portexp_write_reg(0x07, 0xfb);
	badge_portexp_write_reg(0x09, 0x08);
	badge_portexp_write_reg(0x0b, 0xff);
	badge_portexp_write_reg(0x0d, 0x00);
	badge_portexp_write_reg(0x11, 0xf7);
	badge_portexp_write_reg(0x13, 0x00);
	struct badge_portexp_state_t init_state = {
		.io_direction        = 0x04,
		.output_state        = 0x04,
		.output_high_z       = 0xfb,
		.input_default_state = 0x08,
		.pull_enable         = 0xff,
		.pull_down_up        = 0x00,
		.interrupt_mask      = 0xf7,
	};
	memcpy(&badge_portexp_state, &init_state, sizeof(init_state));
	xTaskCreate(&badge_portexp_intr_task, "port-expander interrupt task", 4096, NULL, 10, NULL);

	/* configure led output */
	badge_portexp_set_output_state(2, 1);
	badge_portexp_set_output_high_z(2, 0);
	badge_portexp_set_io_direction(2, 1);

	/* configure touch-controller */
	badge_touch_init();

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
}

void
badge_i2c_init(void)
{
	// create mutex for I2C bus
	badge_i2c_mux = xSemaphoreCreateMutex();

	// configure I2C
	i2c_config_t conf;
	memset(&conf, 0, sizeof(i2c_config_t));
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = PIN_NUM_I2C_DATA;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
//	conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
	conf.scl_io_num = PIN_NUM_I2C_CLOCK;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
//	conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
	conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
	i2c_param_config(I2C_MASTER_NUM, &conf);

	i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);

	/* configure port-expander */
	badge_portexp_init();
}

int
badge_portexp_set_io_direction(uint8_t pin, uint8_t direction)
{
	xSemaphoreTake(badge_portexp_mux, portMAX_DELAY);

	uint8_t value = badge_portexp_state.io_direction;
	if (direction)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	int res = 0;
	if (badge_portexp_state.io_direction != value)
	{
		badge_portexp_state.io_direction = value;
		res = badge_portexp_write_reg(0x03, value);
		if (res == -1)
			badge_portexp_state.io_direction |= 0x100;
	}

	xSemaphoreGive(badge_portexp_mux);

	return res;
}

int
badge_portexp_set_output_state(uint8_t pin, uint8_t state)
{
	xSemaphoreTake(badge_portexp_mux, portMAX_DELAY);

	uint8_t value = badge_portexp_state.output_state;
	if (state)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	int res = 0;
	if (badge_portexp_state.output_state != value)
	{
		badge_portexp_state.output_state = value;
		res = badge_portexp_write_reg(0x05, value);
		if (res == -1)
			badge_portexp_state.output_state |= 0x100;
	}

	xSemaphoreGive(badge_portexp_mux);

	return res;
}

int
badge_portexp_set_output_high_z(uint8_t pin, uint8_t high_z)
{
	xSemaphoreTake(badge_portexp_mux, portMAX_DELAY);

	uint8_t value = badge_portexp_state.output_high_z;
	if (high_z)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	int res = 0;
	if (badge_portexp_state.output_high_z != value)
	{
		badge_portexp_state.output_high_z = value;
		res = badge_portexp_write_reg(0x07, value);
		if (res == -1)
			badge_portexp_state.output_high_z |= 0x100;
	}

	xSemaphoreGive(badge_portexp_mux);

	return res;
}

int
badge_portexp_set_input_default_state(uint8_t pin, uint8_t state)
{
	xSemaphoreTake(badge_portexp_mux, portMAX_DELAY);

	uint8_t value = badge_portexp_state.input_default_state;
	if (state)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	int res = 0;
	if (badge_portexp_state.input_default_state != value)
	{
		badge_portexp_state.input_default_state = value;
		res = badge_portexp_write_reg(0x09, value);
		if (res == -1)
			badge_portexp_state.input_default_state |= 0x100;
	}

	xSemaphoreGive(badge_portexp_mux);

	return res;
}

int
badge_portexp_set_pull_enable(uint8_t pin, uint8_t enable)
{
	xSemaphoreTake(badge_portexp_mux, portMAX_DELAY);

	uint8_t value = badge_portexp_state.pull_enable;
	if (enable)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	int res = 0;
	if (badge_portexp_state.pull_enable != value)
	{
		badge_portexp_state.pull_enable = value;
		res = badge_portexp_write_reg(0x0b, value);
		if (res == -1)
			badge_portexp_state.pull_enable |= 0x100;
	}

	xSemaphoreGive(badge_portexp_mux);

	return res;
}

int
badge_portexp_set_pull_down_up(uint8_t pin, uint8_t up)
{
	xSemaphoreTake(badge_portexp_mux, portMAX_DELAY);

	uint8_t value = badge_portexp_state.pull_down_up;
	if (up)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	int res = 0;
	if (badge_portexp_state.pull_down_up != value)
	{
		badge_portexp_state.pull_down_up = value;
		res = badge_portexp_write_reg(0x0d, value);
		if (res == -1)
			badge_portexp_state.pull_down_up |= 0x100;
	}

	xSemaphoreGive(badge_portexp_mux);

	return res;
}

int
badge_portexp_set_interrupt_enable(uint8_t pin, uint8_t enable)
{
	xSemaphoreTake(badge_portexp_mux, portMAX_DELAY);

	uint8_t value = badge_portexp_state.interrupt_mask;
	if (enable)
		value &= ~(1 << pin);
	else
		value |= 1 << pin;

	int res = 0;
	if (badge_portexp_state.interrupt_mask != value)
	{
		badge_portexp_state.interrupt_mask = value;
		res = badge_portexp_write_reg(0x11, value);
		if (res == -1)
			badge_portexp_state.interrupt_mask |= 0x100;
	}

	xSemaphoreGive(badge_portexp_mux);

	return res;
}

void
badge_portexp_set_interrupt_handler(uint8_t pin, gpio_isr_t handler, void *arg)
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

#endif // PIN_NUM_I2C_CLOCK
