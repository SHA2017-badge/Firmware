#ifndef HLDEMUX_H
#define HLDEMUX_H

#include <stdint.h>
#include "recvif.h"

typedef void (HlCallback)(int subtype, uint8_t *data, int len, void *arg);

void hldemuxAddType(int type, HlCallback cb, void *arg);
void hldemuxRecv(uint8_t *packet, size_t len);


#endif