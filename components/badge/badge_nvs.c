#include <sdkconfig.h>

#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_partition.h>
#include <nvs_flash.h>
#include <nvs.h>

#include "badge_nvs.h"

esp_err_t
badge_nvs_init(void)
{
  static bool badge_nvs_init_done = false;
  if (badge_nvs_init_done)
    return ESP_OK;

  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
      // NVS partition was truncated and needs to be erased
      const esp_partition_t* nvs_partition = esp_partition_find_first(
              ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
      assert(nvs_partition && "partition table must have an NVS partition");
      ESP_ERROR_CHECK( esp_partition_erase_range(nvs_partition, 0, nvs_partition->size) );
      // Retry nvs_flash_init
      err = nvs_flash_init();
  }
  ESP_ERROR_CHECK( err );
  badge_nvs_init_done = true;
  return ESP_OK;
}

esp_err_t badge_nvs_set_u8(const char* namespace, const char* key, uint8_t value)
{
  nvs_handle my_handle;
  esp_err_t err;

  // Open
  err = nvs_open(namespace, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) return err;

  // Write
  err = nvs_set_u8(my_handle, key, value);
  if (err != ESP_OK) return err;

  // Commit written value.
  // After setting any values, nvs_commit() must be called to ensure changes are written
  // to flash storage. Implementations may write to storage at other times,
  // but this is not guaranteed.
  err = nvs_commit(my_handle);
  if (err != ESP_OK) return err;

  // Close
  nvs_close(my_handle);
  return ESP_OK;
}

esp_err_t badge_nvs_get_u8(const char* namespace, const char* key, uint8_t *value)
{
  nvs_handle my_handle;
  esp_err_t err;

  // Open
  err = nvs_open(namespace, NVS_READONLY, &my_handle);
  if (err != ESP_OK) return err;

  // Read
  err = nvs_get_u8(my_handle, key, value);
  if (err != ESP_OK) return err;

  // Close
  nvs_close(my_handle);
  return ESP_OK;
}

esp_err_t badge_nvs_set_u16(const char* namespace, const char* key, uint16_t value)
{
  nvs_handle my_handle;
  esp_err_t err;

  // Open
  err = nvs_open(namespace, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) return err;

  // Write
  err = nvs_set_u16(my_handle, key, value);
  if (err != ESP_OK) return err;

  // Commit written value.
  // After setting any values, nvs_commit() must be called to ensure changes are written
  // to flash storage. Implementations may write to storage at other times,
  // but this is not guaranteed.
  err = nvs_commit(my_handle);
  if (err != ESP_OK) return err;

  // Close
  nvs_close(my_handle);
  return ESP_OK;
}

esp_err_t badge_nvs_get_u16(const char* namespace, const char* key, uint16_t *value)
{
  nvs_handle my_handle;
  esp_err_t err;

  // Open
  err = nvs_open(namespace, NVS_READONLY, &my_handle);
  if (err != ESP_OK) return err;

  // Read
  err = nvs_get_u16(my_handle, key, value);
  if (err != ESP_OK) return err;

  // Close
  nvs_close(my_handle);
  return ESP_OK;
}

esp_err_t badge_nvs_set_str(const char* namespace, const char* key, const char *value) {
  nvs_handle my_handle;
  esp_err_t err;

  // Open
  err = nvs_open(namespace, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) return err;

  // Write
  err = nvs_set_str(my_handle, key, value);
  if (err != ESP_OK) return err;

  // Commit
  err = nvs_commit(my_handle);
  if (err != ESP_OK) return err;

  // Close
  nvs_close(my_handle);
  return ESP_OK;
}

esp_err_t badge_nvs_get_str(const char* namespace, const char* key, char *value, size_t *length) {
  nvs_handle my_handle;
  esp_err_t err;

  // Open
  err = nvs_open(namespace, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) return err;

  // Read the size of memory space required for string
  err = nvs_get_str(my_handle, key, NULL, length);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

  // Read previously saved blob if available
  if ((size_t)&length > 0) {
      err = nvs_get_str(my_handle, key, value, length);
      if (err != ESP_OK) return err;
  }

  // Close
  nvs_close(my_handle);
  return ESP_OK;
}
