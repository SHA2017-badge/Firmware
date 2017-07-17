#ifndef BD_ROPART_H
#define BD_ROPART_H

#include "blockdevif.h"

extern BlockdevIf blockdevIfRoPart;


typedef struct {
	int major;
	int minor;
} BlockdevIfRoPartDesc;

void bdropartDumpJournal(BlockdevifHandle *h);


#endif