#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "powerdown.h"

typedef struct PowerItem PowerItem;
static PowerDownCb *pdCb=NULL;
static void *pdCbArg=NULL;

#define ST_ACTIVE 0
#define ST_CANSLEEP 1
#define ST_CANSLEEP_UNTIL 2

struct PowerItem {
	int ref;
	int state;
	struct timeval sleepUntil;
	PowerItem *next;
};

static PowerItem *powerItems;

static PowerItem *findItem(int ref) {
	for (PowerItem *i=powerItems; i!=NULL; i=i->next) {
		if (i->ref==ref) return i;
	}
	//Not found. Add item.
	PowerItem *p=malloc(sizeof(PowerItem));
	memset(p, 0, sizeof(PowerItem));
	p->ref=ref;
	p->next=powerItems;
	powerItems=p;
	return p;
}


static void doSleep(int sleepMs) {
	if (pdCb) pdCb(sleepMs, pdCbArg);
}

static void checkCanSleep() {
	PowerItem *i=powerItems;
	int canSleepForMs=-1;
	int cannotSleep=0;
	while (i!=NULL) {
		if (i->state==ST_ACTIVE) {
			printf("Power: Ref %x: active\n", i->ref);
			cannotSleep=1;
		} else if (i->state==ST_CANSLEEP_UNTIL) {
			struct timeval now;
			gettimeofday(&now, NULL);
			int mssleep=(i->sleepUntil.tv_sec-now.tv_sec)*1000;
			mssleep+=((i->sleepUntil.tv_usec-now.tv_usec)/1000);
			if (mssleep<2000) {
				//Sleep req is too short.
				//We're going to ignore this sleep request, and make the thing active again.
					printf("Power: Ref %x: can sleep for %d ms. Too short, making active again.\n", i->ref, mssleep);
					i->state=ST_ACTIVE;
					cannotSleep=1;
			} else {
				printf("Power: Ref %x: can sleep for %d ms\n", i->ref, mssleep);
				if (canSleepForMs==-1 || mssleep<canSleepForMs) {
					canSleepForMs=mssleep;
				}
			}
		} else if (i->state==ST_CANSLEEP) {
//			printf("Power: Ref %x: can sleep.\n", i->ref);
			//Erm, nothing to check, thing can sleep.
		}
		i=i->next;
	}
	//If we're here, we can sleep.
	if (!cannotSleep) doSleep(canSleepForMs);
}

void powerHold(int ref) {
	PowerItem *p=findItem(ref);
	p->state=ST_ACTIVE;
}

void powerCanSleepFor(int ref, int delayMs) {
//	printf("canSleepFor %d\n", delayMs);
	PowerItem *p=findItem(ref);
	gettimeofday(&p->sleepUntil, NULL);
	p->sleepUntil.tv_sec+=delayMs/1000;
	p->sleepUntil.tv_usec+=(delayMs%1000)*1000;
	if (p->sleepUntil.tv_usec>1000000) {
		p->sleepUntil.tv_usec-=1000000;
		p->sleepUntil.tv_sec++;
	}
	p->state=ST_CANSLEEP_UNTIL;
	checkCanSleep();
}

void powerCanSleep(int ref) {
	PowerItem *p=findItem(ref);
	p->state=ST_CANSLEEP;
	checkCanSleep();
}

void powerDownMgrInit(PowerDownCb *cb, void *arg) {
	pdCb=cb;
	pdCbArg=arg;
}
