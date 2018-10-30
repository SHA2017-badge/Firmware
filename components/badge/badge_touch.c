#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "driver/touch_pad.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

#include "badge_base.h"
#include "badge_button.h"
#include "badge_input.h"

static const char* TAG = "Touch pad";
#define TOUCH_THRESH_NO_USE   (0)
#define TOUCH_THRESH_PERCENT  (99)


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

    for (int i = 5; i<=6; i++) {
        // read filtered value
        touch_pad_read_filtered(i, &touch_value);

        ESP_LOGI(TAG, "test init touch val: %d\n", touch_value);

        // set interrupt threshold.
        ESP_ERROR_CHECK(touch_pad_set_thresh(i, touch_value * 2 / 3));

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

	if ((pad_intr >> 5) & 1) {
		// Button B
		badge_input_add_event(BADGE_BUTTON_B, EVENT_BUTTON_PRESSED, IN_ISR);
	}

	if ((pad_intr >> 6) & 1) {
		// Button A
		badge_input_add_event(BADGE_BUTTON_A, EVENT_BUTTON_PRESSED, IN_ISR);
	}
}

/*
 * Before reading touch pad, we need to initialize the RTC IO.
 */
static void
tp_example_touch_pad_init()
{
	// init RTC IO and mode for touch pad.
	touch_pad_config(5, TOUCH_THRESH_NO_USE);

	// init RTC IO and mode for touch pad.
	touch_pad_config(6, TOUCH_THRESH_NO_USE);
}

void
badge_touch_init()
{
    // Initialize touch pad peripheral, it will start a timer to run a filter
    ESP_LOGI(TAG, "Initializing touch pad");
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
}
