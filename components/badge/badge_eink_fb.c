#include <sdkconfig.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <esp_err.h>
#include <esp_log.h>

#include "badge_eink_fb.h"

static const char *TAG = "badge_eink_fb";

uint8_t *badge_eink_fb = NULL;

esp_err_t
badge_eink_fb_init(void)
{
	static bool badge_eink_fb_init_done = false;

	if (badge_eink_fb_init_done)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	badge_eink_fb = (uint8_t *) malloc(BADGE_EINK_FB_LEN);
	if (badge_eink_fb == NULL)
		return ESP_ERR_NO_MEM;

	badge_eink_fb_init_done = true;

	ESP_LOGD(TAG, "init done");

	return ESP_OK;
}
