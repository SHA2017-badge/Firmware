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

uint32_t baseline_def[8] = {
	0x0138,
	0x0144,
	0x0170,
	0x0174, // guessed; 0x01c9, // too high on my badge
	0x00f0,
	0x0103,
	0x00ff,
	0x00ed,
};

const char *touch_name[8] = {
	"A",
	"B",
	"START",
	"SELECT",
	"DOWN",
	"RIGHT",
	"UP",
	"LEFT",
};

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
update_mpr121_bars( const struct badge_mpr121_touch_info *ti, const uint32_t *baseline_top, const uint32_t *baseline_bottom )
{
	int y;
	for (y=0; y<8; y++) {
		int x  = ti->data[y]        - 180;
		int xu = baseline_top[y]    - 180;
		int xd = baseline_bottom[y] - 180;

		if (x > 295) x = 295;
		if (xu > 295) xu = 295;
		if (xd > 295) xd = 295;

		int pos = ( 102 + y*3 ) * (296/8);
		memset(&image_buf[pos-(296/8)], 0xff, (296/8)*3);
		while (x >= 0)
		{
			image_buf[pos + (x >> 3)] &= ~( 1 << (x&7) );
			x--;
		}
		image_buf[pos - (296/8) + (xu >> 3)] &= ~( 1 << (xu&7) );
		image_buf[pos + (296/8) + (xd >> 3)] &= ~( 1 << (xd&7) );
	}
	badge_eink_display(image_buf, DISPLAY_FLAG_LUT(1));
	vTaskDelay(100 / portTICK_PERIOD_MS);
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
	}

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
	int i;
	uint32_t baseline[8] = { 0,0,0,0,0,0,0,0 };
	// read touch data for a few seconds; take average as baseline.
	for (i=0; i<16; i++) {
		struct badge_mpr121_touch_info ti;
		int res = badge_mpr121_get_touch_info(&ti);
		if (res != 0)
		{
			disp_line("Error: failed to read touch info!", FONT_MONOSPACE);
			return;
		}
		int y;
		for (y=0; y<8; y++) {
			baseline[y] += ti.data[y];
		}
		update_mpr121_bars(&ti, baseline_def, baseline_def);
	}

	for (i=0; i<8; i++) {
		baseline[i] = (baseline[i] + 8) >> 4;
	}

	disp_line("re-initializing MPR121.",0);
	{
		uint32_t baseline_8bit[8];
		for (i=0; i<8; i++)
			baseline_8bit[i] = baseline[i] >> 2;
		badge_mpr121_reconfigure(baseline_8bit);
	}

	for (i=0; i<8; i++) {
		bool check = false;
		char line[100];
		if (baseline[i] * 100 < baseline_def[i] * 95) {
			// more than 5% off
			sprintf(line, "odd readings for button %s. (low)", touch_name[i]);
			disp_line(line,FONT_MONOSPACE);
			check = true;
		} else if (baseline[i] * 95 > baseline_def[i] * 100) {
			// more than 5% off
			sprintf(line, "odd readings for button %s. (high)", touch_name[i]);
			disp_line(line,FONT_MONOSPACE);
			check = true;
		}

		if (check) {
			sprintf(line, "*ACTION* touch button %s.", touch_name[i]);
			disp_line(line, FONT_INVERT|NO_NEWLINE);
			// wait for touch event
			while (1) {
				struct badge_mpr121_touch_info ti;
				int res = badge_mpr121_get_touch_info(&ti);
				if (res != 0)
				{
					disp_line("Error: failed to read touch info!", FONT_MONOSPACE);
					return;
				}
				update_mpr121_bars(&ti, baseline_def, baseline);
				if (((ti.touch_state >> i) & 1) == 1)
					break;
			}

			sprintf(line, "button %s touch ok.", touch_name[i]);
			disp_line(line, 0);

			sprintf(line, "*ACTION* release button %s.", touch_name[i]);
			disp_line(line, FONT_INVERT|NO_NEWLINE);
			// wait for release event
			while (1) {
				struct badge_mpr121_touch_info ti;
				int res = badge_mpr121_get_touch_info(&ti);
				if (res != 0)
				{
					disp_line("Error: failed to read touch info!", FONT_MONOSPACE);
					return;
				}
				update_mpr121_bars(&ti, baseline_def, baseline);
				if (((ti.touch_state >> i) & 1) == 0)
					break;
			}

			sprintf(line, "button %s release ok.", touch_name[i]);
			disp_line(line, 0);
		}
	}

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
