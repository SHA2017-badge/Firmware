#include <sdkconfig.h>

#ifdef CONFIG_SHA_BADGE_EINK_DEBUG
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif // CONFIG_SHA_BADGE_EINK_DEBUG

#include <stdbool.h>
#include <stdint.h>

#include <rom/ets_sys.h>
#include <rom/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <soc/gpio_reg.h>
#include <soc/gpio_sig_map.h>
#include <soc/gpio_struct.h>
#include <soc/spi_reg.h>
#include <esp_err.h>
#include <esp_log.h>
#include <driver/gpio.h>

#include "badge_pins.h"
#include "badge_base.h"
#include "badge_eink_dev.h"

#define SPI_NUM 0x3

#ifndef LOW
#define LOW 0
#endif
#ifndef HIGH
#define HIGH 1
#endif

#ifndef VSPICLK_OUT_IDX
// use old define
#define VSPICLK_OUT_IDX VSPICLK_OUT_MUX_IDX
#endif

#ifdef PIN_NUM_EPD_RESET

static const char *TAG = "badge_eink_dev";

esp_err_t
badge_eink_dev_reset(void) {
	esp_err_t res = gpio_set_level(PIN_NUM_EPD_RESET, LOW);
	if (res != ESP_OK)
		return res;

	ets_delay_us(200000);

	res = gpio_set_level(PIN_NUM_EPD_RESET, HIGH);
	if (res != ESP_OK)
		return res;

	ets_delay_us(200000);

	return ESP_OK;
}

bool
badge_eink_dev_is_busy(void)
{
	return gpio_get_level(PIN_NUM_EPD_BUSY);
}

// semaphore to trigger on gde-busy signal
xSemaphoreHandle badge_eink_dev_intr_trigger = NULL;

void
badge_eink_dev_busy_wait(void)
{
	while (badge_eink_dev_is_busy())
	{
		xSemaphoreTake(badge_eink_dev_intr_trigger, 100 / portTICK_PERIOD_MS);
	}
}

void
badge_eink_dev_intr_handler(void *arg)
{ /* in interrupt handler */
	int gpio_state = gpio_get_level(PIN_NUM_EPD_BUSY);

#ifdef CONFIG_SHA_BADGE_EINK_DEBUG
	static int gpio_last_state = -1;
	if (gpio_state != -1 && gpio_last_state != gpio_state)
	{
		ets_printf("badge_eink_dev: EPD-Busy Int %s\n", gpio_state == 0 ? "up" : "down");
	}
	gpio_last_state = gpio_state;
#endif // CONFIG_SHA_BADGE_EINK_DEBUG

#ifdef PIN_NUM_LED
	// pass on BUSY signal to LED.
	gpio_set_level(PIN_NUM_LED, 1 - gpio_state);
#endif // PIN_NUM_LED

//	if (gpio_state == 0)
//	{
		xSemaphoreGiveFromISR(badge_eink_dev_intr_trigger, NULL);
//	}
}

void
badge_eink_dev_write_command(uint8_t command)
{
	badge_eink_dev_busy_wait();

	gpio_set_level(PIN_NUM_EPD_CS, HIGH);
	gpio_set_level(PIN_NUM_EPD_CS, LOW);
	badge_eink_dev_write_byte(command);
	gpio_set_level(PIN_NUM_EPD_CS, HIGH);
}

void
badge_eink_dev_write_command_init(uint8_t command)
{
	badge_eink_dev_busy_wait();

	gpio_set_level(PIN_NUM_EPD_CS, HIGH);
	gpio_set_level(PIN_NUM_EPD_CS, LOW);
	badge_eink_dev_write_byte(command);
	gpio_set_level(PIN_NUM_EPD_DATA, HIGH);
}

void
badge_eink_dev_write_command_end(void)
{
	gpio_set_level(PIN_NUM_EPD_CS, HIGH);
	gpio_set_level(PIN_NUM_EPD_DATA, LOW);
}

esp_err_t
badge_eink_dev_init(void)
{
	static bool badge_eink_dev_init_done = false;

	if (badge_eink_dev_init_done)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	esp_err_t res = badge_base_init();
	if (res != ESP_OK)
		return res;

#ifdef PIN_NUM_LED
	gpio_pad_select_gpio(PIN_NUM_LED);
	res = gpio_set_direction(PIN_NUM_LED, GPIO_MODE_OUTPUT);
	if (res != ESP_OK)
		return res;
#endif // PIN_NUM_LED

	badge_eink_dev_intr_trigger = xSemaphoreCreateBinary();
	if (badge_eink_dev_intr_trigger == NULL)
		return ESP_ERR_NO_MEM;

	res = gpio_isr_handler_add(PIN_NUM_EPD_BUSY, badge_eink_dev_intr_handler, NULL);
	if (res != ESP_OK)
		return res;

	gpio_config_t io_conf = {
		.intr_type    = GPIO_INTR_ANYEDGE,
		.mode         = GPIO_MODE_INPUT,
		.pin_bit_mask = 1LL << PIN_NUM_EPD_BUSY,
		.pull_down_en = 0,
		.pull_up_en   = 1,
	};
	res = gpio_config(&io_conf);
	if (res != ESP_OK)
		return res;

	res = gpio_set_direction(PIN_NUM_EPD_CS, GPIO_MODE_OUTPUT);
	if (res != ESP_OK)
		return res;

	res = gpio_set_direction(PIN_NUM_EPD_DATA, GPIO_MODE_OUTPUT);
	if (res != ESP_OK)
		return res;

	res = gpio_set_direction(PIN_NUM_EPD_RESET, GPIO_MODE_OUTPUT);
	if (res != ESP_OK)
		return res;

	res = gpio_set_direction(PIN_NUM_EPD_BUSY, GPIO_MODE_INPUT);
	if (res != ESP_OK)
		return res;

	gpio_matrix_out(PIN_NUM_EPD_MOSI, VSPID_OUT_IDX, 0, 0);
	gpio_matrix_out(PIN_NUM_EPD_CLK, VSPICLK_OUT_IDX, 0, 0);
	gpio_matrix_out(PIN_NUM_EPD_CS, VSPICS0_OUT_IDX, 0, 0);
	CLEAR_PERI_REG_MASK(SPI_SLAVE_REG(SPI_NUM), SPI_TRANS_DONE << 5);
	SET_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_CS_SETUP);
	CLEAR_PERI_REG_MASK(SPI_PIN_REG(SPI_NUM), SPI_CK_IDLE_EDGE);
	CLEAR_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_CK_OUT_EDGE);
	CLEAR_PERI_REG_MASK(SPI_CTRL_REG(SPI_NUM), SPI_WR_BIT_ORDER);
	CLEAR_PERI_REG_MASK(SPI_CTRL_REG(SPI_NUM), SPI_RD_BIT_ORDER);
	CLEAR_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_DOUTDIN);
	WRITE_PERI_REG(SPI_USER1_REG(SPI_NUM), 0);
	SET_PERI_REG_BITS(SPI_CTRL2_REG(SPI_NUM), SPI_MISO_DELAY_MODE, 0,
			SPI_MISO_DELAY_MODE_S);
	CLEAR_PERI_REG_MASK(SPI_SLAVE_REG(SPI_NUM), SPI_SLAVE_MODE);

	WRITE_PERI_REG(SPI_CLOCK_REG(SPI_NUM),
			(1 << SPI_CLKCNT_N_S) | (1 << SPI_CLKCNT_L_S)); // 40 MHz
//	WRITE_PERI_REG(SPI_CLOCK_REG(SPI_NUM), SPI_CLK_EQU_SYSCLK); // 80Mhz

	SET_PERI_REG_MASK(SPI_USER_REG(SPI_NUM),
			SPI_CS_SETUP | SPI_CS_HOLD | SPI_USR_MOSI);
	SET_PERI_REG_MASK(SPI_CTRL2_REG(SPI_NUM),
			((0x4 & SPI_MISO_DELAY_NUM) << SPI_MISO_DELAY_NUM_S));
	CLEAR_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_USR_COMMAND);
	SET_PERI_REG_BITS(SPI_USER2_REG(SPI_NUM), SPI_USR_COMMAND_BITLEN, 0,
			SPI_USR_COMMAND_BITLEN_S);
	CLEAR_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_USR_ADDR);
	SET_PERI_REG_BITS(SPI_USER1_REG(SPI_NUM), SPI_USR_ADDR_BITLEN, 0,
			SPI_USR_ADDR_BITLEN_S);
	CLEAR_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_USR_MISO);
	SET_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_USR_MOSI);

	char i;
	for (i = 0; i < 16; i++) {
		WRITE_PERI_REG((SPI_W0_REG(SPI_NUM) + (i << 2)), 0);
	}

	badge_eink_dev_init_done = true;

	ESP_LOGD(TAG, "init done");

	return ESP_OK;
}

void
badge_eink_dev_write_byte(uint8_t data)
{
	SET_PERI_REG_BITS(SPI_MOSI_DLEN_REG(SPI_NUM), SPI_USR_MOSI_DBITLEN, 0x7,
			SPI_USR_MOSI_DBITLEN_S);
	WRITE_PERI_REG((SPI_W0_REG(SPI_NUM)), data);
	SET_PERI_REG_MASK(SPI_CMD_REG(SPI_NUM), SPI_USR);

	// wait until ready?
	while (READ_PERI_REG(SPI_CMD_REG(SPI_NUM)) & SPI_USR);
}

#else

// add dummy functions
esp_err_t
badge_eink_dev_reset(void) {
	return ESP_OK;
}

bool
badge_eink_dev_is_busy(void)
{
	return false;
}

void
badge_eink_dev_busy_wait(void)
{
}

void
badge_eink_dev_write_command(uint8_t command)
{
}

void
badge_eink_dev_write_command_init(uint8_t command)
{
}

void
badge_eink_dev_write_command_end(void)
{
}

esp_err_t
badge_eink_dev_init(void)
{
	return ESP_OK;
}

void
badge_eink_dev_write_byte(uint8_t data)
{
}

#endif // PIN_NUM_EPD_RESET
