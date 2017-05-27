#include "gfx.h"
#include "demo_ugfx.h"

void demoUgfx() {
  ets_printf("Initializing gfx\n");
  gfxInit();
  ets_printf("Initialized gfx\n");
  while (1) {
    ets_printf("Clearing white\n");
    gdispClear(White);
    ets_delay_us(5000000);
    ets_printf("Clearing black\n");
    gdispClear(Black);
    ets_delay_us(5000000);
  }
}
