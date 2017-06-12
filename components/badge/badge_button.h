#ifndef BADGE_BUTTON_H
#define BADGE_BUTTON_H

extern void badge_button_add(int gpio_num, uint32_t button_id);

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

// Number of buttons on the badge
#define BADGE_BUTTONS 10

#define EVENT_BUTTON_RELEASED false
#define EVENT_BUTTON_PRESSED  true
#define NOT_IN_ISR false
#define IN_ISR true

#endif // BADGE_BUTTON_H

