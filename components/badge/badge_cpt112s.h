/** @file badge_cpt112s.h */
#ifndef BADGE_CPT112S_H
#define BADGE_CPT112S_H

#include <esp_err.h>

/** callback handler for cpt112s events */
typedef void (*badge_cpt112s_event_t)(int event);

/** initialize the badge cpt112s controller
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_cpt112s_init(void);

/** set the cpt112s-event callback method */
extern void badge_cpt112s_set_event_handler(badge_cpt112s_event_t handler);

#endif // BADGE_CPT112S_H
