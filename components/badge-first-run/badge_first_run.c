#include <sdkconfig.h>

#include <string.h>

#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_spi_flash.h>
#include <esp_partition.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>

#include <badge.h>
#include <badge_input.h>
#include <badge_mpr121.h>
#include <badge_eink.h>
#include <badge_pins.h>
#include <badge_button.h>
#include <badge_power.h>
#include <badge_sdcard.h>
#include <font.h>
#include <sha2017_ota.h>

#include <mem_reader.h>
#include <png_reader.h>

#include "png_hacking.h"

#define TAG "badge_first_run"

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
		badge_eink_display(image_buf, DISPLAY_FLAG_LUT(2));
#ifdef CONFIG_DEBUG_ADD_DELAYS
		vTaskDelay(1000 / portTICK_PERIOD_MS);
#endif // CONFIG_DEBUG_ADD_DELAYS

		if (len == 0 || line[len] == 0)
			return;

		line = &line[len];
	}
}

#ifdef I2C_MPR121_ADDR
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
		image_buf[pos           + (xu >> 3)] &= ~( 1 << (xu&7) );
		image_buf[pos           + (xd >> 3)] &= ~( 1 << (xd&7) );
		image_buf[pos + (296/8) + (xd >> 3)] &= ~( 1 << (xd&7) );
	}
	badge_eink_display(image_buf, DISPLAY_FLAG_LUT(2));
#ifdef CONFIG_DEBUG_ADD_DELAYS
	vTaskDelay(100 / portTICK_PERIOD_MS);
#endif // CONFIG_DEBUG_ADD_DELAYS
}
#endif // I2C_MPR121_ADDR

void
badge_first_run(void)
{
	char line[100];

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

#ifdef PIN_NUM_BUTTON_FLASH
	// flash button
	int btn_flash = gpio_get_level(PIN_NUM_BUTTON_FLASH);
	if (btn_flash != 1)
	{
		disp_line("Error: Flash button is pressed.",FONT_MONOSPACE);
		return;
	}
	disp_line("flash button ok.",0);
#endif // PIN_NUM_BUTTON_FLASH

#ifdef I2C_MPR121_ADDR
	// mpr121
	disp_line("initializing MPR121.",0);
	badge_mpr121_init();
	badge_mpr121_configure(NULL, false);

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
#ifdef CONFIG_BASVS_BROKEN_BADGE
/* FIXME */ ti.data[3] = baseline_def[3]; /* FIXME */
#endif // CONFIG_BASVS_BROKEN_BADGE
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
		badge_mpr121_configure(baseline, true);
	}

	for (i=0; i<8; i++) {
		bool check = false;
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
#endif // I2C_MPR121_ADDR

	// power measurements
	disp_line("measure power.",0);
	badge_power_init();
	bool bat_chrg = badge_battery_charge_status();
	if (bat_chrg)
		disp_line("battery is charging",0);
	else
		disp_line("battery is not charging",0);

	disp_line("Vbat = ...",NO_NEWLINE);
	int pwr_vbat = badge_battery_volt_sense();
	sprintf(line, "Vbat = %u.%03u V", pwr_vbat/1000, pwr_vbat % 1000);
	disp_line(line, 0);
	if (pwr_vbat > 100) {
		disp_line("Error: Did not expect any power on Vbat.",FONT_MONOSPACE);
		return;
	}

	disp_line("Vusb = ...",NO_NEWLINE);
	int pwr_vusb = badge_usb_volt_sense();
	sprintf(line, "Vusb = %u.%03u V", pwr_vusb/1000, pwr_vusb % 1000);
	disp_line(line, 0);
	if (pwr_vusb < 4500 || pwr_vusb > 5500) {
		disp_line("Error: Vusb should be approx. 5 volt.",FONT_MONOSPACE);
		return;
	}

	disp_line("power measurements ok.",0);

#if defined(PORTEXP_PIN_NUM_SD_CD) || defined(MPR121_PIN_NUM_SD_CD)
	// sdcard detect (not expecting an sd-card)
	disp_line("read sdcard-detect line.",0);
	badge_sdcard_init();
	bool sdcard = badge_sdcard_detected();
	if (sdcard) {
		disp_line("sdcard detected. (error)",FONT_MONOSPACE);
		return;
	}
	disp_line("no sdcard detected. (as expected)",0);
#endif // *_PIN_NUM_SD_CD

	// test wifi
	disp_line("testing wifi.",0);
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	cfg.nvs_enable = 0;
	int res = esp_wifi_init(&cfg);
	if (res == ESP_OK) {
		res = esp_wifi_set_country(WIFI_COUNTRY_EU);
	}
	if (res == ESP_OK) {
		res = esp_wifi_set_mode(WIFI_MODE_STA);
	}
	if (res == ESP_OK) {
		wifi_config_t wifi_config = {
			.sta = {
				.ssid     = CONFIG_WIFI_SSID,
				.password = CONFIG_WIFI_PASSWORD,
			},
		};
		res = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
	}
	if (res == ESP_OK) {
		res = esp_wifi_start();
	}
	if (res == ESP_OK) {
		wifi_scan_config_t config = {
			.scan_time = {
				.active = {
					.min = 100,
					.max = 200,
				},
			},
		};
		res = esp_wifi_scan_start(&config, true);
	}
	uint16_t num_ap = 0;
	if (res == ESP_OK) {
		res = esp_wifi_scan_get_ap_num(&num_ap);
	}
	wifi_ap_record_t *ap_records = NULL;
	if (res == ESP_OK) {
		ap_records = (wifi_ap_record_t *) malloc(sizeof(wifi_ap_record_t) * num_ap);
		assert(ap_records != NULL);
		res = esp_wifi_scan_get_ap_records(&num_ap, ap_records);
	}
	if (res == ESP_OK) {
		int i;
		sprintf(line, "available ssids: (%d)\n", num_ap);
		disp_line(line, 0);
		for (i=0; i<num_ap; i++) {
			sprintf(line, "ssid '%s'\n", ap_records[i].ssid);
			disp_line(line, 0);
		}
		if (num_ap == 0) {
			disp_line("no ssids found.",0);
			res = -1;
		}
	}
	free(ap_records);
	if (res != ESP_OK) {
		disp_line("wifi init failed.",FONT_MONOSPACE);
		return;
	}

	disp_line("wifi test ok.",0);

	disp_line("Testing done.",0);
#ifdef CONFIG_DEBUG_ADD_DELAYS
	vTaskDelay(10000 / portTICK_PERIOD_MS);
#endif // CONFIG_DEBUG_ADD_DELAYS


	// store initial nvs data
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

	nvs_handle my_handle;
	err = nvs_open("badge", NVS_READWRITE, &my_handle);
	ESP_ERROR_CHECK( err );

#ifdef I2C_MPR121_ADDR
	err = nvs_set_u16(my_handle, "mpr121.base.0", baseline[0]);
	ESP_ERROR_CHECK( err );
	err = nvs_set_u16(my_handle, "mpr121.base.1", baseline[1]);
	ESP_ERROR_CHECK( err );
	err = nvs_set_u16(my_handle, "mpr121.base.2", baseline[2]);
	ESP_ERROR_CHECK( err );
	err = nvs_set_u16(my_handle, "mpr121.base.3", baseline[3]);
	ESP_ERROR_CHECK( err );
	err = nvs_set_u16(my_handle, "mpr121.base.4", baseline[4]);
	ESP_ERROR_CHECK( err );
	err = nvs_set_u16(my_handle, "mpr121.base.5", baseline[5]);
	ESP_ERROR_CHECK( err );
	err = nvs_set_u16(my_handle, "mpr121.base.6", baseline[6]);
	ESP_ERROR_CHECK( err );
	err = nvs_set_u16(my_handle, "mpr121.base.7", baseline[7]);
	ESP_ERROR_CHECK( err );
#endif // I2C_MPR121_ADDR

	err = nvs_set_str(my_handle, "wifi.ssid", CONFIG_WIFI_SSID);
	ESP_ERROR_CHECK( err );
	err = nvs_set_str(my_handle, "wifi.password", CONFIG_WIFI_PASSWORD);
	ESP_ERROR_CHECK( err );

	err = nvs_commit(my_handle);
	ESP_ERROR_CHECK( err );

	nvs_close(my_handle);

	// if we get here, the badge is ok.
	badge_init();
	display_png(png_hacking, sizeof(png_hacking));

	while (1)
	{
		// infinite loop - FIXME: replace by deep sleep
	}
}

void
badge_check_first_run(void)
{
	// search non-volatile storage partition
	const esp_partition_t * nvs_partition = esp_partition_find_first(
			ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
	if (nvs_partition == NULL)
	{
		ESP_LOGE(TAG, "NVS partition not found.");
		return;
	}

	uint8_t buf[64];
	ESP_LOGD(TAG, "nvs partition address: 0x%x\n", nvs_partition->address);
	int res = spi_flash_read(nvs_partition->address, buf, sizeof(buf));
	if (res != ESP_OK)
	{
		ESP_LOGE(TAG, "failed to read from NVS partition: %d", res);
		return;
	}

	{
		ESP_LOGV(TAG, "nvs read:");
		int i;
		for (i=0; i<sizeof(buf); i+=16)
		{
			ESP_LOGV(TAG, "  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
				buf[i+0], buf[i+1], buf[i+2], buf[i+3], buf[i+4], buf[i+5], buf[i+6], buf[i+7],
				buf[i+8], buf[i+9], buf[i+10], buf[i+11], buf[i+12], buf[i+13], buf[i+14], buf[i+15]
			);
		}
	}

	static const uint8_t empty[16] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
	if (memcmp(buf, empty, 16) == 0)
	{ // nvs partition seems empty. start first_run();
		badge_first_run();

		// error occurred; erase nvs sector
		int res = spi_flash_erase_sector(nvs_partition->address / SPI_FLASH_SEC_SIZE);
		assert(res == ESP_OK);

		while (1)
		{
			// infinite loop - FIXME: replace by deep sleep
		}
	}
}
