#ifndef MATRIX_CLIENT
#define MATRIX_CLIENT

#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "mbedtls/certs.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net.h"
#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"

/**
 * Based on code from https://matrix.org/docs/projects/other/matrix-esp8266.html
 * By matt-williams
 */

void createLoginBody(char *buffer, int bufferLen, const char *user,
                     const char *password);

void createMessageBody(char *buffer, int bufferLen, const char *message);

bool login(const char *user, const char *password);

bool getMessages(const char *roomId);

bool sendMessage(const char *roomId, const char *message);

#endif
