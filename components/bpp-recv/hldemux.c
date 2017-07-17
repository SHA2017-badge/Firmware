/*
Distribute packets depending on type.
*/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "recvif.h"
#include "structs.h"

#include "hldemux.h"

typedef struct HlCallbackInfo HlCallbackInfo;

struct HlCallbackInfo {
	int type;
	HlCallback *cb;
	void *arg;
	HlCallbackInfo *next;
};

static HlCallbackInfo *cbinfo=NULL;


void hldemuxAddType(int type, HlCallback cb, void *arg) {
	HlCallbackInfo *item=malloc(sizeof(HlCallbackInfo));
	item->type=type;
	item->cb=cb;
	item->arg=arg;
	item->next=cbinfo;
	cbinfo=item;
}

void hldemuxRecv(uint8_t *packet, size_t len) {
	if (len<sizeof(HlPacket)) return;
	HlPacket *p=(HlPacket*)packet;
	int plLen=len-sizeof(HlPacket);

	int type=ntohs(p->type);
	int subtype=ntohs(p->subtype);

	int found=0;
	for (HlCallbackInfo *i=cbinfo; i!=NULL; i=i->next) {
		if (i->type==type) {
			i->cb(subtype, p->data, plLen, i->arg);
			found=1;
		}
	}
	if (!found) {
		printf("hldemux: Huh? No handler known for type %d (subtype %d)\n", type, subtype);
	}
}
