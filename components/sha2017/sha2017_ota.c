#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "mbedtls/certs.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/net.h"
#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"

#include "badge.h"
#include "badge_wifi.h"
#include "wildcard_sha2017_org.h"
#include "sha2017_ota_graphics.h"
#include <gfx.h>

#define BADGE_OTA_WEB_SERVER "badge.sha2017.org"
#define BADGE_OTA_WEB_PORT "443"

#ifdef CONFIG_SHA_BADGE_V1
 #define BADGE_OTA_WEB_PATH "/firmware-rev0.0.1-" CONFIG_ESPTOOLPY_FLASHSIZE ".bin"
#elif defined(CONFIG_SHA_BADGE_V2)
 #define BADGE_OTA_WEB_PATH "/firmware-rev0.1.0-" CONFIG_ESPTOOLPY_FLASHSIZE ".bin"
#else
 #define BADGE_OTA_WEB_PATH "/firmware-" CONFIG_ESPTOOLPY_FLASHSIZE ".bin"
#endif

#define BUFFSIZE 1024
#define TEXT_BUFFSIZE 1024

static const char *TAG = "ota";
/* an ota data write buffer ready to write to the flash */
static char ota_write_data[BUFFSIZE + 1] = {0};
/* a packet receive buffer */
static char text[BUFFSIZE + 1] = {0};
/* image total length */
static int binary_file_length = 0;
/* http content length */
static int content_length = 0;
/* socket id */
static int socket_id = -1;

static const char *REQUEST = "GET " BADGE_OTA_WEB_PATH " HTTP/1.0\r\n"
                             "Host: " BADGE_OTA_WEB_SERVER "\r\n"
                             "User-Agent: SHA2017-Badge/1.0 esp32\r\n"
                             "\r\n";


static void __attribute__((noreturn)) task_fatal_error() {
  ESP_LOGE(TAG, "Exiting task due to fatal error...");
  close(socket_id);
  show_percentage("OTA Update failed :(", 0, false);

  (void)vTaskDelete(NULL);

  vTaskDelay(1000 / portTICK_PERIOD_MS);
  esp_restart();
}

/* resolve a packet from http socket
 * return true if packet including \r\n\r\n that means http packet header
 * finished,start to receive packet body otherwise return false
 * */
static bool sha2017_ota_read_past_http_header(char text[], int total_len,
                                              esp_ota_handle_t update_handle) {
  /* i means current position */
  int i = 0;
  char *ptr = text;
  bool first_line = true;

  while (ptr != NULL) {
    ptr = memchr(text + i, '\n', total_len);
    int len = ptr - text - i;

    // ESP_LOGI(TAG, "Len: %d", len);

    // feels a bit oldschool
    char dest[BUFFSIZE];
    strncpy(dest, text + i, len);
    dest[len] = 0; // null terminate destination

    // if we resolve \r\n line,we know packet header is finished
    if (strcmp("\r", dest) == 0) {
      int i_write_len = total_len - i - 2;
      memset(ota_write_data, 0, BUFFSIZE);
      /*copy first http packet body to write buffer*/
      memcpy(ota_write_data, text + i + 2, i_write_len);

      esp_err_t err = esp_ota_write(update_handle, (const void *)ota_write_data,
                                    i_write_len);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error: esp_ota_write failed! err=0x%x", err);
        return false;
      } else {
        ESP_LOGI(TAG, "esp_ota_write header OK");
        binary_file_length += i_write_len;
      }
      return true;
    }

    if (first_line) {
      if (strcmp("HTTP/1.1 200 OK\r", dest) == 0) {
        ESP_LOGI(TAG, "Got ourselves a 200, continue!");
      } else {
        ESP_LOGE(TAG, "Got some other val! %s", dest);
        task_fatal_error();
      }
      first_line = false;
    } else {
      // ESP_LOGI(TAG, "Wut? %s", dest);
      if (strstr(dest, "Content-Length:") != NULL) {
        strncpy(dest, dest + 16, len - 17);
        dest[len - 17] = 0; // null terminate destination
        // ESP_LOGI(TAG, "Length: %s", dest);
        content_length = atoi(dest);
        ESP_LOGI(TAG, "Length: %d", content_length);
      }
    }
    i += (len + 1);
  }
  return false;
}

static void sha2017_ota_task(void *pvParameter) {
  esp_err_t err;
  /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
  esp_ota_handle_t update_handle = 0;
  const esp_partition_t *update_partition = NULL;

  ESP_LOGI(TAG, "Starting OTA update ...");

  const esp_partition_t *configured = esp_ota_get_boot_partition();
  const esp_partition_t *running = esp_ota_get_running_partition();

  assert(configured == running); /* fresh from reset, should be running from
                                    configured boot partition */
                                 /* NOTE: is this always true? what if the bootloader detects that the selected
                                  *       OTA partition is corrupted and decides to boot the old OTA partition.
                                  */
  ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
           configured->type, configured->subtype, configured->address);

  target_lut = 3;

  show_percentage("Connecting to WiFi", 0, false);
  badge_wifi_wait();
  ESP_LOGI(TAG, "Connect to Wifi ! Start to Connect to Server....");

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
  ESP_LOGI(TAG, "Seeding the random number generator");

  mbedtls_ssl_config_init(&conf);

  mbedtls_entropy_init(&entropy);
  if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                   NULL, 0)) != 0) {
    ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
    abort();
  }

  ESP_LOGI(TAG, "Loading the CA root certificate...");

  ret = mbedtls_x509_crt_parse_der(&cacert, wildcard_sha2017_org, 856);

  if (ret < 0) {
    ESP_LOGE(TAG, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
    abort();
  }

  ESP_LOGI(TAG, "Setting hostname for TLS session...");

  /* Hostname set here should match CN in server certificate */
  if ((ret = mbedtls_ssl_set_hostname(&ssl, BADGE_OTA_WEB_SERVER)) != 0) {
    ESP_LOGE(TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
    abort();
  }

  ESP_LOGI(TAG, "Setting up the SSL/TLS structure...");

  if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
                                         MBEDTLS_SSL_TRANSPORT_STREAM,
                                         MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
    ESP_LOGE(TAG, "mbedtls_ssl_config_defaults returned %d", ret);
    task_fatal_error();
  }

  mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
  mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
  mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef CONFIG_MBEDTLS_DEBUG
  mbedtls_esp_enable_debug_log(&conf, 4);
#endif

  if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
    ESP_LOGE(TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
    task_fatal_error();
  }

  badge_wifi_wait();
  ESP_LOGI(TAG, "Connected to AP");

  mbedtls_net_init(&server_fd);

  ESP_LOGI(TAG, "Connecting to %s:%s...", BADGE_OTA_WEB_SERVER,
           BADGE_OTA_WEB_PORT);

  if ((ret = mbedtls_net_connect(&server_fd, BADGE_OTA_WEB_SERVER,
                                 BADGE_OTA_WEB_PORT, MBEDTLS_NET_PROTO_TCP)) !=
      0) {
    ESP_LOGE(TAG, "mbedtls_net_connect returned -%x", -ret);
    task_fatal_error();
  }

  ESP_LOGI(TAG, "Connected.");

  mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv,
                      NULL);

  ESP_LOGI(TAG, "Performing the SSL/TLS handshake...");

  while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
      ESP_LOGE(TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
      task_fatal_error();
    }
  }

  ESP_LOGI(TAG, "Verifying peer X.509 certificate...");

  /* NOTE: Afaik, the mbedtls_ssl_get_verify_result() always returns 0 if
   *       MBEDTLS_SSL_VERIFY_REQUIRED is used.
   */
  if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0) {
    /* In real life, we probably want to close connection if ret != 0 */
    ESP_LOGW(TAG, "Failed to verify peer certificate!");
    bzero(text, sizeof(text));
    mbedtls_x509_crt_verify_info(text, sizeof(text), "  ! ", flags);
    ESP_LOGW(TAG, "verification info: %s", text);
  } else {
    ESP_LOGI(TAG, "Certificate verified.");
  }

  ESP_LOGI(TAG, "Sending HTTP request for %s", BADGE_OTA_WEB_PATH);

  while ((ret = mbedtls_ssl_write(&ssl, (const unsigned char *)REQUEST,
                                  strlen(REQUEST))) <= 0) {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
      ESP_LOGE(TAG, "mbedtls_ssl_write returned -0x%x", -ret);
      task_fatal_error();
    }
  }

  show_percentage("Handshaking server", 0, false);

  update_partition = esp_ota_get_next_update_partition(NULL);
  ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
           update_partition->subtype, update_partition->address);
  assert(update_partition != NULL);

  err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
  /* FIXME: first check the HTTP response and content-length. then decide if we
   * should start updating one of the OTA partitions.
   */
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_begin failed, error=%d", err);
    task_fatal_error();
  }
  ESP_LOGI(TAG, "esp_ota_begin succeeded");

  bool resp_body_start = false, flag = true;
  /*deal with all receive packet*/

  len = ret;
  ESP_LOGI(TAG, "%d bytes written", len);
  ESP_LOGI(TAG, "Reading HTTP response...");

  uint8_t percentage = 110;

  while (flag) {
    memset(text, 0, TEXT_BUFFSIZE);
    memset(ota_write_data, 0, BUFFSIZE);

    len = sizeof(text) - 1;
    bzero(text, sizeof(text));
    ret = mbedtls_ssl_read(&ssl, (unsigned char *)text, len);

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
      ESP_LOGI(TAG, "connection closed");
      break;
    }

    len = ret;
    // ESP_LOGI(TAG, "%d bytes read", len);

    if (len < 0) { /*receive error*/
      ESP_LOGE(TAG, "Error: receive data error! errno=%d", errno);
      task_fatal_error();
    } else if (len > 0 && !resp_body_start) { /*deal with response header*/
      memcpy(ota_write_data, text, len);
      resp_body_start =
          sha2017_ota_read_past_http_header(text, len, update_handle);
    } else if (len > 0 && resp_body_start) { /*deal with response body*/
      memcpy(ota_write_data, text, len);
      err = esp_ota_write(update_handle, (const void *)ota_write_data, len);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error: esp_ota_write failed! err=0x%x", err);
        task_fatal_error();
      }
      binary_file_length += len;

      uint8_t newperc = (uint8_t)round(((float)binary_file_length*100)/content_length);
      if (newperc != percentage) {
        percentage = newperc;
        show_percentage("Updating", percentage, true);
      }
      // ESP_LOGI(TAG, "Have written image length %d", binary_file_length);
    } else if (len == 0) { /*packet over*/
      flag = false;
      ESP_LOGI(TAG, "Connection closed, all packets received");
      close(socket_id);
    } else {
      ESP_LOGE(TAG, "Unexpected recv result");
    }
  }

  mbedtls_ssl_close_notify(&ssl);

  ESP_LOGI(TAG, "Total Write binary data length : %d", binary_file_length);

  if (esp_ota_end(update_handle) != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_end failed!");
    task_fatal_error();
  }

  if (binary_file_length != content_length) {
    ESP_LOGE(TAG, "Firmware size differs, expected: %d\nReceived: %d",
             content_length, binary_file_length);
    task_fatal_error();
  }

  /* FIXME: we should really add code here which verifies the integrity of the
   * new OTA partition.
   */

   show_percentage("Rebooting the badge", 0, false);

  err = esp_ota_set_boot_partition(update_partition);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_set_boot_partition failed! err=0x%x", err);
    task_fatal_error();
  }
  ESP_LOGI(TAG, "Prepare to restart system!");
  esp_restart();
  return;
}

void sha2017_ota_update() {
  // Init the badge
  badge_init();

  sha2017_ota_percentage_init();
  badge_wifi_init();

  xTaskCreate(&sha2017_ota_task, "sha2017_ota_task", 8192, NULL, 5, NULL);
}
