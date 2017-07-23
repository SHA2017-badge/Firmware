#include <sdkconfig.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <esp_log.h>
#include <driver/i2c.h>

#include "badge_pins.h"
#include "badge_i2c.h"

#ifdef PIN_NUM_I2C_CLK

#define I2C_MASTER_NUM             I2C_NUM_1
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

static const char *TAG = "badge_i2c";

// mutex for accessing the I2C bus
static xSemaphoreHandle badge_i2c_mux = NULL;

esp_err_t
badge_i2c_init(void)
{
	static bool badge_i2c_init_done = false;

	if (badge_i2c_init_done)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	// create mutex for I2C bus
	badge_i2c_mux = xSemaphoreCreateMutex();
	if (badge_i2c_mux == NULL)
		return ESP_ERR_NO_MEM;

	// configure I2C
	i2c_config_t conf = {
		.mode             = I2C_MODE_MASTER,
		.sda_io_num       = PIN_NUM_I2C_DATA,
		.sda_pullup_en    = GPIO_PULLUP_ENABLE,
//		.sda_pullup_en    = GPIO_PULLUP_DISABLE,
		.scl_io_num       = PIN_NUM_I2C_CLK,
		.scl_pullup_en    = GPIO_PULLUP_ENABLE,
//		.scl_pullup_en    = GPIO_PULLUP_DISABLE,
		.master.clk_speed = I2C_MASTER_FREQ_HZ,
	};
	esp_err_t res = i2c_param_config(I2C_MASTER_NUM, &conf);
	if (res != ESP_OK)
		return res;

	res = i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
	if (res != ESP_OK)
		return res;

	badge_i2c_init_done = true;

	ESP_LOGD(TAG, "init done");

	return ESP_OK;
}

esp_err_t
badge_i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *value, size_t value_len)
{
	esp_err_t res;
	if (xSemaphoreTake(badge_i2c_mux, portMAX_DELAY) != pdTRUE)
		return ESP_ERR_TIMEOUT;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	res = i2c_master_start(cmd);
	assert( res == ESP_OK );
	res = i2c_master_write_byte(cmd, ( addr << 1 ) | WRITE_BIT, ACK_CHECK_EN);
	assert( res == ESP_OK );
	res = i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
	assert( res == ESP_OK );

	res = i2c_master_start(cmd);
	assert( res == ESP_OK );
	res = i2c_master_write_byte(cmd, ( addr << 1 ) | READ_BIT, ACK_CHECK_EN);
	assert( res == ESP_OK );
	if (value_len > 1)
	{
		res = i2c_master_read(cmd, value, value_len-1, ACK_VAL);
		assert( res == ESP_OK );
	}
	res = i2c_master_read_byte(cmd, &value[value_len-1], NACK_VAL);
	assert( res == ESP_OK );
	res = i2c_master_stop(cmd);
	assert( res == ESP_OK );

	res = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (xSemaphoreGive(badge_i2c_mux) != pdTRUE)
	{
		ESP_LOGE(TAG, "xSemaphoreGive() did not return pdTRUE.");
	}

	return res;
}

esp_err_t
badge_i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
	esp_err_t res;
	if (xSemaphoreTake(badge_i2c_mux, portMAX_DELAY) != pdTRUE)
		return ESP_ERR_TIMEOUT;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	res = i2c_master_start(cmd);
	assert( res == ESP_OK );
	res = i2c_master_write_byte(cmd, ( addr << 1 ) | WRITE_BIT, ACK_CHECK_EN);
	assert( res == ESP_OK );
	res = i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
	assert( res == ESP_OK );
	res = i2c_master_write_byte(cmd, value, ACK_CHECK_EN);
	assert( res == ESP_OK );
	res = i2c_master_stop(cmd);
	assert( res == ESP_OK );

	res = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (xSemaphoreGive(badge_i2c_mux) != pdTRUE)
	{
		ESP_LOGE(TAG, "xSemaphoreGive() did not return pdTRUE.");
	}

	return res;
}

esp_err_t
badge_i2c_read_event(uint8_t addr, uint8_t *buf)
{
	esp_err_t res;
	if (xSemaphoreTake(badge_i2c_mux, portMAX_DELAY) != pdTRUE)
		return ESP_ERR_TIMEOUT;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	res = i2c_master_start(cmd);
	assert( res == ESP_OK );
	res = i2c_master_write_byte(cmd, ( addr << 1 ) | READ_BIT, ACK_CHECK_EN);
	assert( res == ESP_OK );
	res = i2c_master_read(cmd, buf, 2, ACK_VAL);
	assert( res == ESP_OK );
	res = i2c_master_read_byte(cmd, &buf[2], NACK_VAL);
	assert( res == ESP_OK );
	res = i2c_master_stop(cmd);
	assert( res == ESP_OK );

	res = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (xSemaphoreGive(badge_i2c_mux) != pdTRUE)
	{
		ESP_LOGE(TAG, "xSemaphoreGive() did not return pdTRUE.");
	}

	return res;
}

#endif // PIN_NUM_I2C_CLK
