#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include <gde.h>
#include <gdeh029a1.h>
#include <pictures.h>

#include "img_hacking.h"

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
demoText1(void)
{
	/* update LUT */
	writeLUT(LUT_DEFAULT);

	/* draw test pattern */
	setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
	setRamPointer(0, 0);
	gdeWriteCommandInit(0x24);
	{
		int x,y;
		for (y=0; y<DISP_SIZE_Y; y++) {
			for (x=0; x<16; x++)
				gdeWriteByte( (y & 1) ? 0x55 : 0xaa);
		}
	}
	gdeWriteCommandEnd();

	/* draw text with 8px font */
	const char *line_1 = "esp-idf supports compiling multiple files in parallel, so all of the above commands can be run as `make -jN` where `N` is the number of parallel make processes to run (generally N should be equal to or one more than the number of CPU cores in your system.)";

	int pos = 0;
	int row = 14;
	while (line_1[pos]) {
		int num = drawText(row, 16, -16, &line_1[pos], FONT_INVERT|FONT_FULL_WIDTH);
		if (num == 0)
			break;
		pos += num;
		row--;
	}
	drawText(row, 16, -16, "", FONT_INVERT|FONT_FULL_WIDTH);
	row--;

	const char *line_2 = "Multiple make functions can be combined into one. For example: to build the app & bootloader using 5 jobs in parallel, then flash everything, and then display serial output from the ESP32 run:";
	pos = 0;
	while (line_2[pos]) {
		int num = drawText(row, 16, -16, &line_2[pos], FONT_INVERT|FONT_FULL_WIDTH);
		if (num == 0)
			break;
		pos += num;
		row--;
	}

	// try monospace
	drawText(0,0,0," Just a status line. Wifi status: not connected",FONT_FULL_WIDTH|FONT_MONOSPACE);

	updateDisplay();
	gdeBusyWait();

	// wait for random keypress
	uint32_t buttons_down = 0;
	while ((buttons_down & 0x7f) == 0)
		xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY);
}

void
demoText2(void)
{
	/* update LUT */
	writeLUT(LUT_DEFAULT);

	/* draw test pattern */
	setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
	setRamPointer(0, 0);
	gdeWriteCommandInit(0x24);
	{
		int x,y;
		for (y=0; y<DISP_SIZE_Y; y++) {
			for (x=0; x<16; x++)
				gdeWriteByte( (y & 1) ? 0x55 : 0xaa);
		}
	}
	gdeWriteCommandEnd();

	/* draw text with 16px font */
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

	drawText(0,0,0," Just a status line. Wifi status: not connected",FONT_FULL_WIDTH);

	updateDisplay();
	gdeBusyWait();

	// wait for random keypress
	uint32_t buttons_down = 0;
	while ((buttons_down & 0x7f) == 0)
		xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY);
}

void
demoGreyscale1(void)
{
	/* update LUT */
	writeLUT(LUT_DEFAULT);

	gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
	gdeWriteCommand_p1(0x3b, 0x08); // 62us per line

	int i;
	for (i=0; i<2; i++)
	{
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
	}

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
	/* update LUT */
	writeLUT(LUT_DEFAULT);

	int i;
	for (i=0; i<3; i++)
	{
		/* draw initial pattern */
		setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
		setRamPointer(0, 0);
		gdeWriteCommandInit(0x24);
		{
			int x,y;
			for (y=0; y<DISP_SIZE_Y; y++) {
				for (x=0; x<8; x++)
					gdeWriteByte( (i & 1) ? 0xff : 0x00 );
				for (x=8; x<16; x++)
					gdeWriteByte( (i & 1) ? 0x00 : 0xff );
			}
		}
		gdeWriteCommandEnd();

		/* update display */
		updateDisplay();
		gdeBusyWait();
	}

	gdeWriteCommand_p1(0x3a, 0x02); // 2 dummy lines per gate
//	gdeWriteCommand_p1(0x3b, 0x08); // 62us per line
	gdeWriteCommand_p1(0x3b, 0x00); // 30us per line

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

	gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
	gdeWriteCommand_p1(0x3b, 0x08); // 62us per line

	// wait for random keypress
	uint32_t buttons_down = 0;
	while ((buttons_down & 0x7f) == 0)
		xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY);
}

/*
 * The greyscale curve was generated with:
 *
 * perl -MPOSIX -we 'my $e = 0.15; my $n=15; for (my $i=0; $i<=$n; $i++) { print (POSIX::round(255 / (1-$e) - ($e**($i/$n)) * 255 / (1-$e)),"\n") }' | xargs | sed 's/ /,/g'
 */
void
demoGreyscaleImg1(void)
{
	/* update LUT */
	writeLUT(LUT_DEFAULT);
	gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
	gdeWriteCommand_p1(0x3b, 0x08); // 62us per line

	// trying to get rid of all ghosting and end with a black screen.
	int i;
	for (i=0; i<3; i++)
	{
		/* draw initial pattern */
		setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
		setRamPointer(0, 0);
		gdeWriteCommandInit(0x24);
		int c;
		for (c=0; c<DISP_SIZE_X_B*DISP_SIZE_Y; c++)
			gdeWriteByte((i&1) ? 0xff : 0x00);
		gdeWriteCommandEnd();

		/* update display */
		updateDisplay();
		gdeBusyWait();
	}

	gdeWriteCommand_p1(0x3a, 0x00); // no dummy lines per gate
	gdeWriteCommand_p1(0x3b, 0x00); // 30us per line

	for (i=1; i<16; i++)
	{
//		// linear
//		uint8_t lvl = 17 * i;

		// curved
		const uint8_t lvl_buf[16] = {
//			0,32,61,87,110,131,150,167,182,196,209,220,230,239,248,255 // 0.21
			0,36,67,95,119,141,160,176,191,204,215,225,234,242,249,255 // 0.15
		};
		uint8_t lvl = lvl_buf[i];

		setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
		setRamPointer(0, 0);
		gdeWriteCommandInit(0x24);
		int x,y;
		const uint8_t *ptr = img_hacking;
		for (y=0; y<DISP_SIZE_Y; y++) {
			uint8_t res=0;
			for (x=0; x<DISP_SIZE_X; x++) {
				res <<= 1;
				if (*ptr++ <= lvl)
					res++;
				if ((x & 7) == 7)
					gdeWriteByte( res );
			}
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

	gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
	gdeWriteCommand_p1(0x3b, 0x08); // 62us per line

	// wait for random keypress
	uint32_t buttons_down = 0;
	while ((buttons_down & 0x7f) == 0)
		xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY);
}

void
demoGreyscaleImg2(void)
{
	// curved
	const uint8_t lvl_buf[64] = {
		0,9,18,26,34,42,50,57,64,71,78,85,91,97,103,109,115,120,126,131,136,141,145,150,154,159,163,167,171,175,178,182,186,189,192,195,199,202,204,207,210,213,215,218,220,223,225,227,229,231,233,235,237,239,241,243,244,246,248,249,251,252,254,255 // e=0.15, n=63
	};

	/* update LUT */
	writeLUT(LUT_DEFAULT);
	gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
	gdeWriteCommand_p1(0x3b, 0x08); // 62us per line

	// trying to get rid of all ghosting and end with a black screen.
	int i;
	for (i=0; i<3; i++)
	{
		/* draw initial pattern */
		setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
		setRamPointer(0, 0);
		gdeWriteCommandInit(0x24);
		int c;
		for (c=0; c<DISP_SIZE_X_B*DISP_SIZE_Y; c++)
			gdeWriteByte((i&1) ? 0xff : 0x00);
		gdeWriteCommandEnd();

		/* update display */
		updateDisplay();
		gdeBusyWait();
	}

	gdeWriteCommand_p1(0x3a, 0x00); // no dummy lines per gate
	gdeWriteCommand_p1(0x3b, 0x00); // 30us per line

	int j;
	for (j=0; j<4; j++)
	{
		int y_start = 0 + j*(DISP_SIZE_Y/4);
		int y_end = y_start + (DISP_SIZE_Y/4) - 1;

		for (i=32; i>0; i>>=1)
		{
			setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
			setRamPointer(0, 0);
			gdeWriteCommandInit(0x24);
			int x,y;
			const uint8_t *ptr = img_hacking;
			for (y=0; y<DISP_SIZE_Y; y++) {
				uint8_t res=0;
				for (x=0; x<DISP_SIZE_X; x++) {
					res <<= 1;
					uint8_t pixel = *ptr++;
					int j;
					for (j=0; pixel > lvl_buf[j]; j++);
					if (y >= y_start && y <= y_end && j & i)
						res++;
					if ((x & 7) == 7)
						gdeWriteByte( res );
				}
			}
			gdeWriteCommandEnd();

			// LUT:
			//   Ignore old state;
			//   Do nothing when bit is not set;
			//   Make pixel whiter when bit is set;
			//   Duration is <i> cycles.
			if (i<=15) {
				uint8_t lut[30] = {
					0x88,0,0,0,0,0,0,0,0,0,
					0,0,0,0,0,0,0,0,0,0,
					i,0,0,0,0,0,0,0,0,0,
				};
				gdeWriteCommandStream(0x32, lut, 30);
			} else if (i<=30) {
				uint8_t lut[30] = {
					0x88,0x88,0,0,0,0,0,0,0,0,
					0,0,0,0,0,0,0,0,0,0,
					((i-15)<<4)+15,0,0,0,0,0,0,0,0,0,
				};
				gdeWriteCommandStream(0x32, lut, 30);
			} else if (i<=45) {
				uint8_t lut[30] = {
					0x88,0x88,0x88,0,0,0,0,0,0,0,
					0,0,0,0,0,0,0,0,0,0,
					0xff,(i-30),0,0,0,0,0,0,0,0,
				};
				gdeWriteCommandStream(0x32, lut, 30);
			} else if (i<=60) {
				uint8_t lut[30] = {
					0x88,0x88,0x88,0x88,0,0,0,0,0,0,
					0,0,0,0,0,0,0,0,0,0,
					0xff,((i-45)<<4)+15,0,0,0,0,0,0,0,0,
				};
				gdeWriteCommandStream(0x32, lut, 30);
			}

			/* update display */
			updateDisplayPartial(y_start, y_end + 1);
			gdeBusyWait();
		}
	}

	gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
	gdeWriteCommand_p1(0x3b, 0x08); // 62us per line

	// wait for random keypress
	uint32_t buttons_down = 0;
	while ((buttons_down & 0x7f) == 0)
		xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY);
}

void
demoGreyscaleImg3(void)
{
	// curved
	const uint8_t lvl_buf[128] = {
		0,4,9,13,17,22,26,30,34,38,42,45,49,53,57,60,64,67,71,74,77,81,84,87,90,93,97,100,103,105,108,111,114,117,119,122,125,127,130,132,135,137,140,142,145,147,149,151,154,156,158,160,162,164,166,168,170,172,174,176,178,179,181,183,185,186,188,190,191,193,195,196,198,199,201,202,204,205,206,208,209,211,212,213,214,216,217,218,219,221,222,223,224,225,226,227,228,230,231,232,233,234,235,236,237,237,238,239,240,241,242,243,244,245,245,246,247,248,249,249,250,251,252,252,253,254,254,255 // e=0.15, n=127
	};

	/* update LUT */
	writeLUT(LUT_DEFAULT);
	gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
	gdeWriteCommand_p1(0x3b, 0x08); // 62us per line

	// trying to get rid of all ghosting and end with a black screen.
	int i;
	for (i=0; i<3; i++)
	{
		/* draw initial pattern */
		setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
		setRamPointer(0, 0);
		gdeWriteCommandInit(0x24);
		int c;
		for (c=0; c<DISP_SIZE_X_B*DISP_SIZE_Y; c++)
			gdeWriteByte((i&1) ? 0xff : 0x00);
		gdeWriteCommandEnd();

		/* update display */
		updateDisplay();
		gdeBusyWait();
	}

	gdeWriteCommand_p1(0x3a, 0x00); // no dummy lines per gate
	gdeWriteCommand_p1(0x3b, 0x00); // 30us per line

	for (i=64; i>0; i>>=1)
	{
		int ii = i;
		int p = 8;

		while ((ii & 1) == 0 && (p > 1))
		{
			ii >>= 1;
			p >>= 1;
		}

		int j;
		for (j=0; j<p; j++)
		{
			int y_start = 0 + j*(DISP_SIZE_Y/p);
			int y_end = y_start + (DISP_SIZE_Y/p) - 1;

			setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
			setRamPointer(0, 0);
			gdeWriteCommandInit(0x24);
			int x,y;
			const uint8_t *ptr = img_hacking;
			for (y=0; y<DISP_SIZE_Y; y++) {
				uint8_t res=0;
				for (x=0; x<DISP_SIZE_X; x++) {
					res <<= 1;
					uint8_t pixel = *ptr++;
					int j;
					for (j=0; pixel > lvl_buf[j]; j++);
					if (y >= y_start && y <= y_end && j & i)
						res++;
					if ((x & 7) == 7)
						gdeWriteByte( res );
				}
			}
			gdeWriteCommandEnd();

			// LUT:
			//   Ignore old state;
			//   Do nothing when bit is not set;
			//   Make pixel whiter when bit is set;
			//   Duration is <ii> cycles.
			uint8_t lut[30] = {
				0x88,0,0,0,0,0,0,0,0,0,
				0,0,0,0,0,0,0,0,0,0,
				ii,0,0,0,0,0,0,0,0,0,
			};
			gdeWriteCommandStream(0x32, lut, 30);

			/* update display */
			updateDisplayPartial(y_start, y_end + 1);
			gdeBusyWait();
		}
	}

	gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
	gdeWriteCommand_p1(0x3b, 0x08); // 62us per line

	// wait for random keypress
	uint32_t buttons_down = 0;
	while ((buttons_down & 0x7f) == 0)
		xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY);
}

void
demoPartialUpdate(void)
{
	/* update LUT */
	writeLUT(LUT_DEFAULT);

	// tweak timing a bit.. (FIXME)
	gdeWriteCommand_p1(0x3a, 0x3f); // 63 dummy lines per gate
	gdeWriteCommand_p1(0x3b, 0x0f); // 208us per line

	int i;
	for (i=0; i<8; i++) {
		int j = ((i << 1) | (i >> 2)) & 7;
		drawImage(gImage_sha);
		updateDisplayPartial(37*j, 37*j+36);
		gdeBusyWait();
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	for (i=0; i<8; i++) {
		int j = ((i << 1) | (i >> 2)) & 7;
		drawImage(gImage_nick);
		updateDisplayPartial(37*j, 37*j+36);
		gdeBusyWait();
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
	gdeWriteCommand_p1(0x3b, 0x08); // 62us per line

	// wait for random keypress
	uint32_t buttons_down = 0;
	while ((buttons_down & 0x7f) == 0)
		xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY);
}

void
demoDot1(void)
{
	/* update LUT */
	writeLUT(LUT_DEFAULT);

	/* clear screen */
	setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
	setRamPointer(0, 0);
	gdeWriteCommandInit(0x24);
	{
		int x,y;
		for (y=0; y<DISP_SIZE_Y; y++) {
			for (x=0; x<16; x++)
				gdeWriteByte(0xff);
		}
	}
	gdeWriteCommandEnd();

	/* update display */
	updateDisplay();
	gdeBusyWait();

	// init LUT
	static const uint8_t lut[30] = {
//		0x18,0,0,0,0,0,0,0,0,0,
		0x99,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		1,0,0,0,0,0,0,0,0,0,
	};
	gdeWriteCommandStream(0x32, lut, 30);

	// tweak timing a bit..
	gdeWriteCommand_p1(0x3a, 0x02); // 2 dummy lines per gate
	gdeWriteCommand_p1(0x3b, 0x00); // 30us per line

	int px = 100;
	int py = 64;
	int xd = 1;
	int yd = 1;

	uint16_t dots[16] = { 0, };
	int dot_pos = 0;

	while (1)
	{
		uint32_t buttons_down = 0;
		if (xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY))
			if ((buttons_down & 0x7f) != 0)
				return;

		/* update dot */
		if (px + xd < 0)
			xd = -xd;
		if (px + xd >= DISP_SIZE_X)
			xd = -xd;
		px += xd;

		if (py + yd < 0)
			yd = -yd;
		if (py + yd >= DISP_SIZE_Y)
			yd = -yd;
		py += yd;

		dots[dot_pos] = (py << 7) | px;
		dot_pos++;
		if (dot_pos == 16)
			dot_pos = 0;

		/* clear screen */
		setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
		setRamPointer(0, 0);
		gdeWriteCommandInit(0x24);
		{
			int x,y;
			for (y=0; y<DISP_SIZE_Y; y++) {
				for (x=0; x<16; x++)
					gdeWriteByte(0xff);
			}
		}
		gdeWriteCommandEnd();

		int i;
		for (i=0; i<16; i++)
		{
			int px = dots[i] & 127;
			int py = dots[i] >> 7;
			setRamPointer(px >> 3, py);
			gdeWriteCommand_p1(0x24, 0xff ^ (128 >> (px & 7)));
		}

		/* update display */
		updateDisplay();
		gdeBusyWait();
	}
}

void testAdc(void)
{
	int channel;
	int adc_mask = 0xFF;

	adc1_config_width(ADC_WIDTH_12Bit);
	for (channel = 0; channel < ADC1_CHANNEL_MAX; channel++)
		if (adc_mask & (1<<channel))
			adc1_config_channel_atten(channel, ADC_ATTEN_0db);

	while (1)
	{
		uint32_t buttons_down = 0;

		/* update LUT */
		writeLUT(LUT_DEFAULT);

		/* clear screen */
		setRamArea(0, DISP_SIZE_X_B-1, 0, DISP_SIZE_Y-1);
		setRamPointer(0, 0);
		gdeWriteCommandInit(0x24);
		{
			int x,y;
			for (y=0; y<DISP_SIZE_Y; y++) {
				for (x=0; x<16; x++)
					gdeWriteByte(0xff);
			}
		}
		gdeWriteCommandEnd();

		for (channel = 0; channel < ADC1_CHANNEL_MAX; channel++)
		{
#define TEXTLEN 32
			char text[TEXTLEN];
			int val;
			if (adc_mask & (1<<channel))
				val = adc1_get_voltage(channel);
			else
				val = -1;
			snprintf(text, TEXTLEN, "ADC channel %d: %d", channel, val);
			drawText(14-channel,16,-16,text,FONT_FULL_WIDTH|FONT_MONOSPACE|FONT_INVERT);
			ets_printf("%s\n", text);
		}

		/* update display */
		updateDisplay();
		gdeBusyWait();

		if (xQueueReceive(evt_queue, &buttons_down, portMAX_DELAY))
			if ((buttons_down & 0x7f) != 0)
				return;
	}

}

const struct menu_item demoMenu[] = {
	{ "text demo 1", &demoText1 },
	{ "text demo 2", &demoText2 },
	{ "greyscale 1", &demoGreyscale1 },
	{ "greyscale 2", &demoGreyscale2 },
	{ "greyscale image 1", &demoGreyscaleImg1 },
	{ "greyscale image 2", &demoGreyscaleImg2 },
	{ "greyscale image 3", &demoGreyscaleImg3 },
	{ "partial update test", &demoPartialUpdate },
	{ "dot 1", &demoDot1 },
	{ "ADC test", &testAdc },
	{ "tetris?", NULL },
	{ "something else", NULL },
	{ "test, test, test", NULL },
	{ "another item..", NULL },
	{ "dot 2", NULL },
	{ "dot 3", NULL },
	{ NULL, NULL },
};

#define MENU_UPDATE_CYCLES 8
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
				2,0,0,0,0,0,0,0,0,0,
			};
			gdeWriteCommandStream(0x32, lut, 30);
			// init timings
			gdeWriteCommand_p1(0x3a, 0x02); // 2 dummy lines per gate
			gdeWriteCommand_p1(0x3b, 0x00); // 30us per line
//			gdeWriteCommand_p1(0x3a, 0x1a); // 26 dummy lines per gate
//			gdeWriteCommand_p1(0x3b, 0x08); // 62us per line
		}
		if (num_draw < MENU_UPDATE_CYCLES) {
			updateDisplay();
			gdeBusyWait();
			num_draw++;
			if (num_draw < MENU_UPDATE_CYCLES)
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
			if (buttons_down & (1 << 1)) {
				ets_printf("Button B handling\n");
				/* redraw with default LUT */
				writeLUT(LUT_DEFAULT);
				drawImage(pictures[picture_id]);
				updateDisplay();
				gdeBusyWait();
				writeLUT(selected_lut);
			}
			if (buttons_down & (1 << 2)) {
				ets_printf("Button MID handling\n");
				/* open menu */
				displayMenu("Demo menu", demoMenu);

				writeLUT(selected_lut);
				drawImage(pictures[picture_id]);
				updateDisplay();
				gdeBusyWait();
			}
			if (buttons_down & (1 << 3)) {
				ets_printf("Button UP handling\n");
				/* switch LUT */
				selected_lut = (selected_lut + 1) % (LUT_MAX+1);
				writeLUT(selected_lut);
				drawImage(pictures[picture_id]);
				updateDisplay();
				gdeBusyWait();
			}
			if (buttons_down & (1 << 4)) {
				ets_printf("Button DOWN handling\n");
				/* switch LUT */
				selected_lut = (selected_lut + LUT_MAX) % (LUT_MAX+1);
				writeLUT(selected_lut);
				drawImage(pictures[picture_id]);
				updateDisplay();
				gdeBusyWait();
			}
			if (buttons_down & (1 << 5)) {
				ets_printf("Button LEFT handling\n");
				/* previous picture */
				if (picture_id > 0) {
					picture_id--;
					drawImage(pictures[picture_id]);
					updateDisplay();
					gdeBusyWait();
				}
			}
			if (buttons_down & (1 << 6)) {
				ets_printf("Button RIGHT handling\n");
				/* next picture */
				if (picture_id + 1 < NUM_PICTURES) {
					picture_id++;
					drawImage(pictures[picture_id]);
					updateDisplay();
					gdeBusyWait();
				}
			}
		}
	}
}
