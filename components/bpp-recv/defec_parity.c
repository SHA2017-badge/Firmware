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

static uint8_t **parPacket;
static uint32_t *parSerial;
static int defecK;

static int lastSentSerial=0; //sent to upper layer, that is
static int lastRecvSerial=0;


static int defecParInit(int k, int n, int maxLen) {
	if (k!=n-1) return 0;
	int i;
	
	parPacket=malloc(sizeof(uint8_t*)*k);
	parSerial=malloc(sizeof(uint32_t)*k);
	for (i=0; i<k; i++) {
		parPacket[i]=malloc(maxLen);
		parSerial[i]=0;
	}
	defecK=k;
	return 1;
}

static void defecParDeinit() {
	if (parPacket) {
		for (int i=0; i<defecK; i++) {
			free(parPacket[i]);
		}
	}
	free(parPacket);
	free(parSerial);
	parPacket=NULL;
	parSerial=NULL;
}


static void defecParRecv(uint8_t *packet, size_t len, int serial, FecSendDefeccedPacket sendFn) {
	lastRecvSerial=serial;

	int spos=serial%(defecK+1);
//	printf("Defec_parity:%d (%d) %s\n", serial, spos, (spos<FEC_M)?"D":"P");
	if (spos<defecK) {
		//Normal packet.
		//First, check if we missed a parity packet.
		int missedParityPacket=0;
		for (int i=spos; i<defecK; i++) {
			if (parSerial[i]!=0) missedParityPacket=1;
		}
		if (missedParityPacket) {
			printf("Defec_parity: missed parity packet\n");
			//Yes, we did. Dump out what's left in buffer for next time.
			lastSentSerial++; //because we missed that parity packet
			for (int i=spos; i<defecK; i++) {
				if (parSerial[i]>=lastSentSerial) {
					if (parSerial[i]!=lastSentSerial) sendFn(NULL, 0);
					sendFn(parPacket[i], len);
					lastSentSerial=parSerial[i];
				} else {
					sendFn(NULL, 0);
				}
				parSerial[i]=0;
			}
		}

		//Okay, we should be up to date with this sequence again.
		memcpy(parPacket[spos], packet, len);
		parSerial[spos]=serial;
		if (serial==lastSentSerial+1) {
			//Nothing weird, just send.
			sendFn(packet, len);
			lastSentSerial=serial;
		}
	} else {
		//Parity packet. See if we need to recover something.
		int exSerial=serial-defecK;
		int missing=-1;
		for (int i=0; i<defecK; i++) {
			if (parSerial[i]!=exSerial) {
				if (missing==-1) missing=i; else missing=-2;
				parSerial[i]=exSerial; //we'll recover this packet later if we can
			}
			exSerial++;
		}
		if (missing==-2) {
			printf("Defec_parity: Missed too many packets in segment\n");
			//Still send packets we do have.
			int exSerial=serial-defecK;
			//If we missed the entire prev fec unit, notify higher layer.
			if (lastSentSerial<(exSerial-2)) sendFn(NULL, 0);
			for (int i=0; i<defecK; i++) {
				if (parSerial[i]==exSerial) {
					sendFn(parPacket[i], len);
					lastSentSerial=i;
				} else {
					sendFn(NULL, 0);
				}
				exSerial++;
			}
		} else if (missing==-1) {
			//Nothing missing in *this* segment. Discard parity packet.
			int exSerial=serial-defecK-1; //We expect at least the last datapacket in the prev seq as the last one sent
			if (exSerial>lastSentSerial) {
				//Seems we're missing entire previous segments.
				printf("Defec_parity: Missing multiple segments! Packets %d - %d.\n", lastSentSerial, exSerial);
				sendFn(NULL, 0);
			}
		} else {
			//Missing one packet. We can recover this.
			//Xor the parity packet with the packets we have to magically allow the
			//missing packet to appear
			for (int i=0; i<defecK; i++) {
				if (i!=missing) {
					for (int j=0; j<len; j++) {
						packet[j]^=parPacket[i][j];
					}
				}
			}
			//We expect at least the last datapacket in the prev seq as the last one sent.
			int exSerial=serial-defecK-1;
			if (exSerial>lastSentSerial) {
				//Seems we're missing entire segments.
				printf("Defec_parity: Fixed 1 packet in this seg, but missing multiple segments! Packets %d - %d.\n", lastSentSerial, exSerial);
				sendFn(NULL, 0);
			}
			for (int i=missing; i<defecK; i++) {
				if (i==missing) {
					sendFn(packet, len);
				} else {
					sendFn(parPacket[i], len);
				}
			}
//			printf("Defec_parity: Restored packet.\n");
		}
		lastSentSerial=serial;
		for (int i=0; i<defecK; i++) parSerial[i]=0;
	}
}


const FecDecoder fecDecoderParity={
	.algId=FEC_ID_PARITY,
	.init=defecParInit,
	.recv=defecParRecv,
	.deinit=defecParDeinit
};
