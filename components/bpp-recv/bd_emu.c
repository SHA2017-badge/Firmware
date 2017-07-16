/*
Host-side blockdev emulation. 

*/
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "structs.h"
#include "blockdevif.h"
#include "bd_emu.h"

struct BlockdevifHandle {
	int size;
	int bdFd, idFd;
	uint8_t *bdData;
	uint32_t *idData;
};

static BlockdevifHandle *blockdevifInit(void *desc, int size) {
	BlockdevIfBdemuDesc *bdesc=(BlockdevIfBdemuDesc*)desc;
	char buf[1024];
	BlockdevifHandle *h=malloc(sizeof(BlockdevifHandle));
	h->size=size;
	h->bdFd=open(bdesc->file, O_RDWR|O_CREAT, 0644);
	if (h->bdFd<=0){
		printf("opening bdev:\n");
		perror(bdesc->file);
		goto error1;
	}
	ftruncate(h->bdFd, size);
	sprintf(buf, "%s.ids", bdesc->file);
	h->idFd=open(buf, O_RDWR|O_CREAT, 0644);
	if (h->idFd<=0){
		printf("opening bdev idfile:\n");
		perror(buf);
		goto error2;
	}
	ftruncate(h->idFd, (size/BLOCKDEV_BLKSZ)*sizeof(uint32_t));
	h->bdData=mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, h->bdFd, 0);
	if (h->bdData==MAP_FAILED) {
		printf("mmap bdev:\n");
		perror(bdesc->file);
		goto error3;
	}
	h->idData=mmap(NULL, (size/BLOCKDEV_BLKSZ)*sizeof(uint32_t), PROT_READ|PROT_WRITE, MAP_SHARED, h->idFd, 0);
	if (h->idData==MAP_FAILED) {
		printf("mmap bdev idfile:\n");
		perror(buf);
		goto error4;
	}
	return h;

error4:
	munmap(h->bdData, size);
error3:
	close(h->idFd);
error2:
	close(h->bdFd);
error1:
	free(h);
	return NULL;
}

static void blockdevifSetChangeID(BlockdevifHandle *handle, int sector, uint32_t changeId) {
	assert(handle && sector>=0 && sector<(handle->size/BLOCKDEV_BLKSZ));
	handle->idData[sector]=changeId;
}

static uint32_t blockdevifGetChangeID(BlockdevifHandle *handle, int sector) {
	assert(handle && sector>=0 && sector<(handle->size/BLOCKDEV_BLKSZ));
	return handle->idData[sector];
}

static int blockdevifGetSectorData(BlockdevifHandle *handle, int sector, uint8_t *buff) {
	assert(handle && sector>=0 && sector<(handle->size/BLOCKDEV_BLKSZ));
	memcpy(buff, &handle->bdData[sector*BLOCKDEV_BLKSZ], BLOCKDEV_BLKSZ);
}

static int blockdevifSetSectorData(BlockdevifHandle *handle, int sector, uint8_t *buff, uint32_t changeId) {
	assert(handle && sector>=0 && sector<(handle->size/BLOCKDEV_BLKSZ));
	memcpy(&handle->bdData[sector*BLOCKDEV_BLKSZ], buff, BLOCKDEV_BLKSZ);
	blockdevifSetChangeID(handle, sector, changeId);
}

static void blockdevifForEachBlock(BlockdevifHandle *handle, BlockdevifForEachBlockFn *cb, void *arg) {
	for (int i=0; i<(handle->size/BLOCKDEV_BLKSZ); i++) {
		cb(i, handle->idData[i], arg);
	}
}


BlockdevIf blockdevIfBdemu={
	.init=blockdevifInit,
	.setChangeID=blockdevifSetChangeID,
	.getChangeID=blockdevifGetChangeID,
	.getSectorData=blockdevifGetSectorData,
	.setSectorData=blockdevifSetSectorData,
	.forEachBlock=blockdevifForEachBlock
};


