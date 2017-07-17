/*
This is a simple testcase for the logic in bd_ropart.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include "bd_ropart.h"
#include "blockdevif.h"
#include "blkidcache.h"

#define BLKSZ 4096
#define PHYS_SIZE 31 //weird number of sectors to squirrel out edge-cases
#define VIRT_SIZE 17

void initBdFile(int major, int minor, int size) {
	char buf[128];
	sprintf(buf, "part-%d-%d.img", major, minor);
	int f=open(buf, O_CREAT|O_TRUNC|O_WRONLY, 0666);
	assert(f>0);
	char *d=malloc(size);
	assert(d);
	for (int i=0; i<size; i++) d[i]=rand();
	write(f, d, size);
	free(d);
	printf("File %s created and filled with random crap.\n", buf);
}

void fillSector(BlkIdCacheHandle *h, int sector, int seed, int chid) {
	uint8_t buf[BLKSZ];
	srand(seed);
	printf("Filling sector %d, seed %d, chid %d\n", sector, seed, chid);
	for (int i=0; i<BLKSZ; i++) buf[i]=rand()&0xff;
	buf[0]=seed; //for easier debug
	idcacheSetSectorData(h, sector, buf, chid);
}

#define CS_LLERR 0
#define CS_DATAERR 1
#define CS_OK 2

int checkSector(BlockdevIf *bdif, BlockdevifHandle *h, int sector, int seed) {
	uint8_t buf[BLKSZ];
	printf("Checking sector %d, seed %d\n", sector, seed);
	int i=bdif->getSectorData(h, sector, buf);
	if (!i) {
		printf("Lower-level error reading sect %d\n", sector);
		return CS_LLERR;
	}
	srand(seed);
	for (i=0; i<BLKSZ; i++) {
		uint8_t ex=rand()&0xff;
		if (i==0) ex=seed;
		if (buf[i]!=ex) {
			printf("Data compare error: sector %d byte %d expected %x got %x\n", sector, i, ex, buf[i]);
			return CS_DATAERR;
		}
	}
	return CS_OK;
}

int main(int argc, char** argv) {
	int tstno=0;
	int i;
	if (argc>1) tstno=atoi(argv[1]);
	BlockdevIfRoPartDesc bddesc={
		.major=12,
		.minor=34
	};

	if (tstno==0) {
		initBdFile(bddesc.major, bddesc.minor, PHYS_SIZE*BLKSZ);
	}

	printf("\n\n\nStarting test phase %d\n", tstno);

	BlockdevIf *bdif=&blockdevIfRoPart;
	BlockdevifHandle *h=bdif->init(&bddesc, VIRT_SIZE*BLKSZ);
	BlkIdCacheHandle *cache=idcacheCreate(VIRT_SIZE, h, bdif);

	if (tstno==0) {
		//Thing should be entirely uninitialized.
		printf("*** Check for uninitialized\n");
		for (int i=0; i<VIRT_SIZE; i++) {
			assert(checkSector(bdif, h, i, 0)==CS_LLERR);
		}
		//Unknown sector shouldn't crash
		printf("*** Check read out-of-bounds sector\n");
		assert(checkSector(bdif, h, VIRT_SIZE+1, 0)==CS_LLERR);

		//Populate with first data
		printf("*** Full populate, chid=1\n");
		for (int i=VIRT_SIZE-1; i>=0; i--) {
			fillSector(cache, i, i, 1);
		}
		//This should've lead to a notifyComplete and the bd snapshotting the state. Check this.
		printf("*** Check full populate, chid=1\n");
		for (i=0; i<VIRT_SIZE; i++) {
			assert(checkSector(bdif, h, i, i)==CS_OK);
		}
		//Restart to see if result sticks.
		execlp(argv[0], argv[0], "1", NULL);
	} else if (tstno==1) {
		//See if we're still there.
		printf("*** Re-check full populate, chid=1\n");
		for (i=0; i<VIRT_SIZE; i++) {
			assert(checkSector(bdif, h, i, i)==CS_OK);
		}
		//Change some sectors. We should still have the old snapshot.
		printf("*** Partial change, chid=2\n");
		for (i=0; i<5; i++) {
			fillSector(cache, i, i+100, 2);
		}
		//Check if this changed anything.
		printf("*** Checking if data still is old snapshot data\n");
		for (i=0; i<VIRT_SIZE; i++) {
			assert(checkSector(bdif, h, i, i)==CS_OK);
		}
		//Update the other sectors to the new id
		printf("*** Updating rest of sectors\n");
		for (i=5; i<VIRT_SIZE; i++) {
			idcacheSet(cache, i, 2);
		}
		//Check if we're on the new snapshot now
		printf("*** Checking for new snapshot data\n");
		for (i=0; i<VIRT_SIZE; i++) {
			assert(checkSector(bdif, h, i, (i<5)?i+100:i)==CS_OK);
		}
		bdropartDumpJournal(h);
		//Do another update. Fill all but one sector with new data.
		printf("*** Filling almost entirely with new data, killing snapshot\n");
		for (i=0; i<VIRT_SIZE-1; i++) {
			fillSector(cache, i, i+200, 3);
		}
		bdropartDumpJournal(h);
		//This oughtta have killed the snapshot.
		printf("*** Check if snapshot is killed\n");
		for (int i=0; i<VIRT_SIZE; i++) {
			assert(checkSector(bdif, h, i+200, 3)==CS_LLERR);
		}
		//Restart to see if result sticks.
		execlp(argv[0], argv[0], "2", NULL);
	} else if (tstno==2) {
		//Snapshot should still be dead
		printf("*** Check if snapshot is dead after reinit\n");
		for (int i=0; i<VIRT_SIZE; i++) {
			assert(checkSector(bdif, h, i+200, 3)==CS_LLERR);
		}
		//Fill last sector.
		printf("*** Fill last sector\n");
		fillSector(cache, VIRT_SIZE-1, VIRT_SIZE-1+200, 3);
		//Snapshot should be ok now.
		printf("*** Check if snapshot is ok now.\n");
		for (int i=0; i<VIRT_SIZE; i++) {
			assert(checkSector(bdif, h, i, i+200)==CS_OK);
		}

		printf("*** Brute-force testing.\n");
		int secSeed[VIRT_SIZE];
		for (int id=100; id<600; id++) {
			for (i=0; i<VIRT_SIZE; i++) {
				if ((rand()&1) || id==100) {
					secSeed[i]=rand()&0xff;
					fillSector(cache, i, secSeed[i], id);
				} else {
					idcacheSet(cache, i, id);
				}
			}
			for (i=0; i<VIRT_SIZE; i++) {
				assert(checkSector(bdif, h, i, secSeed[i])==CS_OK);
			}
		}
		execlp(argv[0], argv[0], "3", NULL);
	} else if (tstno==3) {
		printf("*** Replacing entire contents\n");
		for (int id=700; i<720; i++) {
			for (i=0; i<VIRT_SIZE; i++) {
				fillSector(cache, i, i, id);
			}
			for (i=0; i<VIRT_SIZE; i++) {
				assert(checkSector(bdif, h, i, i)==CS_OK);
			}
		}

	}

}