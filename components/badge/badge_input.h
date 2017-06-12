#ifndef BADGE_INPUT_H
#define BADGE_INPUT_H

#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <esp_event.h>

extern xQueueHandle badge_input_queue;

extern void badge_input_init(void);

extern void badge_input_add_event(uint32_t button_id, bool pressed, bool in_isr);

extern void (*badge_input_notify)(void);

#endif // BADGE_INPUT_H
