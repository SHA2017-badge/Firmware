/** @file badge_sdcard.h */
#ifndef BADGE_SDCARD_H
#define BADGE_SDCARD_H

#include <stdbool.h>
#include <esp_err.h>

__BEGIN_DECLS

/** initialize the sdcard inserted sensor
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_sdcard_init(void);

/** report if an sdcard is inserted */
extern bool badge_sdcard_detected(void);

__END_DECLS

#endif // BADGE_SDCARD_H
