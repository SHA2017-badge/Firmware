#ifndef POWERDOWN_H
#define POWERDOWN_H

#define POWERDOWN_DBG 1

typedef enum {
	POWER_MODE_BPP=0,
	POWER_MODE_UPY,
	POWER_MODE_MAX
} PowerMode;

typedef void (PowerDownCb)(int delayMs, void *arg, PowerMode newmode);

void powerDownMgrInit(PowerDownCb *cb, void *arg, PowerMode mode);

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