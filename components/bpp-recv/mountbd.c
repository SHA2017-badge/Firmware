#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include "structs.h"
#include "hldemux.h"
#include "blockdevif.h"
#include "blockdecode.h"
#include "mountbd.h"
#include "diskio.h"
#include "esp_vfs_fat.h"


#define BD_FAT_SECTOR_SZ 512

typedef struct {
	uint8_t drive;
	BlockdevifHandle *handle;
	BlockdevIf *iface;
} DriveToBd;

#define NO_BD 4
static DriveToBd driveToBd[NO_BD]; 


static DSTATUS bdvfat_init(BYTE pdrv) {
	//ToDo: If we're here, the path is actually used. Maybe stop BPP and/or snapshot here.
//	printf("bdvfat_init\n");
	return STA_PROTECT;
}

static DSTATUS bdvfat_status(BYTE pdrv) {
//	printf("bdvfat_status\n");
	return STA_PROTECT;
}

static DRESULT bdvfat_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count) {
	//Find handle
//	printf("bdvfat_read: pdrv %d sect %d len %d\n", (int)pdrv, (int)sector, (int)count);
	BlockdevifHandle *h=NULL;
	BlockdevIf *iface=NULL;
	for (int i=0; i<NO_BD; i++) {
		if (driveToBd[i].handle!=NULL && driveToBd[i].drive==pdrv) {
			h=driveToBd[i].handle;
			iface=driveToBd[i].iface;
			break;
		}
	}
	if (!h) return RES_ERROR;

	uint8_t *bbuf=malloc(BLOCKDEV_BLKSZ);
	if (!bbuf) return RES_ERROR;
	int bufBlk=-1;
	int r=1;
	for (int i=0; i<count; i++) {
		int blk=(sector+i)/(BLOCKDEV_BLKSZ/BD_FAT_SECTOR_SZ);
		int ssect=(sector+i)%(BLOCKDEV_BLKSZ/BD_FAT_SECTOR_SZ);
		if (blk!=bufBlk) {
			r=iface->getSectorData(h, blk, bbuf);
			if (!r) break;
			bufBlk=blk;
		}
		memcpy(&buff[BD_FAT_SECTOR_SZ*i], &bbuf[BD_FAT_SECTOR_SZ*ssect], BD_FAT_SECTOR_SZ);
	}
	free(bbuf);
	return r?RES_OK:RES_ERROR;
}

static DRESULT bdvfat_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count) {
//	printf("bdvfat_write\n");
	return RES_WRPRT;
}

static DRESULT bdvfat_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
//	printf("bdvfat_ioctl %d\n",(int)cmd);
	if (cmd==GET_SECTOR_SIZE) {
		*((uint32_t*) buff)=BD_FAT_SECTOR_SZ;
		return RES_OK;
	} else {
		printf("bdvfat_ioctl: Unknown ioctl %d\n", (int)cmd);
	}
	return RES_PARERR;
}



static const ff_diskio_impl_t bdRopartVfatDiskioimpl={
	.init=bdvfat_init,
	.status=bdvfat_status,
	.read=bdvfat_read,
	.write=bdvfat_write,
	.ioctl=bdvfat_ioctl,
};

//ToDo: better error handling (free/unregister stuff)
int bd_mount(BlockdevIf *iface, BlockdevifHandle *h, const char *path, size_t max_files) {
	FATFS *ret=malloc(sizeof (FATFS));
	FRESULT fr;
	esp_err_t er;
	BYTE pdrv = 0xFF;

	int i;
	for (i=0; i<NO_BD; i++) {
		if (driveToBd[i].handle==NULL) break;
	}
	if (i==NO_BD) return 0;

	//Grab available pdrv
	if (ff_diskio_get_drive(&pdrv) != ESP_OK) {
		return 0;
	}
	driveToBd[i].drive=pdrv;
	driveToBd[i].handle=h;
	driveToBd[i].iface=iface;

	//Create logical drive path
	char drv[3]={'0'+pdrv, ':', 0};
	printf("bd_mount: using slot %d, pdrv %d (%s)\n", i, (int)pdrv, drv);
	er=esp_vfs_fat_register(path, drv, max_files, &ret);
	if (er!=ESP_OK) {
		printf("bd_mount: esp_vfs_fat_register failed: %x\n", er);
		return 0;
	}
	ff_diskio_register(pdrv, &bdRopartVfatDiskioimpl);
	fr=f_mount(ret, drv, 1);
	if (fr!=FR_OK) {
		printf("bd_mount: f_mount failed: %x\n", fr);
		return 0;
	}
	
	return 1;
}

