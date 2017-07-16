#ifndef BLOCKDEVIF_H
#define BLOCKDEVIF_H

//Interface to an abstracted block device. Should have the capability of storing both sector change IDs 
//as well as sector data.

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	SSDR_ERROR=0,	//some error happened
	SSDR_SET,		//data is set
	SSDR_SETCHID	//data + changeId is set
} SetSectorDataRetVal;

typedef struct BlockdevifHandle BlockdevifHandle;

typedef void (BlockdevifForEachBlockFn)(int blockno, uint32_t changeId, void *arg);

typedef struct {
	//Initialize the block device. Size is the virtual size of the file, which may be larger
	//than the physical size of the storage medium.
	BlockdevifHandle* (*init)(void *desc, int size);
	//Update a sector to a new changeId. Contents stay the same.
	void (*setChangeID)(BlockdevifHandle *handle, int sector, uint32_t changeId);
	//Get the changeid for a sector
	uint32_t (*getChangeID)(BlockdevifHandle *handle, int sector);
	//Set the data for a sector. adv_id is advisory changeid and doesn't need to be taken into account, but
	//can be used if setting the changeId is 'free'.
	SetSectorDataRetVal (*setSectorData)(BlockdevifHandle *handle, int sector, uint8_t *buff, uint32_t adv_id);
	//Get the data for a sector. WARNING: If the block device supports it, the block device returns
	//the data that was current when notifyComplete was called, not the last written one. This allows
	//the block device to return an integral snapshot, not whatever was halfway throiugh an update.
	int (*getSectorData)(BlockdevifHandle *handle, int sector, uint8_t *buff);
	//Call the callback for each block to update the sectorID in higher levels
	void (*forEachBlock)(BlockdevifHandle *handle, BlockdevifForEachBlockFn *cb, void *arg);
	//Is called when the block device has received an entire update.
	void (*notifyComplete)(BlockdevifHandle *handle, uint32_t id);
} BlockdevIf;


#endif