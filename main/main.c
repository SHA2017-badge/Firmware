#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include <gde.h>
#include <gdeh029a1.h>
#include <pictures.h>

esp_err_t event_handler(void *ctx, system_event_t *event) { return ESP_OK; }

bool menuX = false;
bool menuY = false;
bool menu  = false;
int menuItem = 0;

int is_known_gpio(int pin) {
  if (pin == 0)
    return 1; // A
  if (pin == 25)
    return 1; // MID
  if (pin == 26)
    return 1; // UP
  if (pin == 27)
    return 1; // B
  if (pin == 32)
    return 1; // DOWN
  if (pin == 33)
    return 1; // LEFT
  if (pin == 35)
    return 1; // RIGHT

  return 0;
}

uint32_t get_buttons(void) {
  uint32_t bits = 0;
  bits |= gpio_get_level(0) << 0;  // A
  bits |= gpio_get_level(27) << 1; // B
  bits |= gpio_get_level(25) << 2; // MID
  bits |= gpio_get_level(26) << 3; // UP
  bits |= gpio_get_level(32) << 4; // DOWN
  bits |= gpio_get_level(33) << 5; // LEFT
  bits |= gpio_get_level(35) << 6; // RIGHT
  return bits;
}

uint32_t buttons_state = -1;


void startMenu(bool active){
	if (!menu) {
		menuImage(pictures[1],0);
		ets_delay_us(2000000);
		initDisplay(true);
		menuImage(pictures[1], menuItem);
		menu = true;
	} else {
		if (menuX && menuY) {
      menuItem = 4;
    } else if (menuX && !menuY) {
      menuItem = 2;
    } else if (!menuX && !menuY) {
      menuItem = 1;
    } else if (!menuX && menuY) {
      menuItem = 3;
    }
		menuImage(pictures[1], menuItem);
		if (active) {
			active = false;

		}
	}
}

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
  uint32_t buttons_add = (~buttons_new) & buttons_state;
  buttons_state = buttons_new;

  if (buttons_add & (1 << 0)) {
    ets_printf("Button A\n");
		startMenu(true);
  }
  if (buttons_add & (1 << 1)) {
    ets_printf("Button B\n");
		startMenu(true);
  }

  if (buttons_add & (1 << 2)) {
    ets_printf("Button MID\n");
  }
  if (buttons_add & (1 << 3)) {
    ets_printf("Button UP\n");
		menuY = !menuY;
		startMenu(false);
  }
  if (buttons_add & (1 << 4)) {
    ets_printf("Button DOWN\n");
		menuY = !menuY;
		startMenu(false);
  }
  if (buttons_add & (1 << 5)) {
    ets_printf("Button LEFT\n");
		menuX = !menuX;
		startMenu(false);
  }
  if (buttons_add & (1 << 6)) {
    ets_printf("Button RIGHT\n");
		menuX = !menuX;
		startMenu(false);
  }
}


void app_main(void) {
  nvs_flash_init();
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  wifi_config_t sta_config = {.sta = {.ssid = "access_point_name",
                                      .password = "password",
                                      .bssid_set = false}};
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());

  gpio_isr_register(gpio_intr_test, NULL, 0, NULL);

  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = GPIO_SEL_0 | GPIO_SEL_25 | GPIO_SEL_26 | GPIO_SEL_27 |
                         GPIO_SEL_32 | GPIO_SEL_33 | GPIO_SEL_35;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 1;
  gpio_config(&io_conf);

  initSPI();
  initDisplay(false);
  displayImage(pictures[0], false);
}
