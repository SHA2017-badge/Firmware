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

#include "uECC.h"
#include "../keys/pubkey.inc"
#include "sha256.h"

static RecvCb *recvCb;

void chksignInit(RecvCb *cb) {
	recvCb=cb;
}

void chksignRecv(uint8_t *packet, size_t len) {
	if (len<sizeof(SignedPacket)) return;
	SignedPacket *p=(SignedPacket*)packet;
	int plLen=len-sizeof(SignedPacket);

	SHA256_CTX sha;
	uint8_t hash[32];
	//Calculate hash of packet
	sha256_init(&sha);
	sha256_update(&sha, p->data, plLen);
	sha256_final(&sha, hash);

	//Check signature of packet
	int isOk=uECC_verify(public_key, hash, sizeof(hash), p->sig, uECC_secp256r1());
	
	if (isOk) {
		recvCb(p->data, plLen);
	}
}
