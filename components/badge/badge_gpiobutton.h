/** @file badge_gpiobutton.h */
#ifndef BADGE_GPIOBUTTON_H
#define BADGE_GPIOBUTTON_H

#include <stdint.h>
#include <esp_err.h>

/** configure gpio pin as button
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_gpiobutton_add(int gpio_num, uint32_t button_id);

#endif // BADGE_GPIOBUTTON_H

