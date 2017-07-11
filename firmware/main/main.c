#include <sdkconfig.h>

#include <string.h>

#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_spi_flash.h>
#include <esp_partition.h>
#include <nvs_flash.h>

#include <badge.h>
#include <badge_input.h>
#include <badge_mpr121.h>
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

#define NUM_DISP_LINES 12
#define NO_NEWLINE 0x80
// can use the lower <n> lines for showing measurements
void
disp_line(const char *line, int flags)
{
	static int next_line = 0;
	while (1)
	{
		int height = (flags & FONT_16PX) ? 2 : 1;
		while (next_line >= NUM_DISP_LINES - height)
		{ // scroll up
			next_line--;
			memmove(image_buf, &image_buf[296], (NUM_DISP_LINES-1)*296);
			memset(&image_buf[(NUM_DISP_LINES-1)*296], 0xff, 296);
		}
		int len = draw_font(image_buf, 0, 8*next_line, 296, line, (FONT_FULL_WIDTH|FONT_INVERT)^flags);
		if (height == 2)
			next_line++;
		if ((flags & NO_NEWLINE) == 0)
		{
			next_line++;
			draw_font(image_buf, 0, 8*next_line, 296, "_", FONT_FULL_WIDTH|FONT_INVERT);
		}
		badge_eink_display(image_buf, DISPLAY_FLAG_LUT(1));
		vTaskDelay(1000 / portTICK_PERIOD_MS);

		if (len == 0 || line[len] == 0)
			return;

		line = &line[len];
	}
}

void
first_run(void)
{
	// initialize display
	badge_eink_init();

	// start with white screen
	memset(image_buf, 0xff, sizeof(image_buf));
	badge_eink_display(image_buf, DISPLAY_FLAG_LUT(0));

	// add line in split-screen
	if (NUM_DISP_LINES < 16) {
		memset(&image_buf[NUM_DISP_LINES*296], 0x00, 296/8);
		draw_font(image_buf, 0, 8 + 8*NUM_DISP_LINES, 296, " <some data>", FONT_FULL_WIDTH|FONT_16PX|FONT_INVERT);
	}
#define NUM_DISP_LINES 12
	disp_line("SHA2017-Badge build #18", FONT_16PX);
	disp_line("",0);
	disp_line("Initializing and testing badge.",0);

	// do checks

	// flash button
	int btn_flash = gpio_get_level(PIN_NUM_BUTTON_FLASH);
	if (btn_flash != 1)
	{
		disp_line("Error: Flash button is pressed.",FONT_MONOSPACE);
		return;
	}
	disp_line("flash button ok.",0);

	// mpr121
	disp_line("initializing MPR121.",0);
	badge_mpr121_init(NULL);
	disp_line("reading touch data.",0);
	// read touch data for 2 seconds; take average as baseline.
	// compare baseline with stored defaults.
	// if more than 5% off, then request touch/release
	disp_line("odd readings for button SELECT.",FONT_MONOSPACE);
	disp_line("re-initializing MPR121.",0);
	disp_line("*ACTION* touch button SELECT.",FONT_INVERT|NO_NEWLINE);
	disp_line("button SELECT touch ok.",0);
	disp_line("*ACTION* release button SELECT.",FONT_INVERT|NO_NEWLINE);
	disp_line("button SELECT release ok.",0);
	disp_line("MPR121 ok.",0);

	// power measurements
	disp_line("TODO: measure power.",0);
	disp_line("power measurements ok.",0); // i guess.. :)

	// sdcard detect (not expecting an sd-card)
	disp_line("TODO: read sdcard-detect line.",0);
	disp_line("no sdcard detected. (as expected)",0);

	// test wifi
	disp_line("TODO: test wifi.",0);
	disp_line("wifi test ok.",0);

	disp_line("Testing done.",0);
	return;


	// store initial nvs data

	// if we get here, the badge is ok.
	badge_init();
	display_png(png_hacking, sizeof(png_hacking));
}

void
app_main(void)
{
	{
		// search non-volatile storage partition
		const esp_partition_t * nvs_partition = esp_partition_find_first(
				ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
		assert(nvs_partition && "partition table must have an NVS partition");

		uint8_t buf[64];
		ets_printf("nvs partition address: 0x%x\n", nvs_partition->address);
		int res = spi_flash_read(nvs_partition->address, buf, sizeof(buf));
		assert(res == ESP_OK);

		char data[sizeof(buf)*3 + 1];
		int i;
		char *ptr = data;
		for (i=0; i<sizeof(buf); i++)
			ptr += sprintf(ptr, "%02x ", buf[i]);
		ptr[-1] = 0;
		ets_printf("nvs read: %s.\n", data);

		static const uint8_t empty[16] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
		if (memcmp(buf, empty, 16) == 0)
		{ // nvs partition seems empty. start first_run();
			first_run();
			while (1)
			{
				// infinite loop
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
