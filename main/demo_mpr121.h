#ifndef DEMO_MPR121_H
#define DEMO_MPR121_H

#include "sdkconfig.h"

#include <badge_pins.h>
#ifdef I2C_MPR121_ADDR
extern void demoMpr121(void);
#endif // I2C_MPR121_ADDR

#endif // DEMO_MPR121_H
