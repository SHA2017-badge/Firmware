/** @file badge_touch.h */
#ifndef BADGE_TOUCH_H
#define BADGE_TOUCH_H

/** callback handler for touch events */
typedef void (*badge_touch_event_t)(int event);

/** initialize the badge touch controller */
extern void badge_touch_init(void);

/** set the touch-event callback method */
extern void badge_touch_set_event_handler(badge_touch_event_t handler);

#endif // BADGE_TOUCH_H
