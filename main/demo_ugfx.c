#include "gfx.h"
#include "demo_ugfx.h"

font_t roboto;
font_t robotoBlackItalic;
font_t permanentMarker;

void showDemo(color_t front, color_t back) {
  gdispClear(back);
  gdispDrawString(150, 25, "STILL", robotoBlackItalic, front);
  gdispDrawString(130, 50, "HACKING", permanentMarker, front);
  // underline:
  gdispDrawLine(127 + 3, 50 + 22,
                127 + 3 + gdispGetStringWidth("HACKING", permanentMarker) + 14, 50 + 22,
                front);
  // cursor:
  gdispDrawLine(127 + 3 + gdispGetStringWidth("HACKING", permanentMarker) + 10, 50 + 2,
                127 + 3 + gdispGetStringWidth("HACKING", permanentMarker) + 10, 50 + 22 - 2,
                front);
  gdispDrawString(140, 75, "Anyway", robotoBlackItalic, front);
  gdispDrawCircle(60, 60, 50, front);
  gdispFlush();
}

void demoUgfx() {
  ets_printf("Initializing gfx\n");
  gfxInit();
  ets_printf("Initialized gfx\n");

  roboto = gdispOpenFont("Roboto_Regular12");
  robotoBlackItalic = gdispOpenFont("Roboto_BlackItalic24");
  permanentMarker = gdispOpenFont("PermanentMarker22");

  while (1) {
    ets_printf("Make it White\n");
    showDemo(Black, White);
    ets_delay_us(5000000);

    ets_printf("Paint it Black\n");
    showDemo(White, Black);
    ets_delay_us(5000000);
  }
}
