#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_wifi.h"

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
#include "badge_eink.h"
#include "badge_eink_dev.h"
#include "badge_nvs.h"
#include "wildcard_sha2017_org.h"
#include "sha2017_ota_graphics.h"
#include <gfx.h>

#define TAG "sha2017-ota"

#define BADGE_OTA_WEB_SERVER CONFIG_OTA_SERVER_NAME
#define BADGE_OTA_WEB_PORT "443"

#ifdef CONFIG_SHA_BADGE_V1
 #define BADGE_OTA_WEB_PATH CONFIG_OTA_SERVER_PATH "-rev0.0.1-" CONFIG_ESPTOOLPY_FLASHSIZE ".bin"
#elif defined(CONFIG_SHA_BADGE_V2)
 #define BADGE_OTA_WEB_PATH CONFIG_OTA_SERVER_PATH "-rev0.1.0-" CONFIG_ESPTOOLPY_FLASHSIZE ".bin"
#else
 #define BADGE_OTA_WEB_PATH CONFIG_OTA_SERVER_PATH "-" CONFIG_ESPTOOLPY_FLASHSIZE ".bin"
#endif

static uint8_t buffer[1024];
static int buffer_len = 0;

static const char *REQUEST = "GET " BADGE_OTA_WEB_PATH " HTTP/1.0\r\n"
                             "Host: " BADGE_OTA_WEB_SERVER "\r\n"
                             "User-Agent: SHA2017-Badge/1.0 esp32\r\n"
                             "\r\n";

/* FreeRTOS event group to signal when we are connected & ready to make a
 * request */
static EventGroupHandle_t wifi_event_group = 0;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
#define CONNECTED_BIT  BIT0

/* the esp event-loop handler */
static esp_err_t
sha2017_ota_event_handler(void *ctx, system_event_t *event)
{
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

static void
sha2017_ota_initialise_wifi(void)
{
	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(sha2017_ota_event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

	wifi_config_t wifi_config = { };

	esp_err_t err;

	size_t len = sizeof(wifi_config.sta.ssid);
	err = badge_nvs_get_str("badge", "wifi.ssid", (char *) wifi_config.sta.ssid, &len);
	if (err != ESP_OK || len == 0) {
		strncpy((char *) wifi_config.sta.ssid, CONFIG_WIFI_SSID, sizeof(wifi_config.sta.ssid));
	}

	len = sizeof(wifi_config.sta.password);
	err = badge_nvs_get_str("badge", "wifi.password", (char *) wifi_config.sta.password, &len);
	if (err != ESP_OK || len == 0) {
		strncpy((char *) wifi_config.sta.password, CONFIG_WIFI_PASSWORD, sizeof(wifi_config.sta.password));
	}

	ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

static void __attribute__((noreturn))
task_fatal_error(void)
{
	ESP_LOGE(TAG, "Exiting task due to fatal error...");
	show_percentage("OTA Update failed :(", 0, false);

	(void)vTaskDelete(NULL);

	vTaskDelay(1000 / portTICK_PERIOD_MS);
	esp_restart();
}

static inline uint8_t *
index_crlf(uint8_t *buf, size_t buf_len)
{
	while (buf_len >= 2) {
		uint8_t *cr = memchr(buf, '\r', buf_len - 1);
		if (cr == NULL) {
			return NULL;
		}
		if (cr[1] == '\n') {
			return cr;
		}
		intptr_t pos = (intptr_t) cr - (intptr_t) buf;
		buf = &buf[pos + 1];
		buf_len -= pos + 1;
	}
	return NULL;
}

int
mbedtls_ssl_handshake_(mbedtls_ssl_context *ssl)
{
	while (1)
	{
		int ret = mbedtls_ssl_handshake(ssl);

		if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
			// FIXME: implement wait?
			ESP_LOGI(TAG, "mbedtls_ssl_handshake returned MBEDTLS_ERR_SSL_WANT_READ");

		} else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			// FIXME: implement wait?
			ESP_LOGI(TAG, "mbedtls_ssl_handshake returned MBEDTLS_ERR_SSL_WANT_WRITE");

		} else {
			return ret;
		}
	}
}

int
mbedtls_ssl_read_(mbedtls_ssl_context *ssl, unsigned char *buf, size_t len)
{
	while (1)
	{
		int ret = mbedtls_ssl_read(ssl, buf, len);

		if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
			// FIXME: implement wait?
			ESP_LOGI(TAG, "mbedtls_ssl_read returned MBEDTLS_ERR_SSL_WANT_READ");

		} else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			// FIXME: implement wait?
			ESP_LOGI(TAG, "mbedtls_ssl_read returned MBEDTLS_ERR_SSL_WANT_WRITE");

		} else {
			return ret;
		}
	}
}

int
mbedtls_ssl_write_(mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len)
{
	int total_len = 0;
	while (len > 0)
	{
		int ret = mbedtls_ssl_write(ssl, buf, len);
		if (ret > 0) {
			total_len += ret;
			buf = &buf[ret];
			len -= ret;

		} else if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
			// FIXME: implement wait?
			ESP_LOGI(TAG, "mbedtls_ssl_write returned MBEDTLS_ERR_SSL_WANT_READ");

		} else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			// FIXME: implement wait?
			ESP_LOGI(TAG, "mbedtls_ssl_write returned MBEDTLS_ERR_SSL_WANT_WRITE");

		} else {
			return ret;
		}
	}

	return total_len;
}

static void
sha2017_ota_task(void *pvParameter)
{
	esp_err_t err;

	ESP_LOGI(TAG, "Starting OTA update ...");

	/* determine partitions */
	const esp_partition_t *part_running = esp_ota_get_running_partition();
	assert(part_running != NULL);
	ESP_LOGI(TAG, "Running from partition type %d subtype %d (offset 0x%08x)",
			part_running->type, part_running->subtype, part_running->address);

	const esp_partition_t *part_update = esp_ota_get_next_update_partition(NULL);
	assert(part_update != NULL);
	ESP_LOGI(TAG, "Writing to partition type %d subtype %d (offset 0x%08x)",
			part_update->type, part_update->subtype, part_update->address);

	target_lut = BADGE_EINK_LUT_FASTER;

	show_percentage("Connecting to WiFi", 0, false);
	/* Wait for the callback to set the CONNECTED_BIT in the
	   event group.
	 */
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true,
			portMAX_DELAY);
	ESP_LOGI(TAG, "Connect to Wifi ! Start to Connect to Server....");

	int ret;

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
	ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
	if (ret != 0) {
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
	ret = mbedtls_ssl_set_hostname(&ssl, BADGE_OTA_WEB_SERVER);
	if (ret != 0) {
		ESP_LOGE(TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
		abort();
	}

	ESP_LOGI(TAG, "Setting up the SSL/TLS structure...");

	ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
			MBEDTLS_SSL_TRANSPORT_STREAM,
			MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		ESP_LOGE(TAG, "mbedtls_ssl_config_defaults returned %d", ret);
		task_fatal_error();
	}

	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef CONFIG_MBEDTLS_DEBUG
	mbedtls_esp_enable_debug_log(&conf, 4);
#endif

	ret = mbedtls_ssl_setup(&ssl, &conf);
	if (ret != 0) {
		ESP_LOGE(TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
		task_fatal_error();
	}

	/* Wait for the callback to set the CONNECTED_BIT in the
	   event group.
	 */
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true,
			portMAX_DELAY);
	ESP_LOGI(TAG, "Connected to AP");

	mbedtls_net_init(&server_fd);

	show_percentage("Handshaking server", 0, false);

	ESP_LOGI(TAG, "Connecting to %s:%s...", BADGE_OTA_WEB_SERVER,
			BADGE_OTA_WEB_PORT);

	ret = mbedtls_net_connect(&server_fd, BADGE_OTA_WEB_SERVER, BADGE_OTA_WEB_PORT, MBEDTLS_NET_PROTO_TCP);
	if (ret != 0) {
		ESP_LOGE(TAG, "mbedtls_net_connect returned -%x", -ret);
		task_fatal_error();
	}

	ESP_LOGI(TAG, "Connected.");

	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv,
			NULL);

	ESP_LOGI(TAG, "Performing the SSL/TLS handshake...");

	ret = mbedtls_ssl_handshake_(&ssl);
	if (ret != 0) {
		ESP_LOGE(TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
		task_fatal_error();
	}

	ESP_LOGI(TAG, "Verifying peer X.509 certificate...");

	/* NOTE: Afaik, the mbedtls_ssl_get_verify_result() always returns 0 if
	 *       MBEDTLS_SSL_VERIFY_REQUIRED is used.
	 */
	ret = mbedtls_ssl_get_verify_result(&ssl);
	if (ret != 0) {
		/* In real life, we probably want to close connection if ret != 0 */
		ESP_LOGW(TAG, "Failed to verify peer certificate!");
		task_fatal_error();
	}

	ESP_LOGI(TAG, "Certificate verified.");

	ESP_LOGI(TAG, "Sending HTTP request for %s", BADGE_OTA_WEB_PATH);

	ret = mbedtls_ssl_write_(&ssl, (const unsigned char *) REQUEST, strlen(REQUEST));
	if (ret <= 0) {
		ESP_LOGE(TAG, "mbedtls_ssl_write returned -0x%x", -ret);
		task_fatal_error();
	}
	ESP_LOGI(TAG, "%d bytes written", strlen(REQUEST));

	show_percentage("Starting OTA update", 0, false);

	/* read until we have received the status line */
	ESP_LOGI(TAG, "Reading HTTP response status line.");
	uint8_t *crlf;
	while ((crlf = index_crlf(buffer, buffer_len)) == NULL) {
		if (sizeof(buffer) == buffer_len) {
			ESP_LOGE(TAG, "received too long status line.");
			task_fatal_error();
		}

		ret = mbedtls_ssl_read_(&ssl, &buffer[buffer_len], sizeof(buffer) - buffer_len);
		if (ret <= 0) {
			ESP_LOGE(TAG, "mbedtls_ssl_read returned -0x%x", -ret);
			task_fatal_error();
		}

		buffer_len += ret;
	}
	*crlf = 0;

	// parse status line in buffer[]; it's zero-terminated.
	// (line is truncated if server responded with null-characters)
	{
		if (strncmp((const char *) buffer, "HTTP/1.", 7) != 0) {
			ESP_LOGE(TAG, "not an HTTP response.");
			task_fatal_error();
		}
		const char *code = index((const char *) buffer, ' ');
		if (code == NULL || strncmp((const char *) code, " 200 ", 5) != 0) {
			ESP_LOGE(TAG, "did not receive 200 code.");
			task_fatal_error();
		}
		ESP_LOGW(TAG, "Status '%s', OK", (const char *) buffer);
	}

	// move left-over data
	intptr_t line_len = (intptr_t) crlf + 2 - (intptr_t) buffer;
	memmove(buffer, &buffer[line_len], buffer_len - line_len);
	buffer_len -= line_len;

	/* read until we have received all headers */
	ESP_LOGI(TAG, "Reading HTTP response headers.");
	ssize_t content_length = -1;
	while (buffer != crlf) {
		while ((crlf = index_crlf(buffer, buffer_len)) == NULL) {
			if (sizeof(buffer) == buffer_len) {
				ESP_LOGE(TAG, "received too long header line.");
				task_fatal_error();
			}

			ret = mbedtls_ssl_read_(&ssl, &buffer[buffer_len], sizeof(buffer) - buffer_len);
			if (ret <= 0) {
				ESP_LOGE(TAG, "mbedtls_ssl_read returned -0x%x", -ret);
				task_fatal_error();
			}

			buffer_len += ret;
		}
		*crlf = 0;

		// parse header line in buffer[]; it's zero-terminated.
		// (line is truncated if server responded with null-characters)
		{
			if (strncasecmp((const char *) buffer, "Content-Length:", 15) == 0) {
				const char *len_str = (const char *) &buffer[15];
				while (*len_str == ' ') { len_str++; }
				content_length = atoi(len_str);
				if (content_length < 0) {
					ESP_LOGE(TAG, "received invalid length.");
					task_fatal_error();
				}
				ESP_LOGW(TAG, "Content-Length: %d", content_length);
			}
		}

		// move left-over data
		intptr_t line_len = (intptr_t) crlf + 2 - (intptr_t) buffer;
		memmove(buffer, &buffer[line_len], buffer_len - line_len);
		buffer_len -= line_len;
	}

	if (content_length < 0) {
		ESP_LOGE(TAG, "no content-length header received.");
		task_fatal_error();
	}

	if (content_length > part_update->size) {
		ESP_LOGE(TAG, "the firmware doesn't fit into the ota partition.");
		task_fatal_error();
	}

	/* read response data */
	// update handle : set by esp_ota_begin(), must be freed via esp_ota_end()
	esp_ota_handle_t update_handle = 0;

	err = esp_ota_begin(part_update, OTA_SIZE_UNKNOWN, &update_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "esp_ota_begin failed, error=%d", err);
		task_fatal_error();
	}
	ESP_LOGI(TAG, "esp_ota_begin succeeded");

	ESP_LOGI(TAG, "Reading HTTP response data.");

	uint8_t percentage = 110;

	ssize_t content_pos = 0;
	while (content_pos < content_length) {
		while (content_pos + buffer_len < content_length && buffer_len < sizeof(buffer)) {
			size_t need_bytes = content_length - content_pos - buffer_len;
			if (need_bytes > sizeof(buffer) - buffer_len) {
				need_bytes = sizeof(buffer) - buffer_len;
			}

			ret = mbedtls_ssl_read_(&ssl, &buffer[buffer_len], need_bytes);
			if (ret <= 0) {
				ESP_LOGE(TAG, "mbedtls_ssl_read returned -0x%x", -ret);
				task_fatal_error();
			}

			buffer_len += ret;
		}

		// buffer_len == sizeof(buffer) or this is the last part
		err = esp_ota_write(update_handle, buffer, buffer_len);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "esp_ota_write failed, error=%d", err);
			task_fatal_error();
		}

		content_pos += buffer_len;
		buffer_len = 0;

		uint8_t newperc = (uint8_t) round(((float) content_pos * 100) / content_length);
		if (newperc != percentage && !badge_eink_dev_is_busy()) {
			percentage = newperc;
			show_percentage("Updating", percentage, true);
		}
	}

	mbedtls_ssl_close_notify(&ssl);

	ESP_LOGI(TAG, "Total Write binary data length : %d", content_pos);

	err = esp_ota_end(update_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "esp_ota_end failed!");
		task_fatal_error();
	}

	/* FIXME: we should really add code here which verifies the integrity of the
	 * new OTA partition.
	 */

	show_percentage("Rebooting the badge", 0, false);

	err = esp_ota_set_boot_partition(part_update);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "esp_ota_set_boot_partition failed! err=0x%x", err);
		task_fatal_error();
	}
	ESP_LOGI(TAG, "Prepare to restart system!");
	esp_restart();
	return;
}

void sha2017_ota_update() {
	esp_err_t err = nvs_flash_init();
	// Init the badge
	badge_init();
	sha2017_ota_percentage_init();

	if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
		// OTA app partition table has a smaller NVS partition size than the non-OTA
		// partition table. This size mismatch may cause NVS initialization to fail.
		// If this happens, we erase NVS partition and initialize NVS again.
		const esp_partition_t *nvs_partition = esp_partition_find_first(
				ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
		assert(nvs_partition && "partition table must have an NVS partition");
		ESP_ERROR_CHECK(
				esp_partition_erase_range(nvs_partition, 0, nvs_partition->size));
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

	sha2017_ota_initialise_wifi();
	xTaskCreatePinnedToCore(&sha2017_ota_task, "sha2017_ota_task", 8192, NULL, 3, NULL, 0);
}
