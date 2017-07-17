#ifndef BD_FLATFLASH_H
#define BD_FLATFLASH_H

#include "blockdevif.h"

extern BlockdevIf blockdevIfFlatFlash;

typedef void (*BlockdevIfFlatFlashDoneCb)(uint32_t changeid, void *arg);

typedef struct {
	int major;
	int minor;
	uint32_t minChangeId;
	BlockdevIfFlatFlashDoneCb doneCb;
	void *doneCbArg;
} BlockdevIfFlatFlashDesc;

#endif