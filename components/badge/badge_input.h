#ifndef BADGE_INPUT_H
#define BADGE_INPUT_H

#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <esp_event.h>

extern xQueueHandle badge_input_queue;
#define BADGE_BUTTON_UP      1
#define BADGE_BUTTON_DOWN    2
#define BADGE_BUTTON_LEFT    3
#define BADGE_BUTTON_RIGHT   4
#define BADGE_BUTTON_MID     5
#define BADGE_BUTTON_A       6
#define BADGE_BUTTON_B       7
#define BADGE_BUTTON_SELECT  8
#define BADGE_BUTTON_START   9
#define BADGE_BUTTON_FLASH  10

extern void badge_input_init(void);

#endif // BADGE_INPUT_H
