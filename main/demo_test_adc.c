#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include <esp_event.h>
#include "driver/adc.h"
#include <gde.h>
#include <gdeh029a1.h>

#include "event_queue.h"

void demoTestAdc(void) {
  int channel;
  int adc_mask = 0xFF;

  adc1_config_width(ADC_WIDTH_12Bit);
  for (channel = 0; channel < ADC1_CHANNEL_MAX; channel++)
    if (adc_mask & (1 << channel))
      adc1_config_channel_atten(channel, ADC_ATTEN_0db);

  while (1) {
    uint32_t buttons_down = 0;

    /* update LUT */
    writeLUT(LUT_DEFAULT);

    /* clear screen */
    setRamArea(0, DISP_SIZE_X_B - 1, 0, DISP_SIZE_Y - 1);
    setRamPointer(0, 0);
    gdeWriteCommandInit(0x24);
    {
      int x, y;
      for (y = 0; y < DISP_SIZE_Y; y++) {
        for (x = 0; x < 16; x++)
          gdeWriteByte(0xff);
      }
    }
    gdeWriteCommandEnd();

    for (channel = 0; channel < ADC1_CHANNEL_MAX; channel++) {
#define TEXTLEN 32
      char text[TEXTLEN];
      int val;
      if (adc_mask & (1 << channel))
        val = adc1_get_voltage(channel);
      else
        val = -1;
      snprintf(text, TEXTLEN, "ADC channel %d: %d", channel, val);
      drawText(14 - channel, 16, -16, text,
               FONT_FULL_WIDTH | FONT_MONOSPACE | FONT_INVERT);
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
