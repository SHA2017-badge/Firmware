#include <sdkconfig.h>

#include <string.h>

#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_spi_flash.h>
#include <nvs_flash.h>

#include <badge.h>
#include <badge_input.h>
#include <badge_eink.h>
#include <badge_pins.h>
#include <badge_button.h>
#include <font.h>
#include <sha2017_ota.h>

#include <mem_reader.h>
#include <png_reader.h>

#include "png_hacking.h"

uint8_t image_buf[296 * 128];

void
display_png(const uint8_t *png, size_t png_size)
{
	struct lib_mem_reader *mr = lib_mem_new(png, png_size);
    if (mr == NULL)
    {
        fprintf(stderr, "out of memory.\n");
        return;
    }

	struct lib_png_reader *pr = lib_png_new((lib_reader_read_t) &lib_mem_read, mr);
    if (pr == NULL)
    {
        fprintf(stderr, "out of memory.\n");
		lib_mem_destroy(mr);
        return;
    }

	int res = lib_png_load_image(pr, image_buf, 296, 128, 296);
	lib_png_destroy(pr);
	lib_mem_destroy(mr);

	if (res < 0)
	{
		fprintf(stderr, "failed to load image: res = %i\n", res);
		return;
	}

	badge_eink_display(image_buf, DISPLAY_FLAG_GREYSCALE);
}

void
app_main(void)
{
	{
		uint8_t buf[16];
		int magic_offset = 0xf000; // should be SPI_FLASH_SEC_SIZE aligned
		int res = spi_flash_read(magic_offset, buf, sizeof(buf)); // read phy_init
		if (res != 0)
			ets_printf("received: error %d.\n", res);
		else
		{
			char data[sizeof(buf)*3 + 1];
			int i;
			char *ptr = data;
			for (i=0; i<sizeof(buf); i++)
				ptr += sprintf(ptr, "%02x ", buf[i]);
			ptr[-1] = 0;
			ets_printf("received: %s.\n", data);

			if (memcmp(buf, "testmode", 8) == 0)
			{
				badge_eink_init();

				/* insert test-code here */

				int res = spi_flash_erase_sector(magic_offset / SPI_FLASH_SEC_SIZE);
				if (res != 0)
					ets_printf("received: error %d.\n", res);
				else
				{
					badge_init();
					display_png(png_hacking, sizeof(png_hacking));
					while (1)
					{
						// infinite loop
					}
				}
			}
		}
	}

	nvs_flash_init();

	badge_init();
//	display_png(png_ota_update, sizeof(png_ota_update));
	sha2017_ota_update();

	while (1)
	{
		// infinite loop
	}
}
