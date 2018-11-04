/** @file badge_touch.h */
#ifndef BADGE_TOUCH_H
#define BADGE_TOUCH_H

#include <stdint.h>
#include <esp_err.h>
// export BADGE_TOUCH_NUM
#include "driver/touch_pad.h"

__BEGIN_DECLS

extern void badge_touch_poll(void);
extern void badge_touch_init(const uint32_t *button_ids);

__END_DECLS

#endif // BADGE_TOUCH_H
