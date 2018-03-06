
#include "sdkconfig.h"

#include <gfx.h>

#include "sha2017_ota_graphics.h"

font_t permanentMarker36;
font_t robotoBlackItalic;
font_t permanentMarker;

void sha2017_ota_percentage_init() {
  gfxInit();

  robotoBlackItalic = gdispOpenFont("Roboto_BlackItalic24");
  permanentMarker = gdispOpenFont("PermanentMarker22");
  permanentMarker36 = gdispOpenFont("PermanentMarker36");
}

void show_percentage(char *name, uint8_t percentage, bool show_percentage) {

  color_t front = White;
  color_t back = Black;

  gdispClear(back);

  if (show_percentage) {
    target_lut = 2;
    char perc[10];
    sprintf(perc, "%d%%", percentage);
    gdispDrawString(30, 45, perc, permanentMarker36, front);
  } else {
    target_lut = 0;
  }

  gdispDrawString(show_percentage ? 150 : 60, 25, "STILL", robotoBlackItalic, front);
  gdispDrawString(show_percentage ? 130 : 40, 46, name, permanentMarker, front);
  // underline:
  gdispDrawLine(show_percentage ? 130 : 40, 72,
                (show_percentage ? 130 : 40) + gdispGetStringWidth(name, permanentMarker) + 14, 72,
                front);
  // cursor:
  gdispDrawLine((show_percentage ? 130 : 40) + gdispGetStringWidth(name, permanentMarker) + 10, 50 + 2,
                (show_percentage ? 130 : 40) + gdispGetStringWidth(name, permanentMarker) + 10, 50 + 22 - 2,
                front);
  gdispDrawString(140, 75, "Anyway", robotoBlackItalic, front);
  gdispFlush();
}
