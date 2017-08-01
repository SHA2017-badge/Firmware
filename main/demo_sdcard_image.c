#include "sdkconfig.h"

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"

#include <badge_eink.h>
#include <badge_eink_fb.h>
#include <badge_input.h>
#include <badge_power.h>

#include <file_reader.h>
#include <png_reader.h>

static const char* TAG = "example";

void
demo_sdcard_image(void)
{
	esp_err_t err = badge_eink_fb_init();
	assert( err == ESP_OK );

	memset(badge_eink_fb, 0, BADGE_EINK_FB_LEN);

	badge_power_sdcard_enable();

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
	host.flags = SDMMC_HOST_FLAG_1BIT;
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5
    };

    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", ret);
        }
        return;
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

	struct lib_file_reader *fr = lib_file_new("/sdcard/images/hacking-test.png", 1024);
    if (fr == NULL)
    {
        perror("open()");
		esp_vfs_fat_sdmmc_unmount();
		badge_power_sdcard_disable();
        return;
    }

	struct lib_png_reader *pr = lib_png_new((lib_reader_read_t) &lib_file_read, fr);
    if (pr == NULL)
    {
        fprintf(stderr, "out of memory.\n");
		lib_file_destroy(fr);
		esp_vfs_fat_sdmmc_unmount();
		badge_power_sdcard_disable();
        return;
    }

	int res = lib_png_load_image(pr, badge_eink_fb, 0, 0, BADGE_EINK_WIDTH, BADGE_EINK_HEIGHT, BADGE_EINK_WIDTH);
	lib_png_destroy(pr);
	lib_file_destroy(fr);
	esp_vfs_fat_sdmmc_unmount();
	badge_power_sdcard_disable();

	if (res < 0)
	{
		fprintf(stderr, "failed to load image: res = %i\n", res);
		return;
	}

	badge_eink_display_greyscale(badge_eink_fb, DISPLAY_FLAG_8BITPIXEL, 16);

	// wait for random keypress
	badge_input_get_event(-1);
}
