#include "gfx.h"
#include "demo_ugfx.h"

font_t roboto;
font_t robotoBlackItalic;
font_t permanentMarker;

const char* displayNames[] = {
    "Kartoffel",
    "Sebastius",
    "Boekenwuurm",
    "Attilla",
    "Stitch",
    "tsd",
    "Sprite_TM",
    "Underhand",
    "MarkusBec",
    "Roosted",
    "the_JinX",
    "realitygaps",
    "raboof"
};

#define numNames (sizeof(displayNames) / sizeof (const char*))

void showDemo(uint8_t name, color_t front, color_t back) {
  gdispClear(back);
  gdispDrawString(150, 25, "STILL", robotoBlackItalic, front);
  gdispDrawString(130, 50, displayNames[name], permanentMarker, front);
  // underline:
  gdispDrawLine(127 + 3, 50 + 22,
                127 + 3 + gdispGetStringWidth(displayNames[name], permanentMarker) + 14, 50 + 22,
                front);
  // cursor:
  gdispDrawLine(127 + 3 + gdispGetStringWidth(displayNames[name], permanentMarker) + 10, 50 + 2,
                127 + 3 + gdispGetStringWidth(displayNames[name], permanentMarker) + 10, 50 + 22 - 2,
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

  gdispClear(Black);
  gdispFlush();

  while (1) {
    uint8_t i;
    for (i = 0; i < numNames; i++) {
      ets_printf("Make it White\n");
      showDemo(i, Black, White);
      ets_delay_us(5000000);

      ets_printf("Paint it Black\n");
      showDemo(i, White, Black);
      ets_delay_us(5000000);
    }
  }
}
