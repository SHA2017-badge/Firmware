/*
Multi-level block cache. Keeps a few changeids in memory, plus the sectors which have
that changeid. Delegates to the underlying block device for everything else.
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "blkidcache.h"
#include "bma.h"

#define LEVELS 5

static void idcacheSetInt(BlkIdCacheHandle *h, int block, uint32_t id, int writeback);


struct BlkIdCacheHandle {
	BlockdevifHandle *blkdev;
	Bma *bmp[LEVELS];
	uint32_t id[LEVELS];
	int size;			//in blocks
	BlockdevIf *bdif;
	Bma *chidFlushed;
};

//called for each block when cache is created
static void initCache(int blockno, uint32_t changeId, void *arg) {
	BlkIdCacheHandle *h=(BlkIdCacheHandle*)arg;
	idcacheSetInt(h, blockno, changeId, 0);
}

BlkIdCacheHandle *idcacheCreate(int size, BlockdevifHandle *blkdev, BlockdevIf *bdif) {
	BlkIdCacheHandle *ret=malloc(sizeof(BlkIdCacheHandle));
	ret->blkdev=blkdev;
	ret->size=size;
	ret->bdif=bdif;
	for (int i=0; i<LEVELS; i++) {
		ret->bmp[i]=bmaCreate(size);
		ret->id[i]=0;
	}
	ret->chidFlushed=bmaCreate(size);
	bmaSetAll(ret->chidFlushed, 1);
	printf("blkidcache_mlvl: reading all block IDs\n");
	bdif->forEachBlock(blkdev, initCache, ret);
	printf("blkidcache_mlvl: initialized\n");
	return ret;
}

void idcacheFlushToStorage(BlkIdCacheHandle *h) {
	printf("Flushing idcache to storage.\n");
	int flushed=0;
	for (int bl=0; bl<h->size; bl++) {
		if (!bmaIsSet(h->chidFlushed, bl)) {
//			printf("Bl %d flushable\n", bl);
			for (int lvl=0; lvl<LEVELS; lvl++) {
				if (bmaIsSet(h->bmp[lvl], bl)) {
					h->bdif->setChangeID(h->blkdev, bl, h->id[lvl]);
					flushed++;
					break; //level for loop
				}
			}
		}
	}
	printf("Flushed %d descs.\n", flushed);
	bmaSetAll(h->chidFlushed, 1);
}

void idcacheSet(BlkIdCacheHandle *h, int block, uint32_t id) {
	idcacheSetInt(h, block, id, 1);
}

//Writeback indicates if the change eventually needs to be written back to the backing
//storage. 0 means it comes from there, so no need to write back.
static void idcacheSetInt(BlkIdCacheHandle *h, int block, uint32_t id, int writeback) {
	//Kill bit in all levels but the one that has the same id. Also check if the id may
	//be newer than anything we have.
	int isSet=-1; //contains the level bit was set in, or -1 if not set
	for (int lvl=0; lvl<LEVELS; lvl++) {
		if (id != h->id[lvl]) {
			//Not in this level. Clear bit here.
			bmaSet(h->bmp[lvl], block, 0);
		} else {
			//Set it in the level that corresponds to this ID
			if (!bmaIsSet(h->bmp[lvl], block)) {
				bmaSet(h->bmp[lvl], block, 1);
				//Only recorded in levels, not in backing storage.
				if (writeback) {
					bmaSet(h->chidFlushed, block, 0);
//					printf("Marked flushable %d\n", block);
				}
//			} else {
//				printf("Already set %d\n", block);
			}
			isSet=lvl;
		}
	}

	if (isSet==-1) {
		//No level to store this. See if the ID is newer than any of the IDs we have already; if so we can kick
		//out the oldest and add this one.
		int oldest=0;
		for (int lvl=1; lvl<LEVELS; lvl++) {
			if (h->id[lvl] < h->id[oldest]) oldest=lvl;
		}
		if (h->id[oldest] < id) {
			printf("blkidcache_mlvl: Level %d: clearing out old ID %d for new id %d\n", oldest, h->id[oldest], id);
			//Flush out level to disk
			for (int blk=0; blk<h->size; blk++) {
				if (bmaIsSet(h->bmp[oldest], blk) && (!bmaIsSet(h->chidFlushed, blk))) {
					h->bdif->setChangeID(h->blkdev, blk, h->id[blk]);
//					printf("Flushed %d\n", blk);
					bmaSet(h->chidFlushed, blk, 1);
				}
			}
			//Re-use level for current one
			h->id[oldest]=id;
			bmaSetAll(h->bmp[oldest], 0);
			bmaSet(h->bmp[oldest], block, 1);
			if (writeback) {
				bmaSet(h->chidFlushed, block, 0);
//				printf("Marked flushable %d\n", block);
			}
			isSet=oldest;
		}
	}

	if (isSet==-1) {
		//Can't handle this in cache. Write back if needed.
		printf("blkidcache_mlvl: no place for id %d\n", id);
		if (writeback) {
			h->bdif->setChangeID(h->blkdev, block, id);
			bmaSet(h->chidFlushed, block, 1);
//			printf("Flushed %d\n", block);
		}
	}


	if (isSet!=-1 && h->bdif->notifyComplete) {
//		printf("Cache: lvl %d chid %d: ", isSet, h->id[isSet]);
//		bmaDump(h->bmp[isSet]);
		if (bmaIsAllSet(h->bmp[isSet])) {
			h->bdif->notifyComplete(h->blkdev, id);
		}
	}

	//printf("Cache: blk %d put in lvl %d chid %d\n ", block, isSet, id);

}

void idcacheSetSectorData(BlkIdCacheHandle *h, int block, uint8_t *data, uint32_t id) {
	SetSectorDataRetVal mr=h->bdif->setSectorData(h->blkdev, block, data, id);
	
	if (mr==SSDR_SETCHID) {
		bmaSet(h->chidFlushed, block, 1);
		idcacheSetInt(h, block, id, 0);
	} else {
		idcacheSetInt(h, block, id, 1);
	}

	static int setCtr=0;
	setCtr++;
	if (setCtr>=50) {
		idcacheFlushToStorage(h);
		setCtr=0;
	}
}

uint32_t idcacheGetLastChangeId(BlkIdCacheHandle *h) {
	uint32_t ret=0;
	for (int i=0; i<LEVELS; i++) {
		if (h->id[i] > ret) ret=h->id[i];
	}
	return ret;
}



uint32_t idcacheGet(BlkIdCacheHandle *h, int block) {
	for (int i=0; i<LEVELS; i++) {
		if (bmaIsSet(h->bmp[i], block)) {
//			printf("Got cached id %d for block %d\n", h->id[i], block);
			return h->id[i];
		}
	}
	printf("Blk %d not in cache.\n", block);
	uint32_t id=h->bdif->getChangeID(h->blkdev, block);
	return id;
}



