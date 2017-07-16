/*
Trivial blkidcache that does zero caching at all. Use for testing or on backing that does not
have issues reading/writing data often.

WARNING: Deprecated. If anything, does not honour bdif->notifyComplete.
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "blkidcache.h"

struct BlkIdCacheHandle {
	BlockdevifHandle *blkdev;
	BlockdevIf *bdif;
};

BlkIdCacheHandle *idcacheCreate(int size, BlockdevifHandle *blkdev, BlockdevIf *bdif) {
	BlkIdCacheHandle *ret=malloc(sizeof(BlkIdCacheHandle));
	ret->blkdev=blkdev;
	ret->bdif=bdif;
	return ret;
}

void idcacheSet(BlkIdCacheHandle *h, int block, uint32_t id) {
	h->bdif->setChangeID(h->blkdev, block, id);
}

uint32_t idcacheGet(BlkIdCacheHandle *h, int block) {
	return h->bdif->getChangeID(h->blkdev, block);
}

