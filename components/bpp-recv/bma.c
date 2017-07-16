#include "bma.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct Bma {
	int len;
	uint32_t bits[];
};


Bma *bmaCreate(int len) {
	Bma *ret=malloc(sizeof(Bma)+(((len+31)/32))*8);
	if (!ret) return NULL;
	ret->len=len;
	return ret;
}

void bmaFree(Bma *b) {
	free(b);
}

void bmaSet(Bma *b, int bit, int val) {
	if (bit<0 || bit>=b->len) return;
	if (val) {
		b->bits[bit/32]|=(1<<(bit&31));
	} else {
		b->bits[bit/32]&=~(1<<(bit&31));
	}
}

void bmaSetAll(Bma *b, int val) {
	char v=val?0xff:0;
	memset(b->bits, v, ((b->len+31)/32)*4);
}

bool bmaIsSet(Bma *b, int bit) {
	if (bit<0 || bit>=b->len) return false;
	return (b->bits[bit/32]&(1<<(bit&31)))!=0;
}

bool bmaIsAll(Bma *b, int val) {
	uint32_t v=val?0xFFFFFFFF:0;
	for (int i=0; i<b->len/32; i++) {
		if (b->bits[i]!=v) return 0;
	}

	int left=b->len&31;
	if (left) {
		uint32_t mask=0xFFFFFFFF>>(32-left);
		int lb=b->bits[b->len/32]&mask;
		if (lb!=(v&mask)) return 0;
	}
	return 1;
}

bool bmaIsAllSet(Bma *b) {
	return bmaIsAll(b, 1);
}

bool bmaIsAllClear(Bma *b) {
	return bmaIsAll(b, 0);
}


void bmaDump(Bma *b) {
	for (int i=0; i<b->len; i++) printf("%d", bmaIsSet(b, i)?1:0);
	printf("\n");
}

//Small testbed. Change 0 into 1 in the next line and compile with 'gcc -o bma bma.c'.
#if 0
#include <assert.h>

static void testBma(int len) {
	Bma *b=bmaCreate(len);
	bmaSetAll(b, 1);
	assert(bmaIsAllSet(b));
	assert(!bmaIsAllClear(b));
	bmaSet(b, len-1, 0);
	assert(!bmaIsAllSet(b));
	assert(!bmaIsAllClear(b));
	bmaSet(b, len-1, 1);
	assert(bmaIsAllSet(b));
	assert(!bmaIsAllClear(b));
	bmaSet(b, 0, 0);
	assert(!bmaIsAllSet(b));
	assert(!bmaIsAllClear(b));


	bmaSetAll(b, 0);
	assert(!bmaIsAllSet(b));
	assert(bmaIsAllClear(b));
	bmaSet(b, len-1, 1);
	assert(!bmaIsAllSet(b));
	assert(!bmaIsAllClear(b));
	bmaSet(b, len-1, 0);
	assert(!bmaIsAllSet(b));
	assert(bmaIsAllClear(b));
	bmaSet(b, 0, 1);
	assert(!bmaIsAllSet(b));
	assert(!bmaIsAllClear(b));
	bmaFree(b);
}

int main() {
	testBma(512);
	testBma(511);
	testBma(513);
	testBma(7);
	printf("Test OK!\n");

}
#endif

