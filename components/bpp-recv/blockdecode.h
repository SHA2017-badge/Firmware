#ifndef BLOCKDECODE_H
#define BLOCKDECODE_H

#include "blockdevif.h"

typedef struct BlockDecodeHandle BlockDecodeHandle;

BlockdevifHandle *blockdecodeGetIf(BlockDecodeHandle *d);

void blockdecodeStatus(BlockDecodeHandle *d);

BlockDecodeHandle *blockdecodeInit(int type, int size, BlockdevIf *bdIf, void *bdevdesc);

void blockdecodeShutDown(BlockDecodeHandle *d);


#endif
