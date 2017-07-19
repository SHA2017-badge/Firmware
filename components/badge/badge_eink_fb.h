/** @file badge_eink_fb.h */
#ifndef BADGE_EINK_FB_H
#define BADGE_EINK_FB_H

#include <stdint.h>
#include "badge_eink.h"

/** A one byte per pixel frame-buffer */
extern uint8_t badge_eink_fb[BADGE_EINK_WIDTH * BADGE_EINK_HEIGHT];

#endif // BADGE_EINK_FB_H
