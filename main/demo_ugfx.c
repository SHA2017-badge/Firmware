#include "gfx.h"
#include "demo_ugfx.h"

font_t roboto;
font_t permanentMarker;

void showDemo(color_t front, color_t back) {
  gdispClear(back);
  gdispDrawString(150, 80, "Hello, ugfx world", roboto, front);
  gdispDrawString(150, 40, "Badge Hack!", permanentMarker, front);
  gdispDrawCircle(50, 50, 50, front);
  gdispFlush();
}

void demoUgfx() {
  ets_printf("Initializing gfx\n");
  gfxInit();
  ets_printf("Initialized gfx\n");

  roboto = gdispOpenFont("Roboto_Regular12");
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
