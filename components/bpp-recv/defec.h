#ifndef DEFEC_H
#define DEFEC_H

#include "recvif.h"

typedef void (*FecSendDefeccedPacket)(uint8_t *packet, size_t len);
typedef int (*FecDecoderInit)(int k, int n, int maxsize);
typedef void (*FecDecoderRecv)(uint8_t *packet, size_t len, int serial, FecSendDefeccedPacket sendFn);
typedef void (*FecDecoderDeinit)();

typedef struct {
	const int algId;
	FecDecoderInit init;
	FecDecoderRecv recv;
	FecDecoderDeinit deinit;
} FecDecoder;



typedef struct {
	int packetsInTotal;
	int packetsInMissed;
} FecStatus;


void defecInit(RecvCb *cb, int maxLen);
void defecRecv(uint8_t *packet, size_t len);
void defecGetStatus(FecStatus *st);

#endif