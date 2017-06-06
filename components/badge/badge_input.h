#ifndef BADGE_INPUT_H
#define BADGE_INPUT_H

#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <esp_event.h>

extern xQueueHandle badge_input_queue;

extern void badge_input_init(void);

#endif // BADGE_INPUT_H
