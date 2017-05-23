#include "sdkconfig.h"

#include <freertos/FreeRTOS.h>
#include <esp_event.h>
#include <gde.h>
#include <gde-driver.h>

#include <badge_eink.h>

#include "event_queue.h"

#include "imgv2_sha.h"
#include "imgv2_nick.h"

void
demoPartialUpdate(void) {
	struct badge_eink_update eink_upd = {
		.lut      = LUT_DEFAULT,
//		.reg_0x3a = 26,   // 26 dummy lines per gate
		.reg_0x3a = 63,   // 63 dummy lines per gate
//		.reg_0x3b = 0x08, // 62us per line
		.reg_0x3b = 0x0f, // 208us per line
		.y_start  = 0,
		.y_end    = 295,
	};

	int i;
	for (i = 0; i < 8; i++) {
		int j = ((i << 1) | (i >> 2)) & 7;
		badge_eink_display(imgv2_sha, DISPLAY_FLAG_NO_UPDATE);
		eink_upd.y_start = 37 * j;
		eink_upd.y_end   = 37 * j + 36;
		badge_eink_update(&eink_upd);

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	for (i = 0; i < 8; i++) {
		int j = ((i << 1) | (i >> 2)) & 7;
		badge_eink_display(imgv2_nick, DISPLAY_FLAG_NO_UPDATE);
		eink_upd.y_start = 37 * j;
		eink_upd.y_end   = 37 * j + 36;
		badge_eink_update(&eink_upd);

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	// wait for random keypress
	uint32_t buttons_down = 0;
	while ((buttons_down & 0xffff) == 0)
		xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY);
}
