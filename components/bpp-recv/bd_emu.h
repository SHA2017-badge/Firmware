#ifndef BD_EMU_H
#define BD_EMU_H


#include "blockdevif.h"

extern BlockdevIf blockdevIfBdemu;


typedef struct {
	const char *file;
} BlockdevIfBdemuDesc;
#endif