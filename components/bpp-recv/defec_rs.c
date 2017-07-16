/*
Try to ressurect missing packets using the parity packet
*/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "recvif.h"
#include "structs.h"
#include "defec.h"
#include "redundancy.h"

static uint8_t *rsPacket;
static gbf_int_t *rsSerial;
static int curBin; // = serial/rsN
static int recved;
static int rsK, rsN;
static int curLen;
static int lastOkBin;


static int defecRsInit(int k, int n, int maxLen) {
	rsPacket=malloc(maxLen*k);
	rsSerial=malloc(sizeof(gbf_int_t)*k);
	if (rsPacket==NULL || rsSerial==NULL) {
		free(rsPacket);
		free(rsSerial);
		return 0;
	}
	rsK=k;
	rsN=n;
	recved=0;
	curLen=0;
	lastOkBin=0;
	gbf_init(GBF_POLYNOME);
	return 1;
}

static void defecRsDeinit() {
	free(rsPacket);
	free(rsSerial);
	rsPacket=NULL;
	rsSerial=NULL;
}

static int flushRsState(FecSendDefeccedPacket sendFn) {
	int r=0;
	if (recved>=rsK) {
		uint8_t *out=malloc(rsK*curLen);
		if (out!=NULL) {
			gbf_decode((gbf_int_t*)out, (gbf_int_t*)rsPacket, rsSerial, rsK, (curLen/sizeof(gbf_int_t)));
			for (int i=0; i<rsK; i++) {
				sendFn(&out[i*curLen], curLen);
			}
			r=1;
		} else {
			printf("defecRs: can't allocate mem to decode packet!\n");
		}
		free(out);
	}
	recved=0;
	return r;
}


static void defecRsRecv(uint8_t *packet, size_t len, int serial, FecSendDefeccedPacket sendFn) {
	int bin=serial/rsN;
	if (bin==lastOkBin) return; //already sent this.
	if (bin!=curBin) {
		//Did we miss a bin?
		if (lastOkBin!=bin-1) {
			printf("defecRs: Missed a bin.\n");
			sendFn(NULL, 0);
		}
		//See if we can send whatever we had from the prev bin and start anew.
		if (flushRsState(sendFn)) lastOkBin=curBin;
		curBin=bin;
	}
	if (curLen==0) curLen=len;
	if (curLen!=len) {
		//shouldn't happen
		flushRsState(sendFn);
		curLen=len;
	}
	
	memcpy(&rsPacket[recved*curLen], packet, len);
	rsSerial[recved]=(serial%rsN)+1;
	recved++;
	if (recved==rsK) {
		//Got enough packets to do decode.
		if (flushRsState(sendFn)) lastOkBin=curBin;
	}
}


const FecDecoder fecDecoderRs={
	.algId=FEC_ID_RS,
	.init=defecRsInit,
	.recv=defecRsRecv,
	.deinit=defecRsDeinit
};
