#include <SPI.h>

/*
 * Basic SPI routines for Sony ECX337A OLED microdisplay
 * 
 * LVDS serializer example: ROHM BU90T82 on pigvision PVS114T1 testbed board
 */

#ifndef ECX_H
#define ECX_H

int xclr;
int xcs;

uint8_t ECX337A_INIT_STANDARD[] = {
  0x01, // First value following is at addr 0x01, and ascending from there
  0x02, // T_SLOPE default, YCB_P default, CALSEL default, LVDS_MAP VESA, MCLKPOL negative
  0x00, // ORBIT_H default
  0x80, // ORBIT_V default
  0x03, // PN_POL A=P/B=N, PINSWP ascending/ascending, PRTSWP IF0,IF1, IFSW 4lane-x2
  0x08, // FORMAT_SEL_DATA 4:4:4, DITHEREN enabled
  0x00, // VD_POL negative, HD_POL negative, OTPCALDAC_REGEN 0, OTPDG_REGEN 0
  0x10, // VD_FILTER 1MCLK, HD_FILTER 1MCLK, C_SLOPE "prompt transition"
};

void ecx_spi_begin() {
  digitalWrite(xcs, LOW);
  delayMicroseconds(1);
  // Clock idles high, change data on falling edge
  SPI.beginTransaction(SPISettings(100000, LSBFIRST, SPI_MODE3));
}

void ecx_spi_end() {
  SPI.endTransaction();
  delayMicroseconds(1);
  digitalWrite(xcs, HIGH);
  delay(1); // arbitrary, don't want it to think we're bursting
}

/**
 * Single burst-mode transaction
 */
void ecx_spi_write8_burst(uint8_t *data, int len) {
  ecx_spi_begin();
  for (int i = 0; i < len; i++) {
    SPI.transfer(data[i]);
  }
  ecx_spi_end();
}

/**
 * Multiple single-register transactions
 */
void ecx_spi_write16_seq(uint16_t *data, int len) {
  for (int i = 0; i < len; i++) {
    ecx_spi_begin();
    SPI.transfer((uint8_t)(data[i] >> 8));
    SPI.transfer((uint8_t)(data[i] & 0x00FF));
    ecx_spi_end();
  }
}

/*
 * Release XCLR and write to "Normal" and "Side A" registers
 */
void ecx_initialize(int xclr_pin, int xcs_pin) {
  xclr = xclr_pin;
  xcs = xcs_pin;
  digitalWrite(xclr, LOW);
  digitalWrite(xcs, HIGH);
  delay(16); // bleh arbitrary
  digitalWrite(xclr, HIGH);
  delay(16); // Spec 16ms from XCLR high to ready in powersave mode
  ecx_spi_write8_burst(ECX337A_INIT_STANDARD, sizeof(ECX337A_INIT_STANDARD));
}

/*
 * Disable PS0/PS1 powersave modes
 */
void ecx_panelon() {
  uint16_t seq[1];
  seq[0] = 0x004D;
  ecx_spi_write16_seq(seq, 1);
  delayMicroseconds(200); // Spec
  seq[0] = 0x004F;
  ecx_spi_write16_seq(seq, 1);
}

/*
 * Enable PS0/PS1 powersave modes
 */
void ecx_paneloff() {
  uint16_t seq[] = {
    0x004D,
    0x004C,
  };
  ecx_spi_write16_seq(seq, 2);
}

/*
 * Set real panel luminance directly. nitsx10 is the brightness
 * in multiples of 10 nits (cd/m^2). Default is 15 (150 nits).
 */
void ecx_luminance(uint8_t nitsx10) {
  nitsx10 = max(nitsx10, (uint8_t)5);
  nitsx10 = min(nitsx10, (uint8_t)100);
  uint16_t seq[] = {
    0x1107, // L_AT_CALEN, L_SEAMLESSEN, WB_CALEN all set
    0x1300 | (uint16_t)nitsx10,
  };
  ecx_spi_write16_seq(seq, 2);
}

/**
 * Set image orbit position; each axis has 10 pixels of orbit
 * space in either direction (for alleviating image retention)
 */
void ecx_orbit(int8_t horiz, int8_t vert) {
  horiz = min(max(horiz, (int8_t)-10), (int8_t)10);
  vert = min(max(horiz, (int8_t)-10), (int8_t)10);
  uint16_t seq[] = {
    0x0200 | (uint16_t)horiz,
    0x0380 | (uint16_t)vert,
  };
  ecx_spi_write16_seq(seq, 2);
}

#endif
