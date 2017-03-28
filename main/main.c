#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include <gde.h>
#include <gdeh029a1.h>
#include <pictures.h>
#include <pins.h>
#include <string.h>

#ifdef CONFIG_SHA_BADGE_V1
#define PIN_NUM_LED          22
#define PIN_NUM_BUTTON_A      0
#define PIN_NUM_BUTTON_B     27
#define PIN_NUM_BUTTON_MID   25
#define PIN_NUM_BUTTON_UP    26
#define PIN_NUM_BUTTON_DOWN  32
#define PIN_NUM_BUTTON_LEFT  33
#define PIN_NUM_BUTTON_RIGHT 35
#else
#define PIN_NUM_BUTTON_FLASH  0
#define PIN_NUM_I2C_INT      25
#define PIN_NUM_I2C_DATA     26
#define PIN_NUM_I2C_CLOCK    27
#endif

//esp_err_t event_handler(void *ctx, system_event_t *event) { return ESP_OK; }

uint32_t
get_buttons(void)
{
	uint32_t bits = 0;
#ifdef CONFIG_SHA_BADGE_V1
	bits |= gpio_get_level(PIN_NUM_BUTTON_A)     << 0; // A
	bits |= gpio_get_level(PIN_NUM_BUTTON_B)     << 1; // B
	bits |= gpio_get_level(PIN_NUM_BUTTON_MID)   << 2; // MID
	bits |= gpio_get_level(PIN_NUM_BUTTON_UP)    << 3; // UP
	bits |= gpio_get_level(PIN_NUM_BUTTON_DOWN)  << 4; // DOWN
	bits |= gpio_get_level(PIN_NUM_BUTTON_LEFT)  << 5; // LEFT
	bits |= gpio_get_level(PIN_NUM_BUTTON_RIGHT) << 6; // RIGHT
#else
	bits |= gpio_get_level(PIN_NUM_BUTTON_FLASH) << 7; // FLASH
	bits |= gpio_get_level(PIN_NUM_I2C_INT)      << 9; // I2C
#endif // CONFIG_SHA_BADGE_V1
	bits |= gpio_get_level(PIN_NUM_BUSY)         << 8; // GDE BUSY
	return bits;
}

#include "event_queue.h"

uint32_t buttons_state = 0;

void gpio_intr_test(void *arg) {
  // read status to get interrupt status for GPIO 0-31
  uint32_t gpio_intr_status_lo = READ_PERI_REG(GPIO_STATUS_REG);
  // read status to get interrupt status for GPIO 32-39
  uint32_t gpio_intr_status_hi = READ_PERI_REG(GPIO_STATUS1_REG);
  // clear intr for GPIO 0-31
  SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status_lo);
  // clear intr for GPIO 32-39
  SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_hi);

  uint32_t buttons_new = get_buttons();
  uint32_t buttons_down = (~buttons_new) & buttons_state;
  uint32_t buttons_up = buttons_new & (~buttons_state);
  buttons_state = buttons_new;

  if ((buttons_down & 0xff) != 0)
    xQueueSendFromISR(evt_queue, &buttons_down, NULL);

  if ((buttons_down & 0x200) != 0)
  {
    uint32_t event = 1;
    xQueueSendFromISR(i2c_queue, &event, NULL);
  }

  if (buttons_down & (1 << 0))
    ets_printf("Button A\n");
  if (buttons_down & (1 << 1))
    ets_printf("Button B\n");
  if (buttons_down & (1 << 2))
    ets_printf("Button MID\n");
  if (buttons_down & (1 << 3))
    ets_printf("Button UP\n");
  if (buttons_down & (1 << 4))
    ets_printf("Button DOWN\n");
  if (buttons_down & (1 << 5))
    ets_printf("Button LEFT\n");
  if (buttons_down & (1 << 6))
    ets_printf("Button RIGHT\n");
  if (buttons_down & (1 << 7))
    ets_printf("Button FLASH\n");

  if (buttons_down & (1 << 8))
    ets_printf("GDE-Busy down\n");
  if (buttons_up & (1 << 8))
    ets_printf("GDE-Busy up\n");
  if (buttons_down & (1 << 9))
    ets_printf("I2C-INT down\n");
  if (buttons_up & (1 << 9))
    ets_printf("I2C-INT up\n");

#ifdef PIN_NUM_LED
  // pass on BUSY signal to LED.
  gpio_set_level(PIN_NUM_LED, 1 - gpio_get_level(PIN_NUM_BUSY));
#endif // PIN_NUM_LED
}

struct menu_item {
  const char *title;
  void (*handler)(void);
};

#include "demo_text1.h"
#include "demo_text2.h"
#include "demo_greyscale1.h"
#include "demo_greyscale2.h"
#include "demo_greyscale_img1.h"
#include "demo_greyscale_img2.h"
#include "demo_greyscale_img3.h"
#include "demo_partial_update.h"
#include "demo_dot1.h"
#include "demo_test_adc.h"

const struct menu_item demoMenu[] = {
    {"text demo 1", &demoText1},
    {"text demo 2", &demoText2},
    {"greyscale 1", &demoGreyscale1},
    {"greyscale 2", &demoGreyscale2},
    {"greyscale image 1", &demoGreyscaleImg1},
    {"greyscale image 2", &demoGreyscaleImg2},
    {"greyscale image 3", &demoGreyscaleImg3},
    {"partial update test", &demoPartialUpdate},
    {"dot 1", &demoDot1},
    {"ADC test", &demoTestAdc},
    {"tetris?", NULL},
    {"something else", NULL},
    {"test, test, test", NULL},
    {"another item..", NULL},
    {"dot 2", NULL},
    {"dot 3", NULL},
    {NULL, NULL},
};

#define MENU_UPDATE_CYCLES 8
void displayMenu(const char *menu_title, const struct menu_item *itemlist) {
  int num_items = 0;
  while (itemlist[num_items].title != NULL)
    num_items++;

  int scroll_pos = 0;
  int item_pos = 0;
  int num_draw = 0;
  while (1) {
    TickType_t xTicksToWait = portMAX_DELAY;

    /* draw menu */
    if (num_draw < 2) {
      // init buffer
      drawText(14, 0, DISP_SIZE_Y, menu_title,
               FONT_16PX | FONT_INVERT | FONT_FULL_WIDTH | FONT_UNDERLINE_2);
      int i;
      for (i = 0; i < 7; i++) {
        int pos = scroll_pos + i;
        drawText(12 - 2 * i, 0, DISP_SIZE_Y,
                 (pos < num_items) ? itemlist[pos].title : "",
                 FONT_16PX | FONT_FULL_WIDTH |
                     ((pos == item_pos) ? 0 : FONT_INVERT));
      }
    }
    if (num_draw == 0) {
      // init LUT
      static const uint8_t lut[30] = {
          0x99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          0,    0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      };
      gdeWriteCommandStream(0x32, lut, 30);
      // init timings
      gdeWriteCommand_p1(0x3a, 0x02); // 2 dummy lines per gate
      gdeWriteCommand_p1(0x3b, 0x00); // 30us per line
      //			gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy
      //lines per gate
      //			gdeWriteCommand_p1(0x3b, 0x08); // 62us per line
    }
    if (num_draw < MENU_UPDATE_CYCLES) {
      updateDisplay();
      gdeBusyWait();
      num_draw++;
      if (num_draw < MENU_UPDATE_CYCLES)
        xTicksToWait = 0;
    }

    /* handle input */
    uint32_t buttons_down;
    if (xQueueReceive(evt_queue, &buttons_down, xTicksToWait)) {
      if (buttons_down & (1 << 1)) {
        ets_printf("Button B handling\n");
        return;
      }
      if (buttons_down & (1 << 2)) {
        ets_printf("Selected '%s'\n", itemlist[item_pos].title);
        if (itemlist[item_pos].handler != NULL)
          itemlist[item_pos].handler();
        num_draw = 0;
        ets_printf("Button MID handled\n");
        continue;
      }
      if (buttons_down & (1 << 3)) {
        if (item_pos > 0) {
          item_pos--;
          if (scroll_pos > item_pos)
            scroll_pos = item_pos;
          num_draw = 0;
        }
        ets_printf("Button UP handled\n");
      }
      if (buttons_down & (1 << 4)) {
        if (item_pos + 1 < num_items) {
          item_pos++;
          if (scroll_pos + 6 < item_pos)
            scroll_pos = item_pos - 6;
          num_draw = 0;
        }
        ets_printf("Button DOWN handled\n");
      }
    }
  }
}

#ifdef PIN_NUM_I2C_CLOCK
#define I2C_MASTER_NUM I2C_NUM_1
#define I2C_SLAVE_ID 68
#define I2C_MASTER_FREQ_HZ 100000
//define I2C_MASTER_FREQ_HZ 400000
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0

#define WRITE_BIT  I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT   I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL    0x0         /*!< I2C ack value */
#define NACK_VAL   0x1         /*!< I2C nack value */

void
i2c_init(void)
{
	int i2c_master_port = I2C_MASTER_NUM;
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
	i2c_param_config(i2c_master_port, &conf);
	i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

uint8_t
i2c_read_reg(uint8_t reg)
{
	uint8_t buf[1];
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ( I2C_SLAVE_ID << 1 ) | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ( I2C_SLAVE_ID << 1 ) | READ_BIT, ACK_CHECK_EN);
	i2c_master_read_byte(cmd, buf, NACK_VAL);
	i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_OK) {
		ets_printf("i2c master read: ok\n");
		ets_printf("reg(0x%02x) = 0x%02x\n", reg, buf[0]);
	} else {
		ets_printf("i2c master read: error %d\n", ret);
	}

	return buf[0];
}

void
i2c_write_reg(uint8_t reg, uint8_t value)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ( I2C_SLAVE_ID << 1 ) | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, value, ACK_CHECK_EN);
	i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_OK) {
		ets_printf("i2c master write: ok\n");
	} else {
		ets_printf("i2c master write: error %d\n", ret);
	}
}

// scan the whole address-space on timeouts..
uint8_t i2c_touchpad_addr = 0x78;

uint32_t
i2c_read_event(void)
{
	uint8_t buf[3];
	i2c_cmd_handle_t cmd;

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ( i2c_touchpad_addr << 1 ) | READ_BIT, ACK_CHECK_EN);

	i2c_master_read(cmd, buf, 3, ACK_VAL);
	i2c_master_stop(cmd);

	esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_OK) {
		ets_printf("i2c master read (0x%02x): ok\n", i2c_touchpad_addr);
		ets_printf("event: 0x%02x, 0x%02x, 0x%02x\n", buf[0], buf[1], buf[2]);
		if ((buf[0] & 0x0f) == 0x00) {
			ets_printf("\e[32mButton %d pressed.\e[0m\n", buf[1]);
			uint32_t buttons_down = 0;
			if (buf[1] == 2)
				buttons_down = (1<<5);
			if (buf[1] == 3)
				buttons_down = (1<<3);
			if (buf[1] == 4)
				buttons_down = (1<<6);
			if (buf[1] == 5)
				buttons_down = (1<<4);
			if (buf[1] == 7)
				buttons_down = (1<<0); // alternative A
			if (buf[1] == 8)
				buttons_down = (1<<2); // MID
			if (buf[1] == 9)
				buttons_down = (1<<1); // B
			if (buf[1] == 11)
				buttons_down = (1<<0); // A (not working)

			if (buttons_down != 0)
				xQueueSendFromISR(evt_queue, &buttons_down, NULL);
		}
		else if ((buf[0] & 0x0f) == 0x01) {
			ets_printf("\e[33mButton %d released.\e[0m\n", buf[1]);
		}

	} else {
		ets_printf("i2c master read (0x%02x): error %d\n", i2c_touchpad_addr, ret);
	}

	return (buf[0] << 16) | (buf[1] << 8) | (buf[2]);
}
#endif // PIN_NUM_I2C_CLOCK

void
set_leds(void) {
	spi_bus_config_t buscfg = {
		.mosi_io_num   = 32,
		.miso_io_num   = -1,  // -1 = unused
		.sclk_io_num   = -1,  // -1 = unused
		.quadwp_io_num = -1,  // -1 = unused
		.quadhd_io_num = -1,  // -1 = unused
	};

	spi_device_interface_config_t devcfg = {
//		.clock_speed_hz = 3333333, // 3.33 Mhz -- too fast?
		.clock_speed_hz = 3200000, // 3.2 Mhz -- works.. :-)
		.mode           = 0,
		.spics_io_num   = -1,
		.queue_size     = 7,
	};

	esp_err_t ret = spi_bus_initialize(HSPI_HOST, &buscfg, 1);
	assert( ret == ESP_OK );

	spi_device_handle_t spi;

	ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
	assert( ret == ESP_OK );

	uint8_t rgb[6*3] = {
		 85, 170, 255,
		170, 255, 170,
		255, 170,  85,
		170,  85,   0,
		 85,   0,  85,
		  0,  85, 170,
	};
	int8_t dir[6*3] = {
		+1, +1, -1,
		+1, -1, -1,
		-1, -1, -1,
		-1, -1, +1,
		-1, +1, +1,
		+1, +1, +1,
	};
	const uint8_t conv[4] = { 0x11, 0x13, 0x31, 0x33 };

	while (1) {
		vTaskDelay(10 / portTICK_RATE_MS);

		uint8_t data[75];
		int i,j;
		j=0;
		// 3 * 24 us 'reset'
		data[j++] = 0;
		data[j++] = 0;
		data[j++] = 0;
		for (i=5; i>=0; i--) {
			int r = rgb[i*3+0] >> 2;
			int g = rgb[i*3+1] >> 2;
			int b = rgb[i*3+2] >> 2;
			data[j++] = conv[(g>>6)&3];
			data[j++] = conv[(g>>4)&3];
			data[j++] = conv[(g>>2)&3];
			data[j++] = conv[(g>>0)&3];
			data[j++] = conv[(r>>6)&3];
			data[j++] = conv[(r>>4)&3];
			data[j++] = conv[(r>>2)&3];
			data[j++] = conv[(r>>0)&3];
			data[j++] = conv[(b>>6)&3];
			data[j++] = conv[(b>>4)&3];
			data[j++] = conv[(b>>2)&3];
			data[j++] = conv[(b>>0)&3];
		}

		spi_transaction_t t;
		memset(&t, 0, sizeof(t));
		t.length = j*8;
		t.tx_buffer = data;

		ret = spi_device_transmit(spi, &t);
		assert(ret == ESP_OK);

		for (i=0; i<6*3; i++) {
			rgb[i] += dir[i];
			if (rgb[i] == 0)
				dir[i] = +1;
			if (rgb[i] == 255)
				dir[i] = -1;
		}
	}
}

void
task_handle_i2c(void *arg)
{
	while (1) {
		uint32_t event;
		if (xQueueReceive(i2c_queue, &event, portMAX_DELAY)) {
			if (event == 1)
			{
				ets_printf("I2C INT handling\n");
				while (1)
				{
					i2c_read_event();
					i2c_read_reg(0x13); // Interrupt Status
					uint8_t x = i2c_read_reg(0x0f); // Input Level
					if ((x & 0x08) != 0)
						break;
				}
			}
		}
	}
}

void
app_main(void) {
	nvs_flash_init();

	/* create event queues */
	evt_queue = xQueueCreate(10, sizeof(uint32_t));
	i2c_queue = xQueueCreate(10, sizeof(uint32_t));

	/** configure input **/
	gpio_isr_register(gpio_intr_test, NULL, 0, NULL);

	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_ANYEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask =
		(1LL << PIN_NUM_BUSY) |
#ifdef CONFIG_SHA_BADGE_V1
		(1LL << PIN_NUM_BUTTON_A) |
		(1LL << PIN_NUM_BUTTON_B) |
		(1LL << PIN_NUM_BUTTON_MID) |
		(1LL << PIN_NUM_BUTTON_UP) |
		(1LL << PIN_NUM_BUTTON_DOWN) |
		(1LL << PIN_NUM_BUTTON_LEFT) |
		(1LL << PIN_NUM_BUTTON_RIGHT) |
#else
		(1LL << PIN_NUM_BUTTON_FLASH) |
		(1LL << PIN_NUM_I2C_INT) |
#endif // CONFIG_SHA_BADGE_V1
		0LL;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);

	/** configure output **/
#ifdef PIN_NUM_LED
	gpio_pad_select_gpio(PIN_NUM_LED);
	gpio_set_direction(PIN_NUM_LED, GPIO_MODE_OUTPUT);
#endif // PIN_NUM_LED

#ifdef PIN_NUM_I2C_CLOCK
	i2c_init();
//	i2c_write_reg(0x03, 0x00); // IO Direction (0=input; 1=output) - default
	i2c_write_reg(0x03, 0x04); // IO Direction (0=input; 1=output)
//	i2c_write_reg(0x05, 0x00); // Output State (0=low; 1=high) - default
	i2c_write_reg(0x05, 0x04); // Output State (0=low; 1=high)
//	i2c_write_reg(0x07, 0xff); // Output High-Z (0=default; 1=high-z) - default
	i2c_write_reg(0x07, 0xfb); // Output High-Z (0=default; 1=high-z)
//	i2c_write_reg(0x09, 0x00); // Input Default State (int at: 0=l->h; 1=h->l)
	i2c_write_reg(0x09, 0x08); // Input Default State (int at: 0=l->h; 1=h->l)
//	i2c_write_reg(0x0b, 0xff); // Pull Enable (0=disabled; 1=enabled) - default
	i2c_write_reg(0x0b, 0xff); // Pull Enable (0=disabled; 1=enabled)
//	i2c_write_reg(0x0d, 0x00); // Pull-Down/Pull-Up (0=down; 1=up) - default
	i2c_write_reg(0x0d, 0x00); // Pull-Down/Pull-Up (0=down; 1=up) - default
	i2c_write_reg(0x11, 0xf7); // Interrupt Mask (0=enabled; 1=disabled)
	i2c_write_reg(0x13, 0x00); // Interrupt Status

	i2c_read_reg(0x01);
	i2c_read_reg(0x03);
	i2c_read_reg(0x05);
	i2c_read_reg(0x07);
	i2c_read_reg(0x09);
	i2c_read_reg(0x0b);
	i2c_read_reg(0x0d);
	i2c_read_reg(0x0f); // pin status
	i2c_read_reg(0x11);

	// start i2c-event task
	xTaskCreate(&task_handle_i2c, "i2c event task", 4096, NULL, 10, NULL);

	uint32_t event = 1;
    xQueueSendFromISR(i2c_queue, &event, NULL);
#endif

//	tcpip_adapter_init();
//	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

#ifdef CONFIG_WIFI_USE
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	wifi_config_t sta_config = {
		.sta = {.ssid = CONFIG_WIFI_SSID, .password = CONFIG_WIFI_PASSWORD, .bssid_set = false}};
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_connect());
#endif // CONFIG_WIFI_USE

	gdeInit();
	initDisplay(LUT_DEFAULT); // configure slow LUT

demoGreyscaleImg3();

// set_leds();

	int picture_id = 0;
/*
	drawImage(pictures[picture_id]);
	updateDisplay();
	gdeBusyWait();
*/
	int selected_lut = LUT_PART;
	writeLUT(selected_lut); // configure fast LUT

	while (1) {
		uint32_t buttons_down;
		if (xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY)) {
			if (buttons_down & (1 << 0)) {
				ets_printf("Button A handling\n");
				set_leds();
			}
			if (buttons_down & (1 << 1)) {
				ets_printf("Button B handling\n");
				/* redraw with default LUT */
				writeLUT(LUT_DEFAULT);
				drawImage(pictures[picture_id]);
				updateDisplay();
				gdeBusyWait();
				writeLUT(selected_lut);
			}
			if (buttons_down & (1 << 2)) {
				ets_printf("Button MID handling\n");
				/* open menu */
				displayMenu("Demo menu", demoMenu);

				writeLUT(selected_lut);
				drawImage(pictures[picture_id]);
				updateDisplay();
				gdeBusyWait();
			}
			if (buttons_down & (1 << 3)) {
				ets_printf("Button UP handling\n");
				/* switch LUT */
				selected_lut = (selected_lut + 1) % (LUT_MAX + 1);
				writeLUT(selected_lut);
				drawImage(pictures[picture_id]);
				updateDisplay();
				gdeBusyWait();
			}
			if (buttons_down & (1 << 4)) {
				ets_printf("Button DOWN handling\n");
				/* switch LUT */
				selected_lut = (selected_lut + LUT_MAX) % (LUT_MAX + 1);
				writeLUT(selected_lut);
				drawImage(pictures[picture_id]);
				updateDisplay();
				gdeBusyWait();
			}
			if (buttons_down & (1 << 5)) {
				ets_printf("Button LEFT handling\n");
				/* previous picture */
				if (picture_id > 0) {
					picture_id--;
					drawImage(pictures[picture_id]);
					updateDisplay();
					gdeBusyWait();
				}
			}
			if (buttons_down & (1 << 6)) {
				ets_printf("Button RIGHT handling\n");
				/* next picture */
				if (picture_id + 1 < NUM_PICTURES) {
					picture_id++;
					drawImage(pictures[picture_id]);
					updateDisplay();
					gdeBusyWait();
				}
			}
			if (buttons_down & (1 << 7))
			{
				/* use the flash button to try to read something from the touchpad */
				ets_printf("Button FLASH handling\n");
				/* try to read an event */
				uint32_t event = 1;
				xQueueSendFromISR(i2c_queue, &event, NULL);
			}
		}
	}
}
