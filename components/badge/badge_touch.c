#include <stdbool.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include <freertos/semphr.h>
#include "freertos/task.h"
#include "esp_log.h"

#include "driver/touch_pad.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

#include "badge_base.h"
#include "badge_button.h"
#include "badge_input.h"
#include "badge_touch.h"

static const char *TAG = "badge_touch";

#define TOUCH_THRESH_NO_USE   (0)
#define TOUCH_THRESH_PERCENT  (99)

static uint32_t badge_touch_mask = (1<<0) | (1<<5) | (1<<8);

static uint16_t badge_touch_threshold_lo[TOUCH_PAD_MAX];
static uint16_t badge_touch_threshold_hi[TOUCH_PAD_MAX];
static volatile uint8_t badge_touch_state[TOUCH_PAD_MAX] = { 0, };
static uint8_t badge_touch_type[TOUCH_PAD_MAX] = { 0, };

static xSemaphoreHandle badge_touch_intr_trigger = NULL;

/*
  Read values sensed at all available touch pads.
  Use 2 / 3 of read value as the threshold
  to trigger interrupt when the pad is touched.
  Note: this routine demonstrates a simple way
  to configure activation threshold for the touch pads.
  Do not touch any pads when this routine
  is running (on application start).
 */
static void
tp_example_set_thresholds(void)
{
    uint16_t touch_value;

    // delay some time in order to make the filter work and get a initial value
    vTaskDelay(500/portTICK_PERIOD_MS);

	badge_touch_type[0] = BADGE_BUTTON_START;
	badge_touch_type[5] = BADGE_BUTTON_B;
	badge_touch_type[8] = BADGE_BUTTON_SELECT;

    for (int i = 0; i < TOUCH_PAD_MAX; i++) {
		if ((1<<i) & badge_touch_mask) {
			// read filtered value
			touch_pad_read_filtered(i, &touch_value);

			ESP_LOGI(TAG, "touch %d val: %d", i, touch_value);

			badge_touch_threshold_lo[i] = touch_value * 2 / 3;
			badge_touch_threshold_hi[i] = touch_value * 5 / 6;

			// set interrupt threshold.
			ESP_ERROR_CHECK(touch_pad_set_thresh(i, badge_touch_threshold_lo[i]));
		}
    }
}

/*
  Handle an interrupt triggered when a pad is touched.
  Recognize what pad has been touched and save it in a table.
 */
static void
tp_example_rtc_intr(void * arg)
{
    uint32_t pad_intr = touch_pad_get_status();

    // clear interrupt
    touch_pad_clear_status();

//	ets_printf("badge_touch: rtc intr status=%x", pad_intr);

	pad_intr &= badge_touch_mask;

    for (int i = 0; i < TOUCH_PAD_MAX; i++) {
		if ((1<<i) & pad_intr) {
			if (badge_touch_state[i] == 0) {
				badge_input_add_event(badge_touch_type[i], EVENT_BUTTON_PRESSED, IN_ISR);
				badge_touch_state[i] = 1;

				// activate badge_touch_intr_task()
				xSemaphoreGiveFromISR(badge_touch_intr_trigger, NULL);
			}
		}
	}
}

static void
badge_touch_intr_task(void *arg)
{
	while (1)
	{
		if (xSemaphoreTake(badge_touch_intr_trigger, portMAX_DELAY))
		{
			// activated; loop until no more buttons pressed
			bool active = true;
			while (active)
			{
				vTaskDelay(100 / portTICK_PERIOD_MS);
				active = false;
				for (int i = 0; i < TOUCH_PAD_MAX; i++)
				{
					if (badge_touch_state[i])
					{
						uint16_t touch_value=0;
						touch_pad_read_filtered(i, &touch_value);
						ESP_LOGI(TAG, "touch %d val: %d", i, touch_value);

						if (touch_value > badge_touch_threshold_hi[i])
						{
							badge_input_add_event(badge_touch_type[i], EVENT_BUTTON_RELEASED, NOT_IN_ISR);
							badge_touch_state[i] = 0;
						}
						else
						{
							active = true;
						}
					}
				}
			}
		}
	}
}

/*
 * Before reading touch pad, we need to initialize the RTC IO.
 */
static void
tp_example_touch_pad_init()
{
	// init RTC IO and mode for touch pad.
    for (int i = 0; i < TOUCH_PAD_MAX; i++) {
		if ((1<<i) & badge_touch_mask) {
			touch_pad_config(i, TOUCH_THRESH_NO_USE);
		}
	}
}

void
badge_touch_poll()
{
    uint16_t touch_value;

    for (int i = 0; i < TOUCH_PAD_MAX; i++) {
		if ((1<<i) & badge_touch_mask) {
			// read filtered value
			touch_pad_read_filtered(i, &touch_value);

			ESP_LOGI(TAG, "touch %d val: %d", i, touch_value);
		}
    }
}

void
badge_touch_init()
{
    // Initialize touch pad peripheral, it will start a timer to run a filter
    ESP_LOGI(TAG, "Initializing touch pad");

	badge_touch_intr_trigger = xSemaphoreCreateBinary();
	assert(badge_touch_intr_trigger != NULL);

    touch_pad_init();

    // Initialize and start a software filter to detect slight change of capacitance.
    touch_pad_filter_start(10);

    // Set measuring time and sleep time
    // In this case, measurement will sustain 0xffff / 8Mhz = 8.19ms
    // Meanwhile, sleep time between two measurement will be 0x1000 / 150Khz = 27.3 ms
    touch_pad_set_meas_time(0x1000, 0xffff);

    //set reference voltage for charging/discharging
    // In this case, the high reference valtage will be 2.4V - 1.5V = 0.9V
    // The low reference voltage will be 0.8V, so that the procedure of charging
    // and discharging would be very fast.
    touch_pad_set_voltage(TOUCH_HVOLT_2V4, TOUCH_LVOLT_0V8, TOUCH_HVOLT_ATTEN_1V5);

    // Init touch pad IO
    tp_example_touch_pad_init();

    // Set thresh hold
    tp_example_set_thresholds();

    // Register touch interrupt ISR
    touch_pad_isr_register(tp_example_rtc_intr, NULL);

	// enable interrupts
	touch_pad_intr_enable();

	// enable depress task
	xTaskCreate(&badge_touch_intr_task, "badge_touch: int", 4096, NULL, 10, NULL);
}
