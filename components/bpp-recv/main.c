#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "structs.h"
#include "chksign.h"
#include "defec.h"
#include "serdec.h"
#include "hldemux.h"

#include "subtitle.h"
#include "blockdecode.h"
#include "bd_emu.h"
#include "bd_flatflash.h"
#include "bd_ropart.h"
#include "hkpackets.h"



static int createListenSock() {
	int sock;
	struct sockaddr_in addr;
	unsigned short port=2017;

	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("creating socket");
		exit(1);
	}

	/* Construct bind structure */
	memset(&addr, 0, sizeof(addr));	/* Zero out structure */
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	addr.sin_port=htons(port);

	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("bind");
		exit(1);
	}
	return sock;
}

void flashDone(void *arg) {
	printf("Flash done!\n");
	exit(0);
}

void myRecv(uint8_t *packet, size_t len) {
	printf("Got packet size %d: ", len);
	if (packet!=NULL) {
		packet[len]=0;
		printf("%s\n", packet);
	}
}


void checkBlockDevAgainst(BlockdevIf *iface, BlockDecodeHandle *h, char *fn) {
	char buff1[4096], buff2[4096];
	int f=open(fn, O_RDONLY);
	if (f<=0) {
		perror(fn);
		exit(1);
	}
	printf("Checking bd against %s\n", fn);
	int blk=0;
	while(read(f, buff1, 4096)==4096) {
		int r=iface->getSectorData(blockdecodeGetIf(h), blk, buff2);
		if (r) {
			if (memcmp(buff1, buff2, 4096)!=0) {
				printf("Check: Difference in block %d. File vs blkdev:\n", blk);
				hexdump(buff1, 256);
				hexdump(buff2, 256);
			}
		} else {
			printf("Check: Read error on block %d\n", blk);
		}
		blk++;
	}
	printf("Check done.\n");
	sleep(3);
}

int simDeepSleepMs=0;

int main(int argc, char** argv) {
	int sock=createListenSock();
	int len;
	uint8_t buff[1400];
	BlockDecodeHandle *ropartblockdecoder;

	chksignInit(defecRecv);
	defecInit(serdecRecv, 1400);
	serdecInit(hldemuxRecv);
	
#if 0 //test flatflash

	BlockdevIfFlatFlashDesc bdesc={
		.major=0x12,
		.minor=0x34,
		.doneCb=flashDone,
		.doneCbArg=NULL,
		.minChangeId=1494667311
	};
	blockdecodeInit(1, 8*1024*1024, &blockdevIfFlatFlash, &bdesc);
#endif

#if 1
	BlockdevIfFlatFlashDesc bdesc_fat={
		.major=0x20,
		.minor=0x17,
	};
	int bpsize=(8192-800)*1024;
	ropartblockdecoder=blockdecodeInit(3, bpsize, &blockdevIfRoPart, &bdesc_fat);
	printf("Initialized ropart blockdev listener; size=%d\n", bpsize);
#endif

	checkBlockDevAgainst(&blockdevIfRoPart, ropartblockdecoder, "/home/jeroen/esp8266/esp32/badge/bpp/blocksend/tst/fatimage.img");

	subtitleInit();
	hkpacketsInit();

	int dly=0;
	while((len = recvfrom(sock, buff, sizeof(buff), 0, NULL, 0)) >= 0) {
		chksignRecv(buff, len);
		if (simDeepSleepMs!=0) break;
		if ((dly&255)==0) {
			blockdecodeStatus(ropartblockdecoder);
		}
		dly++;
	}
	
	close(sock);

	if (simDeepSleepMs) {
		simDeepSleepMs-=200; //to be sure
		printf("Deep sleep for %d sec.\n", simDeepSleepMs/1000);
		if (simDeepSleepMs>0) usleep(simDeepSleepMs*1000);
		printf("Restarting.\n");
		//restart
		execv(argv[0], argv);
		printf("SHOULDNT COME HERE\n");
	}

	exit(0);
}
