#ifndef SERDEC_H
#define SERDEC_H

#include "recvif.h"

void serdecInit(RecvCb *cb);
void serdecRecv(uint8_t *packet, size_t len);

#endif