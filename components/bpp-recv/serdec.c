/*
Decode the flexible length packets encoded in the fixed-length stream
*/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "recvif.h"
#include "structs.h"
#ifndef HOST_BUILD
#include "rom/crc.h"
#endif
#include "crc16-ccitt.h"
#include "hexdump.h"

#define MAX_PACKET_LEN (8*1024)

static RecvCb *recvCb;

static uint8_t serPacket[MAX_PACKET_LEN];
static SerdesHdr hdr;
static int pos=-1;
static int hdrBytesScanned=0; //for information purposes


void serdecInit(RecvCb *cb) {
	recvCb=cb;
}

static int scanHdr(uint8_t in) {
	uint8_t *h=(uint8_t*)&hdr;
	//Scanning for header
	hdrBytesScanned++;
	for (int x=1; x<sizeof(SerdesHdr); x++) h[x-1]=h[x];
	h[sizeof(SerdesHdr)-1]=in;
	if (ntohl(hdr.magic)==SERDES_MAGIC) {
		if (ntohs(hdr.len)<MAX_PACKET_LEN) {
//			if (hdrBytesScanned!=sizeof(SerdesHdr)) {
//				printf("Serdec: skipped %d bytes\n", hdrBytesScanned-sizeof(SerdesHdr));
//			}
			hdrBytesScanned=0;
			return 1;
		}
	}
	return 0;
}


static void finishPacket() {
	int plen=ntohs(hdr.len);
#ifndef HOST_BUILD
	uint16_t crc, rcrc;
	rcrc=ntohs(hdr.crc16);
	hdr.crc16=0;
//	crc=crc16_le(0, (uint8_t*)&hdr, sizeof(SerdesHdr));
//	crc=crc16_le(crc, serPacket, plen);
	crc=crc16_ccitt(0, (uint8_t*)&hdr, sizeof(SerdesHdr));
	crc=crc16_ccitt(crc, serPacket, plen);
	if (crc!=rcrc) {
		printf("Serdec: CRC16 error! Got %04X expected %04X\n", crc, rcrc);
		//hexdump(serPacket, plen);
	} else {
		recvCb(serPacket, plen);
	}
#else
	recvCb(serPacket, plen);
#endif
}

void serdecRecv(uint8_t *packet, size_t len) {
	int i=0;
	if (len==0 || packet==NULL) {
		//Used to indicate some packets got lost. Reset receive system, discard
		//any data we may have received, propagate lost packet info up.
		recvCb(NULL, 0);
		pos=-1;
		memset(&hdr, 0, sizeof(SerdesHdr));
		return;
	}

	while (i<len) {
		if (pos==-1) {
			if (scanHdr(packet[i])) {
				pos=0;
			}
			i++;
		} else {
			//Receiving
			int plen=ntohs(hdr.len);
			int left=plen-pos;
			if (left>(len-i)) left=len-i;
			memcpy(&serPacket[pos], &packet[i], left);
			//printf("Pos=%d plen=%d len=%d  i=%d (len-i)=%d left=%d\n", pos, plen, len, i, len-i, left);
			pos+=left;
			i+=left;
			if (pos==plen) {
				finishPacket();
				pos=-1;
				memset(&hdr, 0, sizeof(SerdesHdr));
			}
		}
	}
}
