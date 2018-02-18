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
#include <string.h>
#include <soc/gpio_reg.h>
#include <soc/gpio_sig_map.h>
#include <soc/gpio_struct.h>
#include <soc/spi_reg.h>
#include <esp_err.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_heap_caps.h>

#include "badge_pins.h"
#include "badge_base.h"
#include "badge_eink_dev.h"

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

// this number should be a multiply of 4
// to transfer complete dwords in one go
#define SPI_TRANSFER_SIZE 128
spi_device_handle_t spi_bus;

static const char *TAG = "badge_eink_dev";

enum badge_eink_dev_t badge_eink_dev_type = BADGE_EINK_DEFAULT;

#ifdef PIN_NUM_EPD_RESET

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
#if defined(CONFIG_SHA_BADGE_EINK_DEBUG) || defined(PIN_NUM_LED)
	int gpio_state = gpio_get_level(PIN_NUM_EPD_BUSY);
#endif

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


/* This function is called (in irq context!) just before a transmission starts.
 * It will set the D/C line to the value indicated in the user field
 */
static
void badge_spi_pre_transfer_callback(spi_transaction_t *t)
{
	uint8_t dc_level = *((uint8_t *) t->user);
	gpio_set_level(PIN_NUM_EPD_DATA, (int) dc_level);
}


void
badge_spi_send(const uint8_t *data, int len, const uint8_t dc_level)
{
	esp_err_t ret;
	if (len == 0) {
		return;
	}
	spi_transaction_t t = {
		.length = len * 8,  // transaction length is in bits
		.tx_buffer = data,
		.user = (void *) &dc_level,
	};
	ret = spi_device_transmit(spi_bus, &t);
	assert(ret == ESP_OK);
}

void
badge_eink_dev_write_command(uint8_t command)
{
	badge_eink_dev_busy_wait();
	badge_spi_send(&command, 1, LOW);
}

void
badge_eink_dev_write_byte(uint8_t data)
{
	badge_eink_dev_busy_wait();
	badge_spi_send(&data, 1, HIGH);
}

void
badge_eink_dev_write_command_stream(uint8_t command, const uint8_t *data,
										 unsigned int datalen)
{
	ESP_LOGI(TAG, "Sending SPI stream of %d bytes", datalen);
	badge_eink_dev_write_command(command);
	badge_spi_send(data, datalen, HIGH);
	ESP_LOGI(TAG, "Done");
}

/* This function is dedicated to copy memory allocated with MALLOC_CAP_32BIT
 * Such a memory can only be accessed via 32-bit reads and writes,
 * any other type of access will generate a fatal LoadStoreError exception.
 */
static void
memcpy_u8_u32(uint8_t *dst, const uint32_t *src, size_t size)
{
	while (size-- > 0)
	{
		uint32_t data_src = *src++;
		*dst++ = data_src >> 24;
		*dst++ = data_src >> 16;
		*dst++ = data_src >> 8;
		*dst++ = data_src;
	}
}

/* Send uint32_t stream in chunks of SPI_TRANSFER_SIZE
 * to use to the maximum the size of memory allocated in the SPI buffer
 */
void
badge_eink_dev_write_command_stream_u32(uint8_t command, const uint32_t *data,
										 unsigned int datalen)
{
	assert(SPI_TRANSFER_SIZE % 4 == 0);

	uint8_t* data_tmpbuf = heap_caps_malloc(SPI_TRANSFER_SIZE, MALLOC_CAP_8BIT);
	ESP_LOGI(TAG, "Sending SPI stream of %d dwords...", datalen);
	if (data_tmpbuf == NULL){
		ESP_LOGE(TAG, "Failed to allocate memory!");
	}
	while (datalen >= SPI_TRANSFER_SIZE / 4){
		memcpy_u8_u32(data_tmpbuf, data, SPI_TRANSFER_SIZE / 4);
		badge_eink_dev_write_command(command);
		badge_spi_send(data_tmpbuf, SPI_TRANSFER_SIZE, HIGH);
		data += SPI_TRANSFER_SIZE / 4;
		datalen -= SPI_TRANSFER_SIZE / 4;
	}
	// send remaining data if < SPI_TRANSFER_SIZE / 4
	if (datalen > 0) {
		memcpy_u8_u32(data_tmpbuf, data, datalen);
		badge_eink_dev_write_command(command);
		badge_spi_send(data_tmpbuf, datalen*4, HIGH);
	}
	heap_caps_free(data_tmpbuf);
	ESP_LOGI(TAG, "Done");
}

esp_err_t
badge_eink_dev_init(enum badge_eink_dev_t dev_type)
{
	static bool badge_eink_dev_init_done = false;

	if (badge_eink_dev_init_done)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	badge_eink_dev_type = dev_type;

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

	spi_bus_config_t buscfg = {
		.mosi_io_num = PIN_NUM_EPD_MOSI,
		// MISO is not used, we are transferring to the slave only
		.miso_io_num = -1,
		.sclk_io_num = PIN_NUM_EPD_CLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = SPI_TRANSFER_SIZE,
	};
	spi_device_interface_config_t devcfg = {
		.clock_speed_hz = 20 * 1000 * 1000,
		.mode = 0,  // SPI mode 0
		.spics_io_num = PIN_NUM_EPD_CS,
		.queue_size = 1,
		// We are sending only in one direction (to the ePaper slave)
		.flags = (SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_3WIRE),
		//Specify pre-transfer callback to handle D/C line
		.pre_cb = badge_spi_pre_transfer_callback,
	};
	res = spi_bus_initialize(VSPI_HOST, &buscfg, 2);
	assert(res == ESP_OK);
	res = spi_bus_add_device(VSPI_HOST, &devcfg, &spi_bus);
	assert(res == ESP_OK);

	badge_eink_dev_init_done = true;

	ESP_LOGD(TAG, "init done");

	return ESP_OK;
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


esp_err_t
badge_eink_dev_init(enum badge_eink_dev_t dev_type)
{
	return ESP_OK;
}

void
badge_eink_dev_write_byte(uint8_t data)
{
}

void
badge_eink_dev_write_command_stream(uint8_t command, const uint8_t *data,
										 unsigned int datalen)
{
}

void
badge_eink_dev_write_command_stream_u32(uint8_t command, const uint32_t *data,
										 unsigned int datalen)
{
}

#endif // PIN_NUM_EPD_RESET
