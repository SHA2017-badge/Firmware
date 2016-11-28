#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "rom/gpio.h"
#include "sdkconfig.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_reg.h"
#include "soc/spi_reg.h"
#include <stdio.h>
#include <string.h>

#include "gde.h"
#include "pins.h"

static void spi_write_byte(const uint8_t data) {
  SET_PERI_REG_BITS(SPI_MOSI_DLEN_REG(SPI_NUM), SPI_USR_MOSI_DBITLEN, 0x7,
                    SPI_USR_MOSI_DBITLEN_S);
  WRITE_PERI_REG((SPI_W0_REG(SPI_NUM)), data);
  SET_PERI_REG_MASK(SPI_CMD_REG(SPI_NUM), SPI_USR);
  while (READ_PERI_REG(SPI_CMD_REG(SPI_NUM)) & SPI_USR)
    ;
}

void resetDisplay() {
  gpio_set_level(PIN_NUM_RESET, 0);
  ets_delay_us(1000);
  gpio_set_level(PIN_NUM_RESET, 1);
  ets_delay_us(1000);
}

void writeCommand(unsigned char command) {
  gpio_set_level(PIN_NUM_CS, 1);
  gpio_set_level(PIN_NUM_CS, 0);
  spi_write_byte(command);
  gpio_set_level(PIN_NUM_CS, 1);
}

void writeData(unsigned char data) {
  gpio_set_level(PIN_NUM_CS, 1);
  gpio_set_level(PIN_NUM_CS, 0);
  gpio_set_level(PIN_NUM_DATA, 1);
  spi_write_byte(data);
  gpio_set_level(PIN_NUM_CS, 1);
  gpio_set_level(PIN_NUM_DATA, 0);
}

void writeCMD_p1(unsigned char command, unsigned char para) {
  gpio_set_level(PIN_NUM_CS, 1);
  gpio_set_level(PIN_NUM_CS, 0);
  spi_write_byte(command);
  gpio_set_level(PIN_NUM_DATA, 1);
  spi_write_byte(para);
  gpio_set_level(PIN_NUM_CS, 1);
  gpio_set_level(PIN_NUM_DATA, 0);
}

void writeCMD_p2(unsigned char command, unsigned char para1,
                 unsigned char para2) {
  gpio_set_level(PIN_NUM_CS, 1);
  gpio_set_level(PIN_NUM_CS, 0);
  spi_write_byte(command);
  gpio_set_level(PIN_NUM_DATA, 1);
  spi_write_byte(para1);
  spi_write_byte(para2);
  gpio_set_level(PIN_NUM_CS, 1);
  gpio_set_level(PIN_NUM_DATA, 0);
}

void writeStream(unsigned char *value, unsigned char datalen) {
  unsigned char i = 0;
  unsigned char *ptemp;

  ptemp = value;
  gpio_set_level(PIN_NUM_CS, 1);
  gpio_set_level(PIN_NUM_CS, 0);
  spi_write_byte(*ptemp);
  ptemp++;
  gpio_set_level(PIN_NUM_DATA, 1);
  for (i = 0; i < datalen - 1; i++) // sub the command
  {
    spi_write_byte(*ptemp);
    ptemp++;
  }
  gpio_set_level(PIN_NUM_CS, 1);
  gpio_set_level(PIN_NUM_DATA, 0);
}

void writeDispRam(unsigned char xSize, unsigned int ySize,
                  const unsigned char *dispdata) {
  unsigned int i = 0, j = 0, c = 0;
  char data;

  if (xSize % 8 != 0) {
    xSize = xSize + (8 - xSize % 8);
  }
  xSize = xSize / 8;

  gpio_set_level(PIN_NUM_CS, 1);
  gpio_set_level(PIN_NUM_CS, 0);
  spi_write_byte(0x24);

  gpio_set_level(PIN_NUM_DATA, 1);
  for (i = 0; i < ySize; i++) {
    for (j = 0; j < xSize; j++) {
      data = dispdata[c];
      spi_write_byte(~data);
      c++;
    }
  }
  gpio_set_level(PIN_NUM_CS, 1);
  gpio_set_level(PIN_NUM_DATA, 0);
}

void writeDispRamMono(unsigned char xSize, unsigned int ySize,
                      unsigned char dispdata) {
  unsigned int i = 0, j = 0;
  if (xSize % 8 != 0) {
    xSize = xSize + (8 - xSize % 8);
  }
  xSize = xSize / 8;

  gpio_set_level(PIN_NUM_CS, 1);
  gpio_set_level(PIN_NUM_CS, 0);
  spi_write_byte(0x24);

  gpio_set_level(PIN_NUM_DATA, 1);
  for (i = 0; i < ySize; i++) {
    for (j = 0; j < xSize; j++) {
      spi_write_byte(dispdata);
    }
  }
  gpio_set_level(PIN_NUM_CS, 1);
  gpio_set_level(PIN_NUM_DATA, 0);
}
