/** @file badge_nvs.h */
#ifndef BADGE_NVS_H
#define BADGE_NVS_H

#include <stdint.h>
#include <esp_err.h>

/**
 * Initialize the nvs driver.
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_nvs_init(void);

/**
 *  Set an uint8 value in namespace, key
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_nvs_set_u8(const char* namespace, const char* key, uint8_t value);

/**
 *  Get an uint8 value from namespace, key
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_nvs_get_u8(const char* namespace, const char* key, uint8_t *value);

/**
 *  Set an uint16 value in namespace, key
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_nvs_set_u16(const char* namespace, const char* key, uint16_t value);

/**
 *  Get an uint16 value from namespace, key
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_nvs_get_u16(const char* namespace, const char* key, uint16_t *value);

/**
 *  Set a string value in namespace, key
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_nvs_set_str(const char* namespace, const char* key, const char *value);

/**
 *  Get a string value from namespace, key
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_nvs_get_str(const char* namespace, const char* key, char *value, size_t *length);

#endif // BADGE_NVS_H
