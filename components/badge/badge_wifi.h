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
/* FreeRTOS event group to signal when we are connected & ready to make a
 * request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
extern const int CONNECTED_BIT;

/**
 * wifi event handler
 */
extern esp_err_t badge_wifi_event_handler(void *ctx, system_event_t *event);
/**
 * Initialize the wifi driver.
 */
extern void badge_wifi_init(void);

#endif

#endif
