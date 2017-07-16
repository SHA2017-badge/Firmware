#include <stdio.h>
#include "hldemux.h"

static void subtitleRecv(int subtype, uint8_t *data, int len, void *arg) {
	data[len]=0;
	printf("Subtitle stream %d: %s\n", subtype, data);
}

void subtitleInit() {
//	hldemuxAddType(HLPACKET_TYPE_SUBTITLES, subtitleRecv, NULL);
	hldemuxAddType(2, subtitleRecv, NULL);
}

