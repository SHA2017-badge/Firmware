/*
Try to ressurect missing packets using FEC
*/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <freertos/portmacro.h>
#include "recvif.h"
#include "structs.h"
#include "defec.h"
#include "esp_attr.h"


extern const FecDecoder fecDecoderParity;
extern const FecDecoder fecDecoderRs;

static const FecDecoder *decoders[]={
	&fecDecoderParity,
	&fecDecoderRs,
	NULL
};

typedef struct {
	int k, n, algId;
} FecSavedStatus;

static RTC_DATA_ATTR FecSavedStatus savedStatus;

static RecvCb *recvCb;
static const FecDecoder *currDecoder;
static int currK, currN;
static size_t maxPacketSize;
static portMUX_TYPE statusMux = portMUX_INITIALIZER_UNLOCKED;
static FecStatus status;
static int lastRecvSerial;

void defecInit(RecvCb *cb, int maxLen) {
	currDecoder=decoders[0];
	currK=3;
	currN=4;
	maxPacketSize=maxLen;
	recvCb=cb;
	if (savedStatus.k!=0 && savedStatus.n!=0) {
		//restore status
		int i;
		currK=savedStatus.k;
		currN=savedStatus.n;
		for (i=0; decoders[i]!=NULL; i++) {
			if (decoders[i]->algId==savedStatus.algId) break;
		}
		currDecoder=decoders[i];
	}
	currDecoder->init(currK, currN, maxLen);
}

void defecGetStatus(FecStatus *st) {
	portENTER_CRITICAL(&statusMux);
	memcpy(st, &status, sizeof(status));
	portEXIT_CRITICAL(&statusMux);
}

static void defecRecvDefecced(uint8_t *packet, size_t len) {
	recvCb(packet, len);
}


void defecRecv(uint8_t *packet, size_t len) {
	if (len<sizeof(FecPacket)) return;
	FecPacket *p=(FecPacket*)packet;
	int plLen=len-sizeof(FecPacket);

	int serial=ntohl(p->serial);
	if (serial==0) {
		//Special packet: contains fec parameters
		if (plLen<sizeof(FecDesc)) return;
		FecDesc *d=(FecDesc*)p->data;
		if (currDecoder==NULL || \
				currDecoder->algId!=d->fecAlgoId || \
				currK!=ntohs(d->k) || \
				currN!=ntohs(d->n)) {
			//Fec parameters changed. Close current decoder, open new one.
			if (currDecoder) currDecoder->deinit();

			currK=ntohs(d->k);
			currN=ntohs(d->n);
			savedStatus.k=currK;
			savedStatus.n=currN;
			savedStatus.algId=d->fecAlgoId;
			int i;
			for (i=0; decoders[i]!=NULL; i++) {
				if (decoders[i]->algId==d->fecAlgoId) break;
			}
			currDecoder=decoders[i];
			if (!currDecoder) {
				printf("FEC: No decoder found for algo id %d!\n", d->fecAlgoId);
			} else {
				int r=currDecoder->init(currK, currN, maxPacketSize);
				if (!r) {
					currDecoder=NULL;
					printf("FEC: Couldn't initialize decoder id %d for k=%d n=%d!\n", d->fecAlgoId, currK, currN);
				} else {
					printf("FEC: Changed to decoder id %d, k=%d n=%d!\n", d->fecAlgoId, currK, currN);
				}
			}
		}
		return;
	}
	if (!currDecoder) return; //can't decode!

	if (serial<=lastRecvSerial) return; //dup

	if (lastRecvSerial!=0) {
		portENTER_CRITICAL(&statusMux);
		status.packetsInTotal+=serial-lastRecvSerial;
		status.packetsInMissed+=(serial-lastRecvSerial)-1;
		portEXIT_CRITICAL(&statusMux);
	}
	lastRecvSerial=serial;

	currDecoder->recv(p->data, plLen, serial, defecRecvDefecced);
}
