/** @file badge_eink_fb.h */
#ifndef BADGE_EINK_FB_H
#define BADGE_EINK_FB_H

#include <stdint.h>
#include <esp_err.h>

#include "badge_eink.h"

__BEGIN_DECLS

#define BADGE_EINK_FB_LEN (BADGE_EINK_WIDTH * BADGE_EINK_HEIGHT)

/** A one byte per pixel frame-buffer */
extern uint8_t *badge_eink_fb;

/**
 * Initialize badge eink framebuffer.
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_eink_fb_init(void);

__END_DECLS

#endif // BADGE_EINK_FB_H
