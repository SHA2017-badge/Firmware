#include "sdkconfig.h"

#ifdef CONFIG_SHA_BADGE_EINK_DEPG0290B1

#include <stdbool.h>
#include <stdint.h>

#include "gde.h"
#include "gde-driver.h"

// DEPG0290B01

void initDisplay(void) {
  // Hardware reset
  gdeReset();
  gdeBusyWait();

  // Software reset
  gdeWriteCommand(0x12);
  gdeBusyWait();

  // Set analog block control
  gdeWriteCommand_p1(0x74, 0x54);

  // Set digital block control
  gdeWriteCommand_p1(0x7E, 0x3B);

  // Set display size and driver output control
  gdeWriteCommand_p3(0x01, 0x27, 0x01, 0x00);

  // Ram data entry mode
  // Adress counter is updated in Y direction, Y increment, X increment
  gdeWriteCommand_p1(0x11, 0x03);

  // Set RAM X address (00h to 0Fh)
  gdeWriteCommand_p2(0x44, 0x00, 0x0F);

  // Set RAM Y address (0127h to 0000h)
  gdeWriteCommand_p4(0x45, 0x00, 0x00, 0x27, 0x01);

  // Set border waveform for VBD (see datasheet)
  gdeWriteCommand_p1(0x3C, 0x01);

  // SET VOLTAGE

  // Set VCOM value
  gdeWriteCommand_p1(0x2C, 0x26);

  // Gate voltage setting (17h = 20 Volt, ranges from 10v to 21v)
  gdeWriteCommand_p1(0x03, 0x17);

  // Source voltage setting (15volt, 0 volt and -15 volt)
  gdeWriteCommand_p3(0x04, 0x41, 0x00, 0x32);
}

#endif // CONFIG_SHA_BADGE_EINK_DEPG0290B1
