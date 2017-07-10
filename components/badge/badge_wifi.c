#include <stdio.h>
#include <string.h>
#include "badge_wifi.h"
#include "badge_nvs.h"

#ifdef CONFIG_WIFI_USE

const int CONNECTED_BIT = BIT0;

static const char *TAG = "wifi";

esp_err_t badge_wifi_event_handler(void *ctx, system_event_t *event) {
  switch (event->event_id) {
  case SYSTEM_EVENT_STA_START:
    esp_wifi_connect();
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    /* This is a workaround as ESP32 WiFi libs don't currently
       auto-reassociate. */
    esp_wifi_connect();
    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    break;
  default:
    break;
  }
  return ESP_OK;
}

void badge_wifi_init(void) {
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(badge_wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    char ssid[32];
    char password[64];

    esp_err_t err;
    size_t len;
    err = badge_nvs_get_str("badge", "wifi.ssid", ssid, &len);
    if (err != ESP_OK || len == 0) {
      strcpy(ssid, CONFIG_WIFI_SSID);
      badge_nvs_set_str("badge", "wifi.ssid", ssid);
    }
    err = badge_nvs_get_str("badge", "wifi.password", password, &len);
    if (err != ESP_OK || len == 0) {
      strcpy(password, CONFIG_WIFI_PASSWORD);
      badge_nvs_set_str("badge", "wifi.password", password);
    }

    wifi_config_t wifi_config = { };
    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, password);

    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

#endif
