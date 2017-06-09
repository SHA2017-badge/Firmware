#include "gfx.h"
#include "demo_ugfx.h"
#include "badge_input.h"
#include "ginput/ginput_driver_toggle.h"

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

GEventToggle ptoggle;

void showDemo(uint8_t name, color_t front, color_t back) {
  // TODO trigger this only when we have reason to believe a button was toggled
  ets_printf("Simulating button interrupt\n");
  ginputToggleWakeup();

  ets_printf("Getting toggle status\n");
  if (!ginputGetToggleStatus(BADGE_BUTTON_UP, &ptoggle)) {
    ets_printf("Error getting toggle status\n");
  } else {
      if (ptoggle.on) {
        ets_printf("button was pressed\n");
      } else {
        ets_printf("button was not pressed\n");
      }
  }

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

  GSourceHandle downHandle = ginputGetToggle(BADGE_BUTTON_UP);

  roboto = gdispOpenFont("Roboto_Regular12");
  robotoBlackItalic = gdispOpenFont("Roboto_BlackItalic24");
  permanentMarker = gdispOpenFont("PermanentMarker22");

  ets_printf("Clearing\n");

  gdispClear(Black);
  gdispFlush();

  while (1) {
    uint8_t i;
    for (i = 0; i < numNames; i++) {
      ets_printf("Make it White\n");
      showDemo(i, Black, White);
      ets_printf("Take 5\n");
      ets_delay_us(5000000);

      ets_printf("Paint it Black\n");
      showDemo(i, White, Black);
      ets_printf("Take 5\n");
      ets_delay_us(5000000);
    }
  }
}
