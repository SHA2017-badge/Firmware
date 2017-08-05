#ifndef POWERDOWN_H
#define POWERDOWN_H

#define POWERDOWN_DBG 1

typedef enum {
	POWER_MODE_BPP=0,
	POWER_MODE_UPY,
} PowerMode;

#define NO_POWER_MODES 2


/*
This callback is called when:
- All processes active have released their power hold. delayMs is the time the ESP can sleep, and
  newMode is the mode it should wake up in.
- A higher-priority mode deep sleep timer passed. We need to reboot into that mode immediately. In
  this case, newMode is different from the current mode and delayMs = 0.
*/
typedef void (PowerDownCb)(int delayMs, void *arg, PowerMode newmode);

/*
Initialize the power down managed. Cb is a callback for when the ESP32 needs to power down or reboot,
mode is the power mode we're currently in, dbg is 1 if it's okay to spit debug info to the console.
Arg is passed straight to the callback.
*/
void powerDownMgrInit(PowerDownCb *cb, void *arg, PowerMode mode, bool dbg);


/*
Each process that can keep the esp32 from sleeping has a reference, which is a 32-bit integer called 'ref'. This
ref can be randomly chosen, for example it can be the address of an object the process is bound to. As long as the
ref is the same over calls, the powerdown manager knows the calls are done by the same process.
- powerHold(ref, holdTimeMs) - Stop the ESP32 from going to sleep for at least holdTimeMs milliseconds.
- powerCanSleepFor(ref, delayMs) - ESP32 is allowed to go to sleep, but should wake up in at least delayMs milliseconds.
- powerCanSleep(ref) - ESP32 is allowed to go to sleep, period.
*/

#if POWERDOWN_DBG
#define powerHold(ref, ht) _powerHold(ref, ht, __FUNCTION__, __LINE__)
#define powerCanSleepFor(ref, del) _powerCanSleepFor(ref, del, __FUNCTION__, __LINE__)
#define powerCanSleep(ref) _powerCanSleep(ref, __FUNCTION__, __LINE__)
#else
#define powerHold(ref) _powerHold(ref, "", 0)
#define powerCanSleepFor(ref, del) _powerCanSleepFor(ref, del, "", 0)
#define powerCanSleep(ref) _powerCanSleep(ref, "", 0)
#endif
void _powerHold(int ref, int holdTimeMs, const char *fn, const int line);
void _powerCanSleepFor(int ref, int delayMs, const char *fn, const int line);
void _powerCanSleep(int ref, const char *fn, const int line);


#endif