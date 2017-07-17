#ifndef BLKIDCACHE_H
#define BLKIDCACHE_H

/*
Blockids change pretty often (in theory, every single one changes once per cycle) and writing these
to storage every time can be costly. On the other hand, not writing these to flash has results that
are not nice but not critical either: worst case, after a reboot the system erroneously thinks some 
sectors are old and need refreshing. That is why this info can be cached in RAM in some fashion.
*/

#include <stdint.h>
#include "blockdevif.h"

typedef struct BlkIdCacheHandle BlkIdCacheHandle;

//Size is in blocks.
BlkIdCacheHandle *idcacheCreate(int size, BlockdevifHandle *blkdev, BlockdevIf *bdif);

void idcacheSet(BlkIdCacheHandle *h, int block, uint32_t id);
uint32_t idcacheGet(BlkIdCacheHandle *h, int block);
void idcacheFlushToStorage(BlkIdCacheHandle *h);
void idcacheSetSectorData(BlkIdCacheHandle *h, int block, uint8_t *data, uint32_t id);
uint32_t idcacheGetLastChangeId(BlkIdCacheHandle *h);
#endif