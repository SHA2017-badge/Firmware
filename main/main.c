#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include <gde.h>
#include <gdeh029a1.h>
#include <matrix_client.h>
#include <pictures.h>
#include <pins.h>
#include "event_queue.h"

/* FreeRTOS event group to signal when we are connected & ready to make a
 * request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

#ifdef CONFIG_SHA_BADGE_V1
#define PIN_NUM_LED 22
#define PIN_NUM_BUTTON_A 0
#define PIN_NUM_BUTTON_B 27
#define PIN_NUM_BUTTON_MID 25
#define PIN_NUM_BUTTON_UP 26
#define PIN_NUM_BUTTON_DOWN 32
#define PIN_NUM_BUTTON_LEFT 33
#define PIN_NUM_BUTTON_RIGHT 35
#else
#define PIN_NUM_BUTTON_MID 25
#endif

// esp_err_t event_handler(void *ctx, system_event_t *event) { return ESP_OK; }

uint32_t get_buttons(void) {
  uint32_t bits = 0;
#ifdef CONFIG_SHA_BADGE_V1
  bits |= gpio_get_level(PIN_NUM_BUTTON_A) << 0;   // A
  bits |= gpio_get_level(PIN_NUM_BUTTON_B) << 1;   // B
#endif                                             // CONFIG_SHA_BADGE_V1
  bits |= gpio_get_level(PIN_NUM_BUTTON_MID) << 2; // MID
#ifdef CONFIG_SHA_BADGE_V1
  bits |= gpio_get_level(PIN_NUM_BUTTON_UP) << 3;    // UP
  bits |= gpio_get_level(PIN_NUM_BUTTON_DOWN) << 4;  // DOWN
  bits |= gpio_get_level(PIN_NUM_BUTTON_LEFT) << 5;  // LEFT
  bits |= gpio_get_level(PIN_NUM_BUTTON_RIGHT) << 6; // RIGHT
#endif                                               // CONFIG_SHA_BADGE_V1
  bits |= gpio_get_level(PIN_NUM_BUSY) << 7;         // GDE BUSY
  return bits;
}


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

  if (buttons_down != 0)
    xQueueSendFromISR(evt_queue, &buttons_down, NULL);

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
    ets_printf("GDE-Busy down\n");
  if (buttons_up & (1 << 7))
    ets_printf("GDE-Busy up\n");

#ifdef PIN_NUM_LED
  // pass on BUSY signal to LED.
  gpio_set_level(PIN_NUM_LED, 1 - gpio_get_level(21));
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
      // lines per gate
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

static esp_err_t event_handler(void *ctx, system_event_t *event) {
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

static void initialise_wifi() {
#ifdef CONFIG_WIFI_USE
  tcpip_adapter_init();
  wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = CONFIG_WIFI_SSID, .password = CONFIG_WIFI_PASSWORD,
          },
  };
  ets_printf("Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
#endif // CONFIG_WIFI_USE
}

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "www.howsmyssl.com"
#define WEB_PORT "443"
#define WEB_URL "https://www.howsmyssl.com/a/check"

static const char *TAG = "example";

static const char *REQUEST = "GET " WEB_URL " HTTP/1.1\n"
                             "Host: " WEB_SERVER "\n"
                             "User-Agent: esp-idf/1.0 esp32\n"
                             "\n";

/* Root cert for howsmyssl.com, taken from server_root_cert.pem
   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect www.howsmyssl.com:443 </dev/null
   The CA root cert is the last cert given in the chain of certs.
   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
extern const uint8_t
    server_root_cert_pem_start[] asm("_binary_lets_encrypt_x3_pem_start");
extern const uint8_t
    server_root_cert_pem_end[] asm("_binary_lets_encrypt_x3_pem_end");

static void https_get_task(void *pvParameters) {
  char buf[512];
  int ret, flags, len;

  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ssl_context ssl;
  mbedtls_x509_crt cacert;
  mbedtls_ssl_config conf;
  mbedtls_net_context server_fd;

  mbedtls_ssl_init(&ssl);
  mbedtls_x509_crt_init(&cacert);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  ets_printf("Seeding the random number generator");

  mbedtls_ssl_config_init(&conf);

  mbedtls_entropy_init(&entropy);
  if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                   NULL, 0)) != 0) {
    ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
    abort();
  }

  ets_printf(TAG, "Loading the CA root certificate...");

  ret = mbedtls_x509_crt_parse(&cacert, server_root_cert_pem_start,
                               server_root_cert_pem_end -
                                   server_root_cert_pem_start);

  if (ret < 0) {
    ESP_LOGE(TAG, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
    abort();
  }

  ets_printf("Setting hostname for TLS session...");

  /* Hostname set here should match CN in server certificate */
  if ((ret = mbedtls_ssl_set_hostname(&ssl, WEB_SERVER)) != 0) {
    ESP_LOGE(TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
    abort();
  }

  ets_printf("Setting up the SSL/TLS structure...");

  if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
                                         MBEDTLS_SSL_TRANSPORT_STREAM,
                                         MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
    ESP_LOGE(TAG, "mbedtls_ssl_config_defaults returned %d", ret);
    goto exit;
  }

  /* MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will
     print
     a warning if CA verification fails but it will continue to connect.
     You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
  */
  mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
  mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
  mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

  if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
    ESP_LOGE(TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
    goto exit;
  }

  while (1) {
    /* Wait for the callback to set the CONNECTED_BIT in the
       event group.
    */
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true,
                        portMAX_DELAY);
    ets_printf("Connected to AP");

    mbedtls_net_init(&server_fd);

    ets_printf("Connecting to %s:%s...", WEB_SERVER, WEB_PORT);

    if ((ret = mbedtls_net_connect(&server_fd, WEB_SERVER, WEB_PORT,
                                   MBEDTLS_NET_PROTO_TCP)) != 0) {
      ESP_LOGE(TAG, "mbedtls_net_connect returned -%x", -ret);
      goto exit;
    }

    ets_printf("Connected.");

    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv,
                        NULL);

    ets_printf("Performing the SSL/TLS handshake...");

    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
      if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
          ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
        ESP_LOGE(TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
        goto exit;
      }
    }

    ets_printf("Verifying peer X.509 certificate...");

    if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0) {
      /* In real life, we probably want to close connection if ret != 0 */
      ESP_LOGW(TAG, "Failed to verify peer certificate!");
      bzero(buf, sizeof(buf));
      mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
      ESP_LOGW(TAG, "verification info: %s", buf);
    } else {
      ets_printf("Certificate verified.");
    }

    ets_printf("Writing HTTP request...");

    while ((ret = mbedtls_ssl_write(&ssl, (const unsigned char *)REQUEST,
                                    strlen(REQUEST))) <= 0) {
      if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
          ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
        ESP_LOGE(TAG, "mbedtls_ssl_write returned -0x%x", -ret);
        goto exit;
      }
    }

    len = ret;
    ets_printf("%d bytes written", len);
    ets_printf("Reading HTTP response...");

    do {
      len = sizeof(buf) - 1;
      bzero(buf, sizeof(buf));
      ret = mbedtls_ssl_read(&ssl, (unsigned char *)buf, len);

      if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
        continue;

      if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
        ret = 0;
        break;
      }

      if (ret < 0) {
        ESP_LOGE(TAG, "mbedtls_ssl_read returned -0x%x", -ret);
        break;
      }

      if (ret == 0) {
        ets_printf("connection closed");
        break;
      }

      len = ret;
      ets_printf("%d bytes read", len);
      /* Print response directly to stdout as it is read */
      for (int i = 0; i < len; i++) {
        putchar(buf[i]);
      }
    } while (1);

    mbedtls_ssl_close_notify(&ssl);

  exit:
    mbedtls_ssl_session_reset(&ssl);
    mbedtls_net_free(&server_fd);

    if (ret != 0) {
      mbedtls_strerror(ret, buf, 100);
      ESP_LOGE(TAG, "Last error was: -0x%x - %s", -ret, buf);
    }

    for (int countdown = 10; countdown >= 0; countdown--) {
      ets_printf("%d...", countdown);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ets_printf("Starting again!");
  }
}

void app_main(void) {
  nvs_flash_init();

  /* create event queue */
  evt_queue = xQueueCreate(10, sizeof(uint32_t));

  /** configure input **/
  gpio_isr_register(gpio_intr_test, NULL, 0, NULL);

  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask =
      (1LL << PIN_NUM_BUSY) |
#ifdef CONFIG_SHA_BADGE_V1
      (1LL << PIN_NUM_BUTTON_A) | (1LL << PIN_NUM_BUTTON_B) |
#endif // CONFIG_SHA_BADGE_V1
      (1LL << PIN_NUM_BUTTON_MID) |
#ifdef CONFIG_SHA_BADGE_V1
      (1LL << PIN_NUM_BUTTON_UP) | (1LL << PIN_NUM_BUTTON_DOWN) |
      (1LL << PIN_NUM_BUTTON_LEFT) | (1LL << PIN_NUM_BUTTON_RIGHT) |
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

  initialise_wifi();

  gdeInit();
  initDisplay(LUT_DEFAULT); // configure slow LUT

  int picture_id = 0;
  drawImage(pictures[picture_id]);
  updateDisplay();
  gdeBusyWait();

  // get demo
  xTaskCreate(&https_get_task, "https_get_task", 8192, NULL, 5, NULL);

  int selected_lut = LUT_PART;
  writeLUT(selected_lut); // configure fast LUT

  while (1) {
    uint32_t buttons_down;
    if (xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY)) {
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
    }
  }
}
