#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "esp_event.h"

extern xQueueHandle evt_queue;

#endif // EVENT_QUEUE_H
