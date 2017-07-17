/*
Small implementation of a BitMapArray, similar to vector<bool> in C++. Made because
doing this manually every time is too error-prone...
*/
#ifndef BMA_H
#define BMA_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Bma Bma;

Bma *bmaCreate(int len);
void bmaSet(Bma *b, int bit, int val);
void bmaSetAll(Bma *b, int val);
bool bmaIsSet(Bma *b, int bit);
bool bmaIsAll(Bma *b, int val);
bool bmaIsAllSet(Bma *b);
bool bmaIsAllClear(Bma *b);
void bmaFree(Bma *b);
void bmaDump(Bma *b);

#endif

