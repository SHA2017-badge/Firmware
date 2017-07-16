/*
Packet signature checking

Every packet sent out is signed using ECDSA. We check that signature using the micro-ecc library.
*/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "recvif.h"
#include "structs.h"

#include "ed25519.h"
#include "pubkey.inc"

static RecvCb *recvCb;

void chksignInit(RecvCb *cb) {
	recvCb=cb;
}

int chksignRecv(uint8_t *packet, size_t len) {
	if (len<sizeof(SignedPacket)) return;
	SignedPacket *p=(SignedPacket*)packet;
	int plLen=len-sizeof(SignedPacket);

	//Check signature of packet
	int isOk=ed25519_verify(p->sig, p->data, plLen, public_key);
	if (isOk) {
		recvCb(p->data, plLen);
	} else {
		printf("Signature check failed.\n");
	}
	return isOk;
}
