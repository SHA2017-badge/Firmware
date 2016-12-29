#ifndef EPD_GDE_H
#define EPD_GDE_H

#include <stdint.h>

void initSPI();
void resetDisplay();
void writeCommand(unsigned char command);
void writeData(unsigned char data);
void writeCMD_p1(unsigned char command, unsigned char para);
void writeCMD_p2(unsigned char command, unsigned char para1,
                 unsigned char para2);
void writeStream(unsigned char *value, unsigned char datalen);
void writeDispRam(unsigned char xSize, unsigned int ySize,
                  const unsigned char *dispdata);
void writeDispRamMono(unsigned char xSize, unsigned int ySize,
                      unsigned char dispdata);

void spiWriteByte(const uint8_t data);

#endif
