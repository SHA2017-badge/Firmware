#ifndef BADGE_BUTTON_H
#define BADGE_BUTTON_H

extern void badge_button_add(int gpio_num, uint32_t button_id);

extern void badge_button_handler(void *arg);

#endif // BADGE_BUTTON_H

