#ifndef POWERDOWN_H
#define POWERDOWN_H

typedef void (PowerDownCb)(int delayMs, void *arg);

void powerDownMgrInit(PowerDownCb *cb, void *arg);
void powerHold(int ref);
void powerCanSleepFor(int ref, int delayMs);
void powerCanSleep(int ref);

#endif