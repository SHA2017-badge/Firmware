/** @file badge_wifi.h */
#ifndef BADGE_WIFI_H
#define BADGE_WIFI_H

#include <netdb.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_log.h"

#ifdef CONFIG_WIFI_USE
/**
 * Initialize the WiFi driver.
 */
extern void badge_wifi_init(void);

/**
 * Wait for WiFi connect.
 */
extern void badge_wifi_wait(void);

#endif

#endif
