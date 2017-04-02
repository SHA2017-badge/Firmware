#include <stdbool.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>
#include "driver/i2c.h"

#include "sdkconfig.h"

#include "badge_i2c.h"

#ifdef CONFIG_SHA_BADGE_V2

//define BADGE_I2C_DEBUG

#define PIN_NUM_I2C_INT      25
#define PIN_NUM_I2C_DATA     26
#define PIN_NUM_I2C_CLOCK    27

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
struct portexp_state_t {
	uint16_t io_direction;         // default is 0x00
	uint16_t output_state;         // default is 0x00
	uint16_t output_high_z;        // default is 0xff
	uint16_t input_default_state;  // default is 0x00
	uint16_t pull_enable;          // default is 0xff
	uint16_t pull_down_up;         // default is 0x00
	uint16_t interrupt_mask;       // default is 0x00
};

xSemaphoreHandle badge_i2c_mux;
xQueueHandle badge_i2c_trigger = NULL;
struct portexp_state_t portexp_state;

extern xQueueHandle evt_queue; // hack

static inline int
portexp_read_reg(uint8_t reg)
{
	uint8_t value;

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
portexp_write_reg(uint8_t reg, uint8_t value)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ( I2C_PORTEXP_ID << 1 ) | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, value, ACK_CHECK_EN);
	i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

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
touch_read_event(void)
{
	uint8_t buf[3];

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ( I2C_TOUCHPAD_ID << 1 ) | READ_BIT, ACK_CHECK_EN);
	i2c_master_read(cmd, buf, 3, ACK_VAL);
	i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

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
task_handle_badge_i2c(void *arg)
{
	// we cannot handle interrupts in the interrupt handler, so we
	// create an extra thread for this..

	while (1)
	{
		uint32_t dummy;
		if (xQueueReceive(badge_i2c_trigger, &dummy, portMAX_DELAY))
		{
			while (1)
			{
				// read touch-controller interrupt line
				int x = portexp_get_input();
				if (x == -1) // error
					continue; // retry..

				if ((x & 0x08) != 0) // no events waiting
				{
					// let port-expander know we have handled this interrupt
					portexp_get_interrupt_status();

					// double-check to avoid race-conditions
					int x = portexp_get_input();
					if (x == -1) // error
						continue; // retry..

					if ((x & 0x08) != 0) // no events waiting
						break;
				}

				// event waiting
				xSemaphoreTake(badge_i2c_mux, portMAX_DELAY);
				int event = touch_read_event();
				xSemaphoreGive(badge_i2c_mux);

				if (event == -1) // error
					continue;

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
	}
}

void
portexp_trigger_event(void)
{
	uint32_t dummy = 0;
	xQueueSendFromISR(badge_i2c_trigger, &dummy, NULL);
}

void
badge_i2c_init(void)
{
	// create mutex for I2C bus
	badge_i2c_mux = xSemaphoreCreateMutex();
	badge_i2c_trigger = xQueueCreate(10, sizeof(uint32_t));

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
//	portexp_write_reg(0x01, 0x01); // sw reset
	portexp_write_reg(0x03, 0x04);
	portexp_write_reg(0x05, 0x04);
	portexp_write_reg(0x07, 0xfb);
	portexp_write_reg(0x09, 0x08);
	portexp_write_reg(0x0b, 0xff);
	portexp_write_reg(0x0d, 0x00);
	portexp_write_reg(0x11, 0xf7);
	portexp_write_reg(0x13, 0x00);
	struct portexp_state_t init_state = {
		.io_direction        = 0x04,
		.output_state        = 0x04,
		.output_high_z       = 0xfb,
		.input_default_state = 0x08,
		.pull_enable         = 0xff,
		.pull_down_up        = 0x00,
		.interrupt_mask      = 0xf7,
	};
	memcpy(&portexp_state, &init_state, sizeof(init_state));

	/* configure led output */
	portexp_set_output_state(2, 1);
	portexp_set_output_high_z(2, 0);
	portexp_set_io_direction(2, 1);

	/* configure touch-controller */
	portexp_set_input_default_state(3, 1);
	portexp_set_interrupt_enable(3, 1);

	xTaskCreate(&task_handle_badge_i2c, "i2c event task", 4096, NULL, 10, NULL);

	// it seems that we need to read some registers to start interrupt handling.. (?)
	portexp_read_reg(0x01);
	portexp_read_reg(0x03);
	portexp_read_reg(0x05);
	portexp_read_reg(0x07);
	portexp_read_reg(0x09);
	portexp_read_reg(0x0b);
	portexp_read_reg(0x0d);
	portexp_read_reg(0x0f);
	portexp_read_reg(0x11);
	portexp_read_reg(0x13);

	portexp_trigger_event();
}

int
portexp_set_io_direction(uint8_t pin, uint8_t direction)
{
	xSemaphoreTake(badge_i2c_mux, portMAX_DELAY);

	uint8_t value = portexp_state.io_direction;
	if (direction)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	int res = 0;
	if (portexp_state.io_direction != value)
	{
		portexp_state.io_direction = value;
		res = portexp_write_reg(0x03, value);
		if (res == -1)
			portexp_state.io_direction |= 0x100;
	}

	xSemaphoreGive(badge_i2c_mux);

	return res;
}

int
portexp_set_output_state(uint8_t pin, uint8_t state)
{
	xSemaphoreTake(badge_i2c_mux, portMAX_DELAY);

	uint8_t value = portexp_state.output_state;
	if (state)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	int res = 0;
	if (portexp_state.output_state != value)
	{
		portexp_state.output_state = value;
		res = portexp_write_reg(0x05, value);
		if (res == -1)
			portexp_state.output_state |= 0x100;
	}

	xSemaphoreGive(badge_i2c_mux);

	return res;
}

int
portexp_set_output_high_z(uint8_t pin, uint8_t high_z)
{
	xSemaphoreTake(badge_i2c_mux, portMAX_DELAY);

	uint8_t value = portexp_state.output_high_z;
	if (high_z)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	int res = 0;
	if (portexp_state.output_high_z != value)
	{
		portexp_state.output_high_z = value;
		res = portexp_write_reg(0x07, value);
		if (res == -1)
			portexp_state.output_high_z |= 0x100;
	}

	xSemaphoreGive(badge_i2c_mux);

	return res;
}

int
portexp_set_input_default_state(uint8_t pin, uint8_t state)
{
	xSemaphoreTake(badge_i2c_mux, portMAX_DELAY);

	uint8_t value = portexp_state.input_default_state;
	if (state)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	int res = 0;
	if (portexp_state.input_default_state != value)
	{
		portexp_state.input_default_state = value;
		res = portexp_write_reg(0x09, value);
		if (res == -1)
			portexp_state.input_default_state |= 0x100;
	}

	xSemaphoreGive(badge_i2c_mux);

	return res;
}

int
portexp_set_pull_enable(uint8_t pin, uint8_t enable)
{
	xSemaphoreTake(badge_i2c_mux, portMAX_DELAY);

	uint8_t value = portexp_state.pull_enable;
	if (enable)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	int res = 0;
	if (portexp_state.pull_enable != value)
	{
		portexp_state.pull_enable = value;
		res = portexp_write_reg(0x0b, value);
		if (res == -1)
			portexp_state.pull_enable |= 0x100;
	}

	xSemaphoreGive(badge_i2c_mux);

	return res;
}

int
portexp_set_pull_down_up(uint8_t pin, uint8_t up)
{
	xSemaphoreTake(badge_i2c_mux, portMAX_DELAY);

	uint8_t value = portexp_state.pull_down_up;
	if (up)
		value |= 1 << pin;
	else
		value &= ~(1 << pin);

	int res = 0;
	if (portexp_state.pull_down_up != value)
	{
		portexp_state.pull_down_up = value;
		res = portexp_write_reg(0x0d, value);
		if (res == -1)
			portexp_state.pull_down_up |= 0x100;
	}

	xSemaphoreGive(badge_i2c_mux);

	return res;
}

int
portexp_set_interrupt_enable(uint8_t pin, uint8_t enable)
{
	xSemaphoreTake(badge_i2c_mux, portMAX_DELAY);

	uint8_t value = portexp_state.interrupt_mask;
	if (enable)
		value &= ~(1 << pin);
	else
		value |= 1 << pin;

	int res = 0;
	if (portexp_state.interrupt_mask != value)
	{
		portexp_state.interrupt_mask = value;
		res = portexp_write_reg(0x11, value);
		if (res == -1)
			portexp_state.interrupt_mask |= 0x100;
	}

	xSemaphoreGive(badge_i2c_mux);

	return res;
}

int
portexp_get_input(void)
{
	xSemaphoreTake(badge_i2c_mux, portMAX_DELAY);
	int res = portexp_read_reg(0x0f);
	xSemaphoreGive(badge_i2c_mux);
	return res;
}

int
portexp_get_interrupt_status(void)
{
	xSemaphoreTake(badge_i2c_mux, portMAX_DELAY);
	int res = portexp_read_reg(0x13);
	xSemaphoreGive(badge_i2c_mux);
	return res;
}

#endif // CONFIG_SHA_BADGE_V2
