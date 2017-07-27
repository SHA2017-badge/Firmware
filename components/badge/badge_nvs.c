#include <sdkconfig.h>

#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_partition.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>

#include "badge_nvs.h"

static const char *TAG = "badge_nvs";

esp_err_t
badge_nvs_init(void)
{
	static bool badge_nvs_init_done = false;

	if (badge_nvs_init_done)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_LOGW(TAG, "no free pages. erasing nvs partition");

		// NVS partition was truncated and needs to be erased
		const esp_partition_t* nvs_partition = esp_partition_find_first(
				ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);

		if (nvs_partition == NULL)
		{
			ESP_LOGW(TAG, "could not find nvs partition");
			return ESP_ERR_NOT_FOUND;
		}

		err = esp_partition_erase_range(nvs_partition, 0, nvs_partition->size);
		if (err != ESP_OK)
		{
			ESP_LOGW(TAG, "failed to erase nvs partition");
			return err;
		}

		// Retry nvs_flash_init
		err = nvs_flash_init();
	}

	if (err != ESP_OK)
	{
		ESP_LOGW(TAG, "failed to initialize nvs");
		return err;
	}

	badge_nvs_init_done = true;

	ESP_LOGD(TAG, "init done");

	return ESP_OK;
}

esp_err_t
badge_nvs_erase_all(const char* namespace)
{
	nvs_handle my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(namespace, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to open namespace '%s' for writing. (err=%d)", namespace, err);
		return err;
	}

	// Write
	err = nvs_erase_all(my_handle);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to erase all nvs keys'. (err=%d)", err);
		nvs_close(my_handle);
		return err;
	}

	// Commit changes to nvs
	err = nvs_commit(my_handle);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to commit changes to nvs. (err=%d)", err);
		nvs_close(my_handle);
		return err;
	}

	// Close
	nvs_close(my_handle);

	return ESP_OK;
}

esp_err_t
badge_nvs_erase_key(const char* namespace, const char* key)
{
	nvs_handle my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(namespace, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to open namespace '%s' for writing. (err=%d)", namespace, err);
		return err;
	}

	// Write
	err = nvs_erase_key(my_handle, key);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to erase key '%s' in namespace '%s'. (err=%d)", key, namespace, err);
		nvs_close(my_handle);
		return err;
	}

	// Commit changes to nvs
	err = nvs_commit(my_handle);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to commit changes to nvs. (err=%d)", err);
		nvs_close(my_handle);
		return err;
	}

	// Close
	nvs_close(my_handle);

	return ESP_OK;
}

esp_err_t
badge_nvs_get_u8(const char* namespace, const char* key, uint8_t *value)
{
	nvs_handle my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(namespace, NVS_READONLY, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to open namespace '%s' for reading. (err=%d)", namespace, err);
		return err;
	}

	// Read
	err = nvs_get_u8(my_handle, key, value);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to read u8 key '%s' in namespace '%s'. (err=%d)", key, namespace, err);
		nvs_close(my_handle);
		return err;
	}

	// Close
	nvs_close(my_handle);

	return ESP_OK;
}

esp_err_t
badge_nvs_set_u8(const char* namespace, const char* key, uint8_t value)
{
	nvs_handle my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(namespace, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to open namespace '%s' for writing. (err=%d)", namespace, err);
		return err;
	}

	// Erase key
	err = nvs_erase_key(my_handle, key);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
		ESP_LOGD(TAG, "failed to erase key '%s' in namespace '%s'. (err=%d)", key, namespace, err);
		nvs_close(my_handle);
		return err;
	}

	// Write
	err = nvs_set_u8(my_handle, key, value);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to write u8 key '%s' in namespace '%s'. (err=%d)", key, namespace, err);
		nvs_close(my_handle);
		return err;
	}

	// Commit written value.
	// After setting any values, nvs_commit() must be called to ensure changes are written
	// to flash storage. Implementations may write to storage at other times,
	// but this is not guaranteed.
	err = nvs_commit(my_handle);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to commit changes to nvs. (err=%d)", err);
		nvs_close(my_handle);
		return err;
	}

	// Close
	nvs_close(my_handle);

	return ESP_OK;
}

esp_err_t
badge_nvs_get_u16(const char* namespace, const char* key, uint16_t *value)
{
	nvs_handle my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(namespace, NVS_READONLY, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to open namespace '%s' for reading. (err=%d)", namespace, err);
		return err;
	}

	// Read
	err = nvs_get_u16(my_handle, key, value);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to read u16 key '%s' in namespace '%s'. (err=%d)", key, namespace, err);
		nvs_close(my_handle);
		return err;
	}

	// Close
	nvs_close(my_handle);

	return ESP_OK;
}

esp_err_t
badge_nvs_set_u16(const char* namespace, const char* key, uint16_t value)
{
	nvs_handle my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(namespace, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to open namespace '%s' for writing. (err=%d)", namespace, err);
		return err;
	}

	// Erase key
	err = nvs_erase_key(my_handle, key);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
		ESP_LOGD(TAG, "failed to erase key '%s' in namespace '%s'. (err=%d)", key, namespace, err);
		nvs_close(my_handle);
		return err;
	}

	// Write
	err = nvs_set_u16(my_handle, key, value);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to write u16 key '%s' in namespace '%s'. (err=%d)", key, namespace, err);
		nvs_close(my_handle);
		return err;
	}

	// Commit written value.
	// After setting any values, nvs_commit() must be called to ensure changes are written
	// to flash storage. Implementations may write to storage at other times,
	// but this is not guaranteed.
	err = nvs_commit(my_handle);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to commit changes to nvs. (err=%d)", err);
		nvs_close(my_handle);
		return err;
	}

	// Close
	nvs_close(my_handle);

	return ESP_OK;
}

esp_err_t
badge_nvs_get_str(const char* namespace, const char* key, char *value, size_t *length)
{
	nvs_handle my_handle;
	esp_err_t err;

	// Open
	// NVS_READONLY should be enough, but that doesn't seem to work.
	err = nvs_open(namespace, NVS_READONLY, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to open namespace '%s' for reading. (err=%d)", namespace, err);
		return err;
	}

	// Read length
	size_t str_len;
	err = nvs_get_str(my_handle, key, NULL, &str_len);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to read length of str key '%s' in namespace '%s'. (err=%d)", key, namespace, err);
		nvs_close(my_handle);
		return err;
	}

	// Check length
	if (str_len > *length) {
		ESP_LOGD(TAG, "buffer not large enough for str key '%s' in namespace '%s'", key, namespace);
		nvs_close(my_handle);
		return ESP_ERR_NVS_INVALID_LENGTH;
	}

	// store length in return-pointer
	*length = str_len;

	// Read
	err = nvs_get_str(my_handle, key, value, length);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to read str key '%s' in namespace '%s'. (err=%d)", key, namespace, err);
		nvs_close(my_handle);
		return err;
	}

	// Close
	nvs_close(my_handle);

	return ESP_OK;
}

esp_err_t
badge_nvs_set_str(const char* namespace, const char* key, const char *value)
{
	nvs_handle my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(namespace, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to open namespace '%s' for writing. (err=%d)", namespace, err);
		return err;
	}

	// Erase key
	err = nvs_erase_key(my_handle, key);
	if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
		ESP_LOGD(TAG, "failed to erase key '%s' in namespace '%s'. (err=%d)", key, namespace, err);
		nvs_close(my_handle);
		return err;
	}

	// Write
	err = nvs_set_str(my_handle, key, value);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to write str key '%s' in namespace '%s'. (err=%d)", key, namespace, err);
		nvs_close(my_handle);
		return err;
	}

	// Commit
	err = nvs_commit(my_handle);
	if (err != ESP_OK) {
		ESP_LOGD(TAG, "failed to commit changes to nvs. (err=%d)", err);
		nvs_close(my_handle);
		return err;
	}

	// Close
	nvs_close(my_handle);

	return ESP_OK;
}
