/*
Blockdev iface for a flat flash file.

Meant to do e.g. firmware updates where it's important to have the data in one sequential
run. Disadvantage is that incremental updates are impossible. Needs an id cache because
it only saves the last change id and the bitmap of sectors that have that ID.

*/
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "esp_partition.h"
#include "structs.h"
#include "blockdevif.h"
#include "bd_flatflash.h"
#include "esp_system.h"

typedef struct {
	uint32_t changeId; //current most recent changeid
	uint8_t bitmap[]; //Warning: inverted: 1 means sector does NOT have changeId.
} FlashMgmtSector;


struct BlockdevifHandle {
	int size;	//in blocks
	const esp_partition_t *part;
	FlashMgmtSector *msec;
	BlockdevIfFlatFlashDoneCb doneCb;
	void *doneCbArg;
};


static void setNewChangeId(BlockdevifHandle *handle, uint32_t changeId) {
	handle->msec->changeId=changeId;
	for (int i=0; i<handle->size/8; i++) handle->msec->bitmap[i]=0xff;
	esp_partition_erase_range(handle->part, handle->size*BLOCKDEV_BLKSZ, BLOCKDEV_BLKSZ);
}

static void flushMgmtSector(BlockdevifHandle *handle) {
//	printf("bd_flatflash: writing mgmt sector\n");
	esp_partition_write(handle->part, handle->size*BLOCKDEV_BLKSZ, handle->msec, sizeof(FlashMgmtSector)+(handle->size/8));
}

static BlockdevifHandle *blockdevifInit(void *desc, int size) {
	BlockdevIfFlatFlashDesc *bdesc=(BlockdevIfFlatFlashDesc*)desc;
	BlockdevifHandle *h=malloc(sizeof(BlockdevifHandle));
	if (h==NULL) goto error1;
	h->part=esp_partition_find_first(bdesc->major, bdesc->minor, NULL);
	if (h->part==NULL) goto error2;
	
	h->size=size/BLOCKDEV_BLKSZ;
	h->msec=malloc((h->size/8)+sizeof(FlashMgmtSector));
	if (h->msec==NULL) goto error2;

	h->msec->changeId=0;
	h->doneCb=bdesc->doneCb;
	h->doneCbArg=bdesc->doneCbArg;

	if (h->part->size < size+BLOCKDEV_BLKSZ) {
		printf("bd_flatflash: Part 0x%X-0x%X is %d bytes. Need %d bytes.\n", bdesc->major, bdesc->minor, h->part->size, size+BLOCKDEV_BLKSZ);
		goto error3;
	}

	//Read in management data
	esp_partition_read(h->part, h->size*BLOCKDEV_BLKSZ, h->msec, sizeof(FlashMgmtSector)+(h->size/8));

	//Pin erased mgmt block to change id 0
	if (h->msec->changeId==0xFFFFFFFF) {
		h->msec->changeId=0;
	}

	//If not at minChangeId, make sure it is.
	if (h->msec->changeId <= bdesc->minChangeId) {
		h->msec->changeId = bdesc->minChangeId;
		for (int i=0; i < h->size/8; i++) h->msec->bitmap[i]=0;
		flushMgmtSector(h);
	}

	return h;

error3:
	free(h->msec);
error2:
	free(h);
error1:
	return NULL;
}

static void blockdevifSetChangeID(BlockdevifHandle *handle, int sector, uint32_t changeId) {
	if (changeId > handle->msec->changeId) setNewChangeId(handle, changeId);
	if (changeId == handle->msec->changeId && (handle->msec->bitmap[sector/8] & (1<<(sector&7)))) {
		//Sector indeed is updated now. Clear bit
		handle->msec->bitmap[sector/8] &= ~(1<<(sector&7));
		flushMgmtSector(handle);

		//ToDo: Is a done callback here really the place to do it?
		if (handle->doneCb) {
			//Check if we're done; call callback if so.
			int allDone=1;
			for (int i=0; i<handle->size/8; i++) {
				if (handle->msec->bitmap[i]!=0) {
					allDone=0;
					break;
				}
			}
			if (allDone) handle->doneCb(changeId, handle->doneCbArg);
		}
	}
}

static uint32_t blockdevifGetChangeID(BlockdevifHandle *handle, int sector) {
	if (handle->msec->bitmap[sector/8] & (1<<(sector&7))) {
		return 0;
	} else {
		return handle->msec->changeId;
	}
}

static int blockdevifGetSectorData(BlockdevifHandle *handle, int sector, uint8_t *buff) {
	esp_err_t r=esp_partition_read(handle->part, sector*BLOCKDEV_BLKSZ, buff, BLOCKDEV_BLKSZ);
	return (r==ESP_OK);
}

static SetSectorDataRetVal blockdevifSetSectorData(BlockdevifHandle *handle, int sector, uint8_t *buff, uint32_t adv_id) {
	if (sector>=handle->size) printf("Huh? Trying to write sector %d\n", sector);

//	printf("Writing %p to block %d of size %d...\n", buff, sector, BLOCKDEV_BLKSZ);

	uint32_t start=system_get_time();
	esp_partition_erase_range(handle->part, sector*BLOCKDEV_BLKSZ, BLOCKDEV_BLKSZ);
	uint32_t durer=system_get_time()-start;
	start=system_get_time();
	esp_partition_write(handle->part, sector*BLOCKDEV_BLKSZ, buff, BLOCKDEV_BLKSZ);
	uint32_t dur=system_get_time()-start;

//	printf("Block write: took %d msec writing %d erasing\n", dur/1000, durer/1000);
	return SSDR_SET; //Data set, ID ignored.
}

static void blockdevifForEachBlock(BlockdevifHandle *handle, BlockdevifForEachBlockFn *cb, void *arg) {
	for (int i=0; i<handle->size; i++) {
		uint32_t chid=blockdevifGetChangeID(handle, i);
		cb(i, chid, arg);
	}
}

BlockdevIf blockdevIfFlatFlash={
	.init=blockdevifInit,
	.setChangeID=blockdevifSetChangeID,
	.getChangeID=blockdevifGetChangeID,
	.getSectorData=blockdevifGetSectorData,
	.setSectorData=blockdevifSetSectorData,
	.forEachBlock=blockdevifForEachBlock
};


