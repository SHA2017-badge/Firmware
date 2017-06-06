#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include <gde.h>
#include <gde-driver.h>
#include <font.h>
#include <string.h>

#include "badge_pins.h"
#include "badge_i2c.h"
#include "badge_portexp.h"
#include "badge_mpr121.h"
#include "badge_touch.h"
#include "badge_leds.h"
#include "badge_eink.h"
#include "badge_power.h"
#include "badge_input.h"

#include "imgv2_sha.h"
#include "imgv2_menu.h"
#include "imgv2_nick.h"
#include "imgv2_weather.h"
#include "imgv2_test.h"

esp_err_t event_handler(void *ctx, system_event_t *event) { return ESP_OK; }

uint32_t buttons_state = 0;

void
gpio_intr_buttons(void *arg) {
	// read status to get interrupt status for GPIO 0-31
	uint32_t gpio_intr_status_lo = READ_PERI_REG(GPIO_STATUS_REG);
	// read status to get interrupt status for GPIO 32-39
	uint32_t gpio_intr_status_hi = READ_PERI_REG(GPIO_STATUS1_REG);
	// clear intr for GPIO 0-31
	SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status_lo);
	// clear intr for GPIO 32-39
	SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_hi);

	uint32_t buttons_new = 0;
#ifdef PIN_NUM_BUTTON_A
	buttons_new |= gpio_get_level(PIN_NUM_BUTTON_A)     <<  BADGE_BUTTON_A;
	buttons_new |= gpio_get_level(PIN_NUM_BUTTON_B)     <<  BADGE_BUTTON_B;
	buttons_new |= gpio_get_level(PIN_NUM_BUTTON_MID)   <<  BADGE_BUTTON_MID;
	buttons_new |= gpio_get_level(PIN_NUM_BUTTON_UP)    <<  BADGE_BUTTON_UP;
	buttons_new |= gpio_get_level(PIN_NUM_BUTTON_DOWN)  <<  BADGE_BUTTON_DOWN;
	buttons_new |= gpio_get_level(PIN_NUM_BUTTON_LEFT)  <<  BADGE_BUTTON_LEFT;
	buttons_new |= gpio_get_level(PIN_NUM_BUTTON_RIGHT) <<  BADGE_BUTTON_RIGHT;
#else
	buttons_new |= gpio_get_level(PIN_NUM_BUTTON_FLASH) <<  BADGE_BUTTON_FLASH;
#endif // ! PIN_NUM_BUTTON_A

	uint32_t buttons_down = (~buttons_new) & buttons_state;
	buttons_state = buttons_new;

	uint32_t button_down;
	for (button_down = 0; button_down < 32; button_down++) {
		if (buttons_down & (1 << button_down))
			xQueueSendFromISR(badge_input_queue, &button_down, NULL);
	}
}

#ifdef I2C_TOUCHPAD_ADDR
void
touch_event_handler(int event)
{
	// convert into button queue event
	if (((event >> 16) & 0x0f) == 0x0) { // button down event
		static const int conv[12] = {
			-1,
			-1,
			BADGE_BUTTON_LEFT,
			BADGE_BUTTON_UP,
			BADGE_BUTTON_RIGHT,
			BADGE_BUTTON_DOWN,
			-1,
			BADGE_BUTTON_SELECT,
			BADGE_BUTTON_START,
			BADGE_BUTTON_B,
			-1,
			BADGE_BUTTON_A,
		};
		if (((event >> 8) & 0xff) < 12) {
			int id = conv[(event >> 8) & 0xff];
			if (id != -1)
			{
				uint32_t button_down = id;
				xQueueSend(badge_input_queue, &button_down, 0);
			}
		}
	}
}
#endif // I2C_TOUCHPAD_ADDR

#ifdef I2C_MPR121_ADDR
void
mpr121_event_handler(void *b)
{
	uint32_t button_down = (int) b;
	xQueueSend(badge_input_queue, &button_down, 0);
}
#endif // I2C_MPR121_ADDR

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
#include "demo_greyscale_img4.h"
#include "demo_partial_update.h"
#include "demo_dot1.h"
#include "demo_test_adc.h"
#include "demo_leds.h"
#include "demo_ugfx.h"
#include "demo_power.h"

const struct menu_item demoMenu[] = {
    {"text demo 1", &demoText1},
    {"text demo 2", &demoText2},
#ifdef CONFIG_SHA_BADGE_EINK_GDEH029A1
    {"greyscale 1", &demoGreyscale1},
    {"greyscale 2", &demoGreyscale2},
    {"greyscale image 1", &demoGreyscaleImg1},
    {"greyscale image 2", &demoGreyscaleImg2},
    {"greyscale image 3", &demoGreyscaleImg3},
#endif // CONFIG_SHA_BADGE_EINK_GDEH029A1
    {"greyscale image 4", &demoGreyscaleImg4},
    {"partial update test", &demoPartialUpdate},
#ifdef CONFIG_SHA_BADGE_EINK_GDEH029A1
    {"dot 1", &demoDot1},
#endif // CONFIG_SHA_BADGE_EINK_GDEH029A1
    {"ADC test", &demoTestAdc},
#ifdef PIN_NUM_LEDS
    {"LEDs demo", &demo_leds},
#endif // PIN_NUM_LEDS
    {"uGFX demo", &demoUgfx},
    {"charging demo", &demoPower},
    {"tetris?", NULL},
    {"something else", NULL},
    {"test, test, test", NULL},
    {"another item..", NULL},
    {"dot 2", NULL},
    {"dot 3", NULL},
    {NULL, NULL},
};

#ifndef CONFIG_SHA_BADGE_EINK_DEPG0290B1
const uint8_t eink_upd_menu_lut[30] = {
	0x99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,    0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
#endif

const struct badge_eink_update eink_upd_menu = {
#ifndef CONFIG_SHA_BADGE_EINK_DEPG0290B1
	.lut      = -1,
	.lut_custom = eink_upd_menu_lut,
	.reg_0x3a = 2, // 2 dummy lines per gate
	.reg_0x3b = 0, // 30us per line
#else
	.lut      = LUT_FASTEST,
	.reg_0x3a = 2, // 2 dummy lines per gate
	.reg_0x3b = 8, // 62us per line
#endif
	.y_start  = 0,
	.y_end    = 295,
};

#define MENU_UPDATE_CYCLES 8
uint8_t screen_buf[296*16];
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
    if (num_draw < MENU_UPDATE_CYCLES) {
	  if (num_draw == 0) {
		// init buffer
		draw_font(screen_buf, 0, 0, BADGE_EINK_WIDTH, menu_title,
			FONT_16PX | FONT_INVERT | FONT_FULL_WIDTH | FONT_UNDERLINE_2);
		int i;
		for (i = 0; i < 7; i++) {
		  int pos = scroll_pos + i;
		  draw_font(screen_buf, 0, 16+16*i, BADGE_EINK_WIDTH,
			  (pos < num_items) ? itemlist[pos].title : "",
			  FONT_16PX | FONT_FULL_WIDTH |
			  ((pos == item_pos) ? 0 : FONT_INVERT));
		}
	  }

	  // all eink displays have 2 'pages'; after writing the second one,
	  // we don't have to write the image itself anymore.
	  if (num_draw < 2)
		badge_eink_display(screen_buf, DISPLAY_FLAG_NO_UPDATE);

	  badge_eink_update(&eink_upd_menu);
      num_draw++;
      if (num_draw < MENU_UPDATE_CYCLES)
        xTicksToWait = 0;
    }

    /* handle input */
    uint32_t button_down;
    if (xQueueReceive(badge_input_queue, &button_down, xTicksToWait)) {
      if (button_down == BADGE_BUTTON_B) {
        ets_printf("Button B handling\n");
        return;
      }
      if (button_down == BADGE_BUTTON_MID) {
        ets_printf("Selected '%s'\n", itemlist[item_pos].title);
        if (itemlist[item_pos].handler != NULL)
          itemlist[item_pos].handler();
        num_draw = 0;
        ets_printf("Button MID handled\n");
        continue;
      }
      if (button_down == BADGE_BUTTON_SELECT) {
        ets_printf("Selected '%s'\n", itemlist[item_pos].title);
        if (itemlist[item_pos].handler != NULL)
          itemlist[item_pos].handler();
        num_draw = 0;
        ets_printf("Button SELECT handled\n");
        continue;
      }
      if (button_down == BADGE_BUTTON_UP) {
        if (item_pos > 0) {
          item_pos--;
          if (scroll_pos > item_pos)
            scroll_pos = item_pos;
          num_draw = 0;
        }
        ets_printf("Button UP handled\n");
      }
      if (button_down == BADGE_BUTTON_DOWN) {
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

// pictures
#define NUM_PICTURES 5
const uint8_t *pictures[NUM_PICTURES] = {
	imgv2_sha,
	imgv2_menu,
	imgv2_nick,
	imgv2_weather,
	imgv2_test,
};

void
display_picture(int picture_id, int selected_lut)
{
	memcpy(screen_buf, pictures[picture_id], 296*128/8);
	char str[30];
	if (selected_lut == -1)
		sprintf(str, "[ pic %d, full update ]", picture_id);
	else
		sprintf(str, "[ pic %d, lut %d ]", picture_id, selected_lut);
	draw_font(screen_buf, 8, 4, BADGE_EINK_WIDTH, str, FONT_INVERT);

	badge_eink_display(screen_buf, (selected_lut+1) << DISPLAY_FLAG_LUT_BIT);
}

void
app_main(void) {
	nvs_flash_init();

	// install isr-service, so we can register interrupt-handlers per
	// gpio pin.
	gpio_install_isr_service(0);

	// configure input queue
	badge_input_init();

	// configure buttons directly connected to gpio pins
#ifdef PIN_NUM_BUTTON_A
	gpio_isr_handler_add(PIN_NUM_BUTTON_A    , gpio_intr_buttons, NULL);
	gpio_isr_handler_add(PIN_NUM_BUTTON_B    , gpio_intr_buttons, NULL);
	gpio_isr_handler_add(PIN_NUM_BUTTON_MID  , gpio_intr_buttons, NULL);
	gpio_isr_handler_add(PIN_NUM_BUTTON_UP   , gpio_intr_buttons, NULL);
	gpio_isr_handler_add(PIN_NUM_BUTTON_DOWN , gpio_intr_buttons, NULL);
	gpio_isr_handler_add(PIN_NUM_BUTTON_LEFT , gpio_intr_buttons, NULL);
	gpio_isr_handler_add(PIN_NUM_BUTTON_RIGHT, gpio_intr_buttons, NULL);
#else
	gpio_isr_handler_add(PIN_NUM_BUTTON_FLASH, gpio_intr_buttons, NULL);
#endif // ! PIN_NUM_BUTTON_A

	// configure the gpio pins for input
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_ANYEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask =
#ifdef PIN_NUM_BUTTON_A
		(1LL << PIN_NUM_BUTTON_A) |
		(1LL << PIN_NUM_BUTTON_B) |
		(1LL << PIN_NUM_BUTTON_MID) |
		(1LL << PIN_NUM_BUTTON_UP) |
		(1LL << PIN_NUM_BUTTON_DOWN) |
		(1LL << PIN_NUM_BUTTON_LEFT) |
		(1LL << PIN_NUM_BUTTON_RIGHT) |
#else
		(1LL << PIN_NUM_BUTTON_FLASH) |
#endif // ! PIN_NUM_BUTTON_A
		0LL;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);

	// configure the i2c bus to the port-expander and touch-controller or to the mpr121
#ifdef PIN_NUM_I2C_CLK
	badge_i2c_init();
#endif // PIN_NUM_I2C_CLK

#ifdef I2C_PORTEXP_ADDR
	badge_portexp_init();
#endif // I2C_PORTEXP_ADDR

#ifdef I2C_TOUCHPAD_ADDR
	badge_touch_init();
	badge_touch_set_event_handler(touch_event_handler);
#endif // I2C_TOUCHPAD_ADDR

#ifdef I2C_MPR121_ADDR
	badge_mpr121_init();
	badge_mpr121_set_interrupt_handler(MPR121_PIN_NUM_A     , mpr121_event_handler, (void*) (BADGE_BUTTON_A));
	badge_mpr121_set_interrupt_handler(MPR121_PIN_NUM_B     , mpr121_event_handler, (void*) (BADGE_BUTTON_B));
	badge_mpr121_set_interrupt_handler(MPR121_PIN_NUM_START , mpr121_event_handler, (void*) (BADGE_BUTTON_START));
	badge_mpr121_set_interrupt_handler(MPR121_PIN_NUM_SELECT, mpr121_event_handler, (void*) (BADGE_BUTTON_SELECT));
	badge_mpr121_set_interrupt_handler(MPR121_PIN_NUM_DOWN  , mpr121_event_handler, (void*) (BADGE_BUTTON_DOWN));
	badge_mpr121_set_interrupt_handler(MPR121_PIN_NUM_RIGHT , mpr121_event_handler, (void*) (BADGE_BUTTON_RIGHT));
	badge_mpr121_set_interrupt_handler(MPR121_PIN_NUM_UP    , mpr121_event_handler, (void*) (BADGE_BUTTON_UP));
	badge_mpr121_set_interrupt_handler(MPR121_PIN_NUM_LEFT  , mpr121_event_handler, (void*) (BADGE_BUTTON_LEFT));
#endif // I2C_MPR121_ADDR

	// configure the voltage measuring for charging-info feedback
	badge_power_init();

	// configure the led-strip on top of the badge
#ifdef PIN_NUM_LEDS
	badge_leds_init();
#endif // PIN_NUM_LEDS

	// configure eink display
	badge_eink_init();

  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

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

  int picture_id = 0;
#if 0
	// simple test-mode
	ets_printf("start drawing image\n");
	display_picture(picture_id, -1);
	ets_printf("done drawing image\n");

	while (1) {
		display_picture(picture_id, -1);
		if (picture_id + 1 < NUM_PICTURES) {
			picture_id++;
		} else {
			picture_id=0;
		}
		ets_delay_us(5000000);
	}
#else
  display_picture(picture_id, -1);
  int selected_lut = LUT_PART;

  while (1) {
    uint32_t button_down;
    if (xQueueReceive(badge_input_queue, &button_down, portMAX_DELAY)) {
      if (button_down == BADGE_BUTTON_B) {
        ets_printf("Button B handling\n");
        /* redraw with default LUT */
		display_picture(picture_id, -1);
      }
      if (button_down == BADGE_BUTTON_MID) {
        ets_printf("Button MID handling\n");
        /* open menu */
        displayMenu("Demo menu", demoMenu);
		display_picture(picture_id, selected_lut);
      }
      if (button_down == BADGE_BUTTON_SELECT) {
        ets_printf("Button SELECT handling\n");
        /* open menu */
        displayMenu("Demo menu", demoMenu);
		display_picture(picture_id, selected_lut);
      }
      if (button_down == BADGE_BUTTON_UP) {
        ets_printf("Button UP handling\n");
        /* switch LUT */
        selected_lut = (selected_lut + 1) % (LUT_MAX + 1);
		display_picture(picture_id, selected_lut);
      }
      if (button_down == BADGE_BUTTON_DOWN) {
        ets_printf("Button DOWN handling\n");
        /* switch LUT */
        selected_lut = (selected_lut + LUT_MAX) % (LUT_MAX + 1);
		display_picture(picture_id, selected_lut);
      }
      if (button_down == BADGE_BUTTON_LEFT) {
        ets_printf("Button LEFT handling\n");
        /* previous picture */
        if (picture_id > 0) {
          picture_id--;
		  display_picture(picture_id, selected_lut);
        }
      }
      if (button_down == BADGE_BUTTON_RIGHT) {
        ets_printf("Button RIGHT handling\n");
        /* next picture */
        if (picture_id + 1 < NUM_PICTURES) {
          picture_id++;
		  display_picture(picture_id, selected_lut);
        }
      }
    }
  }
#endif
}

void vPortCleanUpTCB ( void *pxTCB ) {
	// place clean up code here
}
