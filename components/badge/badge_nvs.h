/** @file badge_nvs.h */
#ifndef BADGE_NVS_H
#define BADGE_NVS_H

/**
 * Initialize the nvs driver.
 */
extern void badge_nvs_init(void);
/**
 *  Set an uint8 value in namespace, key
 */
extern esp_err_t badge_nvs_set_u8(const char* namespace, const char* key, uint8_t value);
/**
 *  Get an uint8 value from namespace, key
 */
extern esp_err_t badge_nvs_get_u8(const char* namespace, const char* key, uint8_t *value);
/**
 *  Set a string value in namespace, key
 */
extern esp_err_t badge_nvs_set_str(const char* namespace, const char* key, char *value);
/**
 *  Get a string value from namespace, key
 */
extern esp_err_t badge_nvs_get_str(const char* namespace, const char* key, char *value, size_t *length);

#endif
