#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include <gde.h>
#include <gdeh029a1.h>
#include <pictures.h>

esp_err_t event_handler(void *ctx, system_event_t *event) { return ESP_OK; }

uint32_t
get_buttons(void)
{
	uint32_t bits = 0;
	bits |= gpio_get_level(0) << 0; // A
	bits |= gpio_get_level(27) << 1; // B
	bits |= gpio_get_level(25) << 2; // MID
	bits |= gpio_get_level(26) << 3; // UP
	bits |= gpio_get_level(32) << 4; // DOWN
	bits |= gpio_get_level(33) << 5; // LEFT
	bits |= gpio_get_level(35) << 6; // RIGHT
	bits |= gpio_get_level(21) << 7; // GDE BUSY
	return bits;
}

xQueueHandle evt_queue = NULL;

uint32_t buttons_state = -1;

void gpio_intr_test(void *arg)
{
	// read status to get interrupt status for GPIO 0-31
	uint32_t gpio_intr_status_lo = READ_PERI_REG(GPIO_STATUS_REG);
	// read status to get interrupt status for GPIO 32-39
	uint32_t gpio_intr_status_hi = READ_PERI_REG(GPIO_STATUS1_REG);
	// clear intr for GPIO 0-31
	SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status_lo);
	// clear intr for GPIO 32-39
	SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_hi);

	uint32_t buttons_new  = get_buttons();
	uint32_t buttons_down = (~buttons_new) & buttons_state;
	uint32_t buttons_up   = buttons_new & (~buttons_state);
	buttons_state = buttons_new;

	if (buttons_down != 0)
		xQueueSendFromISR(evt_queue, &buttons_down, NULL);

	if (buttons_down & (1 << 0))
		ets_printf("Button A\n");
	if (buttons_down & (1 << 1))
		ets_printf("Button B\n");
	if (buttons_down & (1 << 2))
		ets_printf("Button MID\n");
	if (buttons_down & (1 << 3))
		ets_printf("Button UP\n");
	if (buttons_down & (1 << 4))
		ets_printf("Button DOWN\n");
	if (buttons_down & (1 << 5))
		ets_printf("Button LEFT\n");
	if (buttons_down & (1 << 6))
		ets_printf("Button RIGHT\n");
	if (buttons_down & (1 << 7))
		ets_printf("GDE-Busy down\n");
	if (buttons_up & (1 << 7))
		ets_printf("GDE-Busy up\n");

	// pass on BUSY signal to LED.
	gpio_set_level(22, 1-gpio_get_level(21));
}

struct menu_item {
	const char *title;
	void (*handler)(void);
};

void
demoGreyscale1(void)
{
	// enable slow waveform
	writeLUT(LUT_DEFAULT);

	/* draw test pattern */
	setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
	setRamPointer(0, 0);
	gdeWriteCommandInit(0x24);
	{
		int x,y;
		for (y=0; y<DISP_SIZE_Y; y++) {
			for (x=0; x<16; x++)
				gdeWriteByte( x & 4 ? 0xff : 0x00 );
		}
	}
	gdeWriteCommandEnd();
	updateDisplay();
	gdeBusyWait();
	int y=0;
	int n=8;
	while (y < DISP_SIZE_Y) {
		/* draw new test pattern */
		setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
		setRamPointer(0, 0);
		gdeWriteCommandInit(0x24);
		{
			int x,y;
			for (y=0; y<DISP_SIZE_Y; y++) {
				for (x=0; x<16; x++)
					gdeWriteByte( x & 2 ? 0xff : 0x00 );
			}
		}
		gdeWriteCommandEnd();

		if (y+n > DISP_SIZE_Y)
			n = DISP_SIZE_Y - y;
		updateDisplayPartial(y, y+n);
		gdeBusyWait();
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		y += n;
		n += 4;
	}
	y = 0;
	n = 4;
	while (y < DISP_SIZE_Y) {
		/* draw new test pattern */
		setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
		setRamPointer(0, 0);
		gdeWriteCommandInit(0x24);
		{
			int x,y;
			for (y=0; y<DISP_SIZE_Y; y++) {
				for (x=0; x<16; x++)
					gdeWriteByte( x & 8 ? 0xff : 0x00 );
			}
		}
		gdeWriteCommandEnd();

		if (y+n > DISP_SIZE_Y)
			n = DISP_SIZE_Y - y;
		updateDisplayPartial(y, y+n);
		gdeBusyWait();
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		y += n;
		n += 2;
	}

	// wait for random keypress
	uint32_t buttons_down = 0;
	while ((buttons_down & 0x7f) == 0)
		xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY);
}

void
demoGreyscale2(void)
{
	/* draw initial pattern */
	setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
	setRamPointer(0, 0);
	gdeWriteCommandInit(0x24);
	{
		int x,y;
		for (y=0; y<DISP_SIZE_Y; y++) {
			for (x=0; x<8; x++)
				gdeWriteByte( 0x00 );
			for (x=8; x<16; x++)
				gdeWriteByte( 0xff );
		}
	}
	gdeWriteCommandEnd();

	/* update LUT */
	writeLUT(LUT_DEFAULT);

	/* update display */
	updateDisplay();
	gdeBusyWait();

	gdeWriteCommand_p1(0x3a, 0x02); // 2 dummy lines per gate
//	gdeWriteCommand_p1(0x3b, 0x08); // 62us per line
	gdeWriteCommand_p1(0x3b, 0x00); // 30us per line

	int i;
	for (i=1; i<16; i++)
	{
		/* draw pattern */
		int y_first = 19;
		int y_next = (i+1)*19;
		if (y_next > DISP_SIZE_Y)
			y_next = DISP_SIZE_Y;

		setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
		setRamPointer(0, 0);
		gdeWriteCommandInit(0x24);
		int x,y;
		for (y=0; y<y_first; y++) {
			for (x=0; x<8; x++)
				gdeWriteByte( 0x00 );
			for (x=8; x<16; x++)
				gdeWriteByte( 0xff );
		}
		for (y=y_first; y<y_next; y++) {
			for (x=0; x<8; x++)
				gdeWriteByte( 0xff );
			for (x=8; x<16; x++)
				gdeWriteByte( 0x00 );
		}
		for (y=y_next; y<DISP_SIZE_Y; y++) {
			for (x=0; x<8; x++)
				gdeWriteByte( 0x00 );
			for (x=8; x<16; x++)
				gdeWriteByte( 0xff );
		}
		gdeWriteCommandEnd();

		/* update LUT */
		uint8_t lut[30] = {
			0x18,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,
			i,0,0,0,0,0,0,0,0,0,
		};
		gdeWriteCommandStream(0x32, lut, 30);

		/* update display */
		updateDisplay();
		gdeBusyWait();
	}

	// wait for random keypress
	uint32_t buttons_down = 0;
	while ((buttons_down & 0x7f) == 0)
		xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY);

	gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
	gdeWriteCommand_p1(0x3b, 0x08); // 62us per line
}

const struct menu_item demoMenu[] = {
	{ "greyscale 1", &demoGreyscale1 },
	{ "greyscale 2", &demoGreyscale2 },
	{ "tetris?", NULL },
	{ "something else", NULL },
	{ "test, test, test", NULL },
	{ "another item..", NULL },
	{ "dot", NULL },
	{ "dot 2", NULL },
	{ "dot 3", NULL },
	{ NULL, NULL },
};

void
displayMenu(const char *menu_title, const struct menu_item *itemlist)
{
	int num_items = 0;
	while (itemlist[num_items].title != NULL)
		num_items++;

	int scroll_pos = 0;
	int item_pos = 0;
	int num_draw = 0;
	while (1)
	{
		TickType_t xTicksToWait = portMAX_DELAY;

		/* draw menu */
		if (num_draw < 2) {
			// init buffer
			drawText(14, 0, DISP_SIZE_Y, menu_title,
					FONT_16PX|FONT_INVERT|FONT_FULL_WIDTH|FONT_UNDERLINE_2);
			int i;
			for (i=0; i<7; i++)
			{
				int pos = scroll_pos + i;
				drawText(12 - 2*i, 0, DISP_SIZE_Y,
						(pos < num_items) ? itemlist[pos].title : "",
						FONT_16PX | FONT_FULL_WIDTH |
							((pos == item_pos) ? 0 : FONT_INVERT));
			}
		}
		if (num_draw == 0) {
			// init LUT
			static const uint8_t lut[30] = {
				0x99,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,
				1,0,0,0,0,0,0,0,0,0,
			};
			gdeWriteCommandStream(0x32, lut, 30);
		}
		if (num_draw < 10) {
			updateDisplay();
			gdeBusyWait();
			num_draw++;
			if (num_draw < 10)
				xTicksToWait = 0;
		}

		/* handle input */
		uint32_t buttons_down;
		if (xQueueReceive(evt_queue, &buttons_down, xTicksToWait)) {
			if (buttons_down & (1 << 1)) {
				ets_printf("Button B handling\n");
				return;
			}
			if (buttons_down & (1 << 2)) {
				ets_printf("Selected '%s'\n", itemlist[item_pos].title);
				if (itemlist[item_pos].handler != NULL)
					itemlist[item_pos].handler();
				num_draw = 0;
				ets_printf("Button MID handled\n");
				continue;
			}
			if (buttons_down & (1 << 3)) {
				if (item_pos > 0)
				{
					item_pos--;
					if (scroll_pos > item_pos)
						scroll_pos = item_pos;
					num_draw = 0;
				}
				ets_printf("Button UP handled\n");
			}
			if (buttons_down & (1 << 4)) {
				if (item_pos+1 < num_items)
				{
					item_pos++;
					if (scroll_pos+6 < item_pos)
						scroll_pos = item_pos-6;
					num_draw = 0;
				}
				ets_printf("Button DOWN handled\n");
			}
		}
	}
}

void
app_main(void) {
	nvs_flash_init();

	/* create event queue */
	evt_queue = xQueueCreate(10, sizeof(uint32_t));

	/** configure input **/
	gpio_isr_register(gpio_intr_test, NULL, 0, NULL);

	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_ANYEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask =
		GPIO_SEL_0 |
		GPIO_SEL_21 | // GDE BUSY pin
		GPIO_SEL_25 |
		GPIO_SEL_26 |
		GPIO_SEL_27 |
		GPIO_SEL_32 |
		GPIO_SEL_33 |
		GPIO_SEL_35;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);

	/** configure output **/
    gpio_pad_select_gpio(22);
	gpio_set_direction(22, GPIO_MODE_OUTPUT);

	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
//  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
//  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
//  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//  wifi_config_t sta_config = {.sta = {.ssid = "access_point_name",
//                                      .password = "password",
//                                      .bssid_set = false}};
//  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
//  ESP_ERROR_CHECK(esp_wifi_start());
//  ESP_ERROR_CHECK(esp_wifi_connect());

	gdeInit();
	initDisplay(LUT_DEFAULT); // configure slow LUT

	int picture_id = 0;
	drawImage(pictures[picture_id]);
	updateDisplay();
	gdeBusyWait();

	int selected_lut = LUT_PART;
	writeLUT(selected_lut); // configure fast LUT

	while (1) {
		uint32_t buttons_down;
		if (xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY)) {
			if (buttons_down & (1 << 0)) {
				ets_printf("Button A handling\n");
				if (picture_id + 1 < NUM_PICTURES) {
					picture_id++;
					drawImage(pictures[picture_id]);
					updateDisplay();
					gdeBusyWait();
				}
			}
			if (buttons_down & (1 << 1)) {
				ets_printf("Button B handling\n");
				if (picture_id > 0) {
					picture_id--;
					drawImage(pictures[picture_id]);
					updateDisplay();
					gdeBusyWait();
				}
			}
			if (buttons_down & (1 << 2)) {
				ets_printf("Button MID handling\n");
				drawImage(pictures[picture_id]);
				const char *line_1 = "esp-idf supports compiling multiple files in parallel, so all of the above commands can be run as `make -jN` where `N` is the number of parallel make processes to run (generally N should be equal to or one more than the number of CPU cores in your system.)";

				int pos = 0;
				int row = 14;
				while (line_1[pos]) {
					int num = drawText(row, 16, -16, &line_1[pos], FONT_16PX|FONT_INVERT|FONT_FULL_WIDTH);
					if (num == 0)
						break;
					pos += num;
					row -= 2;
				}
				drawText(row, 16, -16, "", FONT_16PX|FONT_INVERT|FONT_FULL_WIDTH);
				row -= 2;
/*
				const char *line_2 = "Multiple make functions can be combined into one. For example: to build the app & bootloader using 5 jobs in parallel, then flash everything, and then display serial output from the ESP32 run:";
				pos = 0;
				while (line_2[pos]) {
					int num = drawText(row, 16, -16, &line_2[pos], FONT_16PX|FONT_INVERT|FONT_FULL_WIDTH);
					if (num == 0)
						break;
					pos += num;
					row -= 2;
				}
*/

				drawText(0,0,0," Just a status line. Wifi status: not connected",FONT_FULL_WIDTH|FONT_MONOSPACE);

				updateDisplay();
				gdeBusyWait();
			}
			if (buttons_down & (1 << 3)) {
				ets_printf("Button UP handling\n");
				writeLUT(LUT_DEFAULT);
				drawImage(pictures[picture_id]);
				updateDisplay();
				gdeBusyWait();
				writeLUT(selected_lut);
			}
			if (buttons_down & (1 << 4)) {
				ets_printf("Button DOWN handling\n");
				picture_id = (picture_id + 1) % NUM_PICTURES;

				int i;
				for (i=0; i<8; i++) {
					int j = ((i << 1) | (i >> 2)) & 7;
					drawImage(pictures[picture_id]);
					updateDisplayPartial(37*j, 37*j+36);
					gdeBusyWait();
					vTaskDelay(1000 / portTICK_PERIOD_MS);
				}
			}
			if (buttons_down & (1 << 5)) {
				ets_printf("Button LEFT handling\n");
				selected_lut = (selected_lut + 1) % (LUT_MAX+1);
				writeLUT(selected_lut);
				drawImage(pictures[picture_id]);
				updateDisplay();
				gdeBusyWait();
			}
			if (buttons_down & (1 << 6)) {
				ets_printf("Button RIGHT handling\n");
//				demoGreyscale1();
				displayMenu("Demo menu", demoMenu);

				writeLUT(selected_lut);
				drawImage(pictures[picture_id]);
				updateDisplay();
				gdeBusyWait();
			}
		}
	}
}
