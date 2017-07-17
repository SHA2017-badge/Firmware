#include <stdio.h>
#include <arpa/inet.h>
#include "hldemux.h"
#include "structs.h"
#include "powerdown.h"
#include "hkpackets.h"

static void hkpacketsRecv(int subtype, uint8_t *data, int len, void *arg) {
	if (subtype==HKPACKET_SUBTYPE_NEXTCATALOG) {
		HKPacketNextCatalog *p=(HKPacketNextCatalog*)data;
		int delayMs=ntohl(p->delayMs);
		printf("HKPacket: next catalog in %d ms\n", delayMs);
		//Use address of init function as random reference.
		powerCanSleepFor((int)hkpacketsInit, delayMs);
	} else {
		printf("hkpackets: Unknown housekeeping packet subtype: %d\n", subtype);
	}
}

void hkpacketsInit() {
	hldemuxAddType(HLPACKET_TYPE_HK, hkpacketsRecv, NULL);
	powerHold((int)hkpacketsInit, 30*1000);
}

