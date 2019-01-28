#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include <font.h>
#include <string.h>

#include <badge.h>
#include <badge_input.h>
#include <badge_eink.h>
#include <badge_eink_fb.h>
#include <badge_pins.h>
#include <badge_button.h>
#include <badge_first_run.h>
#include <sha2017_ota.h>

#include "imgv2_sha.h"
#include "imgv2_menu.h"
#include "imgv2_nick.h"
#include "imgv2_weather.h"
#include "imgv2_test.h"

struct menu_item {
  const char *title;
  void (*handler)(void);
};

#include "demo_text1.h"
#include "demo_text2.h"
#include "demo_greyscale1.h"
#include "demo_greyscale2.h"
#include "demo_greyscale_img1.h"
#include "demo_greyscale_img2.h"
#include "demo_greyscale_img3.h"
#include "demo_greyscale_img4.h"
#include "demo_partial_update.h"
#include "demo_dot1.h"
#include "demo_test_adc.h"
#include "demo_leds.h"
#include "demo_ugfx.h"
#include "demo_power.h"
#include "demo_mpr121.h"
#include "demo_sdcard_image.h"

const struct menu_item demoMenu[] = {
#ifdef I2C_MPR121_ADDR
    {"mpr121 touch demo", &demoMpr121},
#endif // I2C_MPR121_ADDR
    {"text demo 1", &demoText1},
    {"text demo 2", &demoText2},
    {"greyscale 1", &demoGreyscale1},
    {"greyscale 2", &demoGreyscale2},
    {"greyscale image 1", &demoGreyscaleImg1},
    {"greyscale image 2", &demoGreyscaleImg2},
    {"greyscale image 3", &demoGreyscaleImg3},
    {"greyscale image 4", &demoGreyscaleImg4},
	{"demo sd-card image", &demo_sdcard_image},
    {"partial update test", &demoPartialUpdate},
    {"dot 1", &demoDot1},
    {"ADC test", &demoTestAdc},
#ifdef PIN_NUM_LEDS
    {"LEDs demo", &demo_leds},
#endif // PIN_NUM_LEDS
    {"uGFX demo", &demoUgfx},
    {"charging demo", &demoPower},
#ifdef CONFIG_WIFI_USE
    {"OTA update", &sha2017_ota_update},
#endif // CONFIG_WIFI_USE
    {"tetris?", NULL},
    {"something else", NULL},
    {"test, test, test", NULL},
    {"another item..", NULL},
    {"dot 2", NULL},
    {"dot 3", NULL},
    {NULL, NULL},
};

void
displayMenu(const char *menu_title, const struct menu_item *itemlist) {
	int num_items = 0;
	while (itemlist[num_items].title != NULL)
		num_items++;

	int scroll_pos = 0;
	int item_pos = 0;
	bool need_redraw = true;
	while (1) {
		/* draw menu */
		if (need_redraw) {
			// init buffer
			draw_font(badge_eink_fb, 0, 0, BADGE_EINK_WIDTH, menu_title,
					FONT_16PX | FONT_INVERT | FONT_FULL_WIDTH | FONT_UNDERLINE_2);
			int i;
			for (i = 0; i < 7; i++) {
				int pos = scroll_pos + i;
				draw_font(badge_eink_fb, 0, 16+16*i, BADGE_EINK_WIDTH,
						(pos < num_items) ? itemlist[pos].title : "",
						FONT_16PX | FONT_FULL_WIDTH |
						((pos == item_pos) ? 0 : FONT_INVERT));
			}

			badge_eink_display(badge_eink_fb, DISPLAY_FLAG_LUT(BADGE_EINK_LUT_NORMAL) );
			need_redraw = false;
		}

		/* handle input */
		uint32_t button_id;
		if ((button_id = badge_input_get_event(-1)) != 0)
		{
			if (button_id == BADGE_BUTTON_B) {
				ets_printf("Button B handling\n");
				return;
			}

			if (button_id == BADGE_BUTTON_START) {
				ets_printf("Selected '%s'\n", itemlist[item_pos].title);
				if (itemlist[item_pos].handler != NULL)
					itemlist[item_pos].handler();

				// reset screen
				memset(badge_eink_fb, 0xff, BADGE_EINK_WIDTH * BADGE_EINK_HEIGHT / 8);
				badge_eink_display(badge_eink_fb, DISPLAY_FLAG_LUT(BADGE_EINK_LUT_NORMAL) | DISPLAY_FLAG_FULL_UPDATE);

				need_redraw = true;
				ets_printf("Button START handled\n");
				continue;
			}

			if (button_id == BADGE_BUTTON_UP) {
				if (item_pos > 0) {
					item_pos--;
					if (scroll_pos > item_pos)
						scroll_pos = item_pos;
					need_redraw = true;
				}
				ets_printf("Button UP handled\n");
			}

			if (button_id == BADGE_BUTTON_DOWN) {
				if (item_pos + 1 < num_items) {
					item_pos++;
					if (scroll_pos + 6 < item_pos)
						scroll_pos = item_pos - 6;
					need_redraw = true;
				}
				ets_printf("Button DOWN handled\n");
			}
		}
	}
}

// pictures
#define NUM_PICTURES 5
const uint8_t *pictures[NUM_PICTURES] = {
	imgv2_sha,
	imgv2_menu,
	imgv2_nick,
	imgv2_weather,
	imgv2_test,
};

void
display_picture(int picture_id, int selected_lut)
{
	memcpy(badge_eink_fb, pictures[picture_id], 296*128/8);
	char str[30];
	if (selected_lut == -1)
		sprintf(str, "[ pic %d, full update ]", picture_id);
	else
		sprintf(str, "[ pic %d, lut %d ]", picture_id, selected_lut);
	draw_font(badge_eink_fb, 8, 4, BADGE_EINK_WIDTH, str, FONT_INVERT);

	badge_eink_display(badge_eink_fb, DISPLAY_FLAG_LUT(selected_lut));
}

#define IIS_SCLK 13
#define IIS_LCLK 15
#define IIS_DSIN 2
#define IIS_DOUT -1

#include "esp_log.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "http_stream.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "esp_peripherals.h"
#include "periph_wifi.h"
#include "badge_power.h"
#include "equalizer.h"

static const char *TAG = "HTTP_MP3_EXAMPLE";
void
do_audio(void) {
    tcpip_adapter_init();

    audio_pipeline_handle_t pipeline;
    audio_element_handle_t http_stream_reader, i2s_stream_writer, mp3_decoder, equalizer;

    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

//    ESP_LOGI(TAG, "[ 1 ] Start audio codec chip");
//	badge_power_sdcard_enable();
//    audio_hal_codec_config_t audio_hal_codec_cfg =  AUDIO_HAL_ES8388_DEFAULT();
//    audio_hal_handle_t hal = audio_hal_init(&audio_hal_codec_cfg, 0);
//    audio_hal_ctrl_codec(hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[2.0] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[2.1] Create http stream to read data");
    http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    http_stream_reader = http_stream_init(&http_cfg);

    ESP_LOGI(TAG, "[2.2] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[2.3] Create mp3 decoder to decode mp3 file");
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_decoder = mp3_decoder_init(&mp3_cfg);

 equalizer_cfg_t eq_cfg = DEFAULT_EQUALIZER_CONFIG();
    int set_gain[] = { -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13, -13};
    eq_cfg.set_gain =
        set_gain; // The size of gain array should be the multiplication of NUMBER_BAND and number channels of audio stream data. The minimum of gain is -13 dB.
    equalizer = equalizer_init(&eq_cfg);

    ESP_LOGI(TAG, "[2.4] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, http_stream_reader, "http");
    audio_pipeline_register(pipeline, mp3_decoder,        "mp3");
    audio_pipeline_register(pipeline, equalizer, 	  "equalizer");
    audio_pipeline_register(pipeline, i2s_stream_writer,  "i2s");

    ESP_LOGI(TAG, "[2.5] Link it together http_stream-->mp3_decoder-->i2s_stream-->[codec_chip]");
    audio_pipeline_link(pipeline, (const char *[]) {"http", "mp3", "equalizer", "i2s"}, 4);

    ESP_LOGI(TAG, "[2.6] Setup uri (http as http_stream, mp3 as mp3 decoder, and default output is i2s)");
    audio_element_set_uri(http_stream_reader, "https://annejan.com/media/Paniq_-_Nonstop_Copyright_Infringement.mp3");

    ESP_LOGI(TAG, "[ 3 ] Start and wait for Wi-Fi network");
    esp_periph_config_t periph_cfg = { 0 };
    esp_periph_init(&periph_cfg);
    periph_wifi_cfg_t wifi_cfg = {
        .ssid = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
    };
    esp_periph_handle_t wifi_handle = periph_wifi_init(&wifi_cfg);
    esp_periph_start(wifi_handle);
    periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);

    ESP_LOGI(TAG, "[ 4 ] Setup event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[4.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(TAG, "[4.2] Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_get_event_iface(), evt);

    ESP_LOGI(TAG, "[ 5 ] Start audio_pipeline");
    audio_pipeline_run(pipeline);

    while (1) {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.source == (void *) mp3_decoder
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(mp3_decoder, &music_info);

            ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                     music_info.sample_rates, music_info.bits, music_info.channels);

            audio_element_setinfo(i2s_stream_writer, &music_info);
            i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
            continue;
        }

        /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2s_stream_writer
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (int) msg.data == AEL_STATUS_STATE_STOPPED) {
            ESP_LOGW(TAG, "[ * ] Stop event received");
            break;
        }
    }

    ESP_LOGI(TAG, "[ 6 ] Stop audio_pipeline");
    audio_pipeline_terminate(pipeline);
}

void
app_main(void) {
	badge_check_first_run();
	badge_init();

	esp_err_t err = badge_eink_fb_init();
	assert( err == ESP_OK );

  int picture_id = 0;
#if 0
	// simple test-mode
	ets_printf("start drawing image\n");
	display_picture(picture_id, -1);
	ets_printf("done drawing image\n");

	while (1) {
		display_picture(picture_id, -1);
		if (picture_id + 1 < NUM_PICTURES) {
			picture_id++;
		} else {
			picture_id=0;
		}
		ets_delay_us(5000000);
	}
#else
  display_picture(picture_id, -1);
  int selected_lut = BADGE_EINK_LUT_NORMAL;

  do_audio();

  while (1) {
    uint32_t button_id;
	if ((button_id = badge_input_get_event(-1)) != 0)
	{
      if (button_id == BADGE_BUTTON_B) {
        ets_printf("Button B handling\n");
        /* redraw with default LUT */
		display_picture(picture_id, -1);
      }
      if (button_id == BADGE_BUTTON_START) {
        ets_printf("Button START handling\n");
        /* open menu */
        displayMenu("Demo menu", demoMenu);
		display_picture(picture_id, selected_lut);
      }
      if (button_id == BADGE_BUTTON_UP) {
        ets_printf("Button UP handling\n");
        /* switch LUT */
        selected_lut = (selected_lut + 1) % (BADGE_EINK_LUT_MAX + 1);
		display_picture(picture_id, selected_lut);
      }
      if (button_id == BADGE_BUTTON_DOWN) {
        ets_printf("Button DOWN handling\n");
        /* switch LUT */
        selected_lut = (selected_lut + BADGE_EINK_LUT_MAX) % (BADGE_EINK_LUT_MAX + 1);
		display_picture(picture_id, selected_lut);
      }
      if (button_id == BADGE_BUTTON_LEFT) {
        ets_printf("Button LEFT handling\n");
        /* previous picture */
        if (picture_id > 0) {
          picture_id--;
		  display_picture(picture_id, selected_lut);
        }
      }
      if (button_id == BADGE_BUTTON_RIGHT) {
        ets_printf("Button RIGHT handling\n");
        /* next picture */
        if (picture_id + 1 < NUM_PICTURES) {
          picture_id++;
		  display_picture(picture_id, selected_lut);
        }
      }
    }
  }
#endif
}

void vPortCleanUpTCB ( void *pxTCB ) {
	// place clean up code here
}
