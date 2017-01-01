#include "gde.h"
#include "driver/gpio.h"
#include "pins.h"
#include "rom/ets_sys.h"
#include "rom/gpio.h"
#include "sdkconfig.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_struct.h"
#include "soc/spi_reg.h"
#include <stdio.h>
#include <string.h>

#define SPI_NUM  0x3

#ifndef LOW
#define LOW 0
#endif
#ifndef HIGH
#define HIGH 1
#endif

void resetDisplay() {
  gpio_set_level(PIN_NUM_RESET, LOW);
  ets_delay_us(200000);
  gpio_set_level(PIN_NUM_RESET, HIGH);
  ets_delay_us(200000);
}

void writeCommand(unsigned char command) {
  gpio_set_level(PIN_NUM_CS, HIGH);
  gpio_set_level(PIN_NUM_CS, LOW);
  spiWriteByte(command);
  gpio_set_level(PIN_NUM_CS, HIGH);
}

void writeData(unsigned char data) {
  gpio_set_level(PIN_NUM_CS, HIGH);
  gpio_set_level(PIN_NUM_CS, LOW);
  gpio_set_level(PIN_NUM_DATA, HIGH);
  spiWriteByte(data);
  gpio_set_level(PIN_NUM_CS, HIGH);
  gpio_set_level(PIN_NUM_DATA, LOW);
}

void writeCMD_p1(unsigned char command, unsigned char para) {
  gpio_set_level(PIN_NUM_CS, HIGH);
  gpio_set_level(PIN_NUM_CS, LOW);
  spiWriteByte(command);
  gpio_set_level(PIN_NUM_DATA, HIGH);
  spiWriteByte(para);
  gpio_set_level(PIN_NUM_CS, HIGH);
  gpio_set_level(PIN_NUM_DATA, LOW);
}

void writeCMD_p2(unsigned char command, unsigned char para1,
                 unsigned char para2) {
  gpio_set_level(PIN_NUM_CS, HIGH);
  gpio_set_level(PIN_NUM_CS, LOW);
  spiWriteByte(command);
  gpio_set_level(PIN_NUM_DATA, HIGH);
  spiWriteByte(para1);
  spiWriteByte(para2);
  gpio_set_level(PIN_NUM_CS, HIGH);
  gpio_set_level(PIN_NUM_DATA, LOW);
}

void writeStream(unsigned char *value, unsigned char datalen) {
  unsigned char i = 0;
  unsigned char *ptemp;

  ptemp = value;
  gpio_set_level(PIN_NUM_CS, HIGH);
  gpio_set_level(PIN_NUM_CS, LOW);
  spiWriteByte(*ptemp);
  ptemp++;
  gpio_set_level(PIN_NUM_DATA, HIGH);
  for (i = 0; i < datalen - 1; i++) // sub the command
  {
    spiWriteByte(*ptemp);
    ptemp++;
  }
  gpio_set_level(PIN_NUM_CS, HIGH);
  gpio_set_level(PIN_NUM_DATA, LOW);
}

void writeDispRam(unsigned char xSize, unsigned int ySize,
                  const unsigned char *dispdata) {
  unsigned int i = 0, j = 0, c = 0;
  char data;

  if (xSize % 8 != 0) {
    xSize = xSize + (8 - xSize % 8);
  }
  xSize = xSize / 8;

  gpio_set_level(PIN_NUM_CS, HIGH);
  gpio_set_level(PIN_NUM_CS, LOW);
  spiWriteByte(0x24);

  gpio_set_level(PIN_NUM_DATA, HIGH);
  for (i = 0; i < ySize; i++) {
    for (j = 0; j < xSize; j++) {
      data = dispdata[c];
      spiWriteByte(data);
      c++;
    }
  }
  gpio_set_level(PIN_NUM_CS, HIGH);
  gpio_set_level(PIN_NUM_DATA, LOW);
}

void writeDispRamMono(unsigned char xSize, unsigned int ySize,
                      unsigned char dispdata) {
  unsigned int i = 0, j = 0;
  if (xSize % 8 != 0) {
    xSize = xSize + (8 - xSize % 8);
  }
  xSize = xSize / 8;

  gpio_set_level(PIN_NUM_CS, HIGH);
  gpio_set_level(PIN_NUM_CS, LOW);
  spiWriteByte(0x24);

  gpio_set_level(PIN_NUM_DATA, HIGH);
  for (i = 0; i < ySize; i++) {
    for (j = 0; j < xSize; j++) {
      spiWriteByte(dispdata);
    }
  }
  gpio_set_level(PIN_NUM_CS, HIGH);
  gpio_set_level(PIN_NUM_DATA, LOW);
}

void initSPI() {
  gpio_set_direction(PIN_NUM_CS, GPIO_MODE_OUTPUT);
  gpio_set_direction(PIN_NUM_DATA, GPIO_MODE_OUTPUT);
  gpio_set_direction(PIN_NUM_RESET, GPIO_MODE_OUTPUT);
  gpio_set_direction(PIN_NUM_BUSY, GPIO_MODE_INPUT);
  ets_printf("epd spi pin mux init ...\r\n");
  gpio_matrix_out(PIN_NUM_MOSI, VSPID_OUT_IDX,0,0);
  gpio_matrix_out(PIN_NUM_CLK, VSPICLK_OUT_MUX_IDX,0,0);
  gpio_matrix_out(PIN_NUM_CS, VSPICS0_OUT_IDX,0,0);
  CLEAR_PERI_REG_MASK(SPI_SLAVE_REG(SPI_NUM), SPI_TRANS_DONE << 5);
  SET_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_CS_SETUP);
  CLEAR_PERI_REG_MASK(SPI_PIN_REG(SPI_NUM), SPI_CK_IDLE_EDGE);
  CLEAR_PERI_REG_MASK(SPI_USER_REG(SPI_NUM),  SPI_CK_OUT_EDGE);
  CLEAR_PERI_REG_MASK(SPI_CTRL_REG(SPI_NUM), SPI_WR_BIT_ORDER);
  CLEAR_PERI_REG_MASK(SPI_CTRL_REG(SPI_NUM), SPI_RD_BIT_ORDER);
  CLEAR_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_DOUTDIN);
  WRITE_PERI_REG(SPI_USER1_REG(SPI_NUM), 0);
  SET_PERI_REG_BITS(SPI_CTRL2_REG(SPI_NUM), SPI_MISO_DELAY_MODE, 0, SPI_MISO_DELAY_MODE_S);
  CLEAR_PERI_REG_MASK(SPI_SLAVE_REG(SPI_NUM), SPI_SLAVE_MODE);

  WRITE_PERI_REG(SPI_CLOCK_REG(SPI_NUM), (1 << SPI_CLKCNT_N_S) | (1 << SPI_CLKCNT_L_S));//40MHz
  //WRITE_PERI_REG(SPI_CLOCK_REG(SPI_NUM), SPI_CLK_EQU_SYSCLK); // 80Mhz

  SET_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_CS_SETUP | SPI_CS_HOLD | SPI_USR_MOSI);
  SET_PERI_REG_MASK(SPI_CTRL2_REG(SPI_NUM), ((0x4 & SPI_MISO_DELAY_NUM) << SPI_MISO_DELAY_NUM_S));
  CLEAR_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_USR_COMMAND);
  SET_PERI_REG_BITS(SPI_USER2_REG(SPI_NUM), SPI_USR_COMMAND_BITLEN, 0, SPI_USR_COMMAND_BITLEN_S);
  CLEAR_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_USR_ADDR);
  SET_PERI_REG_BITS(SPI_USER1_REG(SPI_NUM), SPI_USR_ADDR_BITLEN, 0, SPI_USR_ADDR_BITLEN_S);
  CLEAR_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_USR_MISO);
  SET_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_USR_MOSI);
  char i;
  for (i = 0; i < 16; ++i) {
      WRITE_PERI_REG((SPI_W0_REG(SPI_NUM) + (i << 2)), 0);
  }
}

void spiWriteByte(const uint8_t data){
    SET_PERI_REG_BITS(SPI_MOSI_DLEN_REG(SPI_NUM), SPI_USR_MOSI_DBITLEN, 0x7, SPI_USR_MOSI_DBITLEN_S);
    WRITE_PERI_REG((SPI_W0_REG(SPI_NUM)), data);
    SET_PERI_REG_MASK(SPI_CMD_REG(SPI_NUM), SPI_USR);
    while (READ_PERI_REG(SPI_CMD_REG(SPI_NUM))&SPI_USR);
}

#define U16x2toU32(m,l) ((((uint32_t)(l>>8|(l&0xFF)<<8))<<16)|(m>>8|(m&0xFF)<<8))
