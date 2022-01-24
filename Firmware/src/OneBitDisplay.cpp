//
// OneBitDisplay (OLED+LCD library)
// Copyright (c) 2020 BitBank Software, Inc.
// Written by Larry Bank (bitbank@pobox.com)
// Project started 3/23/2020
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifdef _LINUX_
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#ifndef MEMORY_ONLY
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <math.h>
#include <armbianio.h>
#endif // MEMORY_ONLY
// convert wire library constants into ArmbianIO values
#define OUTPUT GPIO_OUT
#define INPUT GPIO_IN
#define INPUT_PULLUP GPIO_IN_PULLUP
#define HIGH 1
#define LOW 0
void delay(int);
#else // Arduino

#include <Arduino.h>
#ifdef __AVR__
#include <avr/pgmspace.h>
#endif
#include <SPI.h>

#endif // _LINUX_
#include "OneBitDisplay.h"
// All of the drawing code is in here
#include "obd.inl"

// EPD look up tables
// new waveform created by Jean-Marc Zingg for the actual panel
#define T1 25 // color change charge balance pre-phase
#define T2  1 // color change or sustain charge balance pre-phase
#define T3  2 // color change or sustain phase
#define T4 25 // color change phase

const unsigned char lut_20_vcom0_partial[] PROGMEM =
{
  0x00, T1, T2, T3, T4, 1, // 00 00 00 00
  0x00,  1,  0,  0,  0, 1, // gnd phase
};

const unsigned char lut_21_ww_partial[] PROGMEM =
{ // 10 w
  0x18, T1, T2, T3, T4, 1, // 00 01 10 00
  0x00,  1,  0,  0,  0, 1, // gnd phase
};

const unsigned char lut_22_bw_partial[] PROGMEM =
{ // 10 w
  0x5A, T1, T2, T3, T4, 1, // 01 01 10 10
  0x00,  1,  0,  0,  0, 1, // gnd phase
};

const unsigned char lut_23_wb_partial[] PROGMEM =
{ // 01 b
  0xA5, T1, T2, T3, T4, 1, // 10 10 01 01
  0x00,  1,  0,  0,  0, 1, // gnd phase
};

const unsigned char lut_24_bb_partial[] PROGMEM =
{ // 01 b
  0x24, T1, T2, T3, T4, 1, // 00 10 01 00
  0x00,  1,  0,  0,  0, 1, // gnd phase
};

const unsigned char lut_vcom0_full[] =
{
  0x40, 0x17, 0x00, 0x00, 0x00, 0x02,
  0x00, 0x17, 0x17, 0x00, 0x00, 0x02,
  0x00, 0x0A, 0x01, 0x00, 0x00, 0x01,
  0x00, 0x0E, 0x0E, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char lut_ww_full[] =
{
  0x40, 0x17, 0x00, 0x00, 0x00, 0x02,
  0x90, 0x17, 0x17, 0x00, 0x00, 0x02,
  0x40, 0x0A, 0x01, 0x00, 0x00, 0x01,
  0xA0, 0x0E, 0x0E, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char lut_bw_full[] =
{
  0x40, 0x17, 0x00, 0x00, 0x00, 0x02,
  0x90, 0x17, 0x17, 0x00, 0x00, 0x02,
  0x40, 0x0A, 0x01, 0x00, 0x00, 0x01,
  0xA0, 0x0E, 0x0E, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
const unsigned char lut_wb_full[] =
{
  0x80, 0x17, 0x00, 0x00, 0x00, 0x02,
  0x90, 0x17, 0x17, 0x00, 0x00, 0x02,
  0x80, 0x0A, 0x01, 0x00, 0x00, 0x01,
  0x50, 0x0E, 0x0E, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char lut_bb_full[] =
{
  0x80, 0x17, 0x00, 0x00, 0x00, 0x02,
  0x90, 0x17, 0x17, 0x00, 0x00, 0x02,
  0x80, 0x0A, 0x01, 0x00, 0x00, 0x01,
  0x50, 0x0E, 0x0E, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// Initialization sequences
const unsigned char oled128_initbuf[] PROGMEM = {0x00, 0xae,0xdc,0x00,0x81,0x40,
      0xa1,0xc8,0xa8,0x7f,0xd5,0x50,0xd9,0x22,0xdb,0x35,0xb0,0xda,0x12,
      0xa4,0xa6,0xaf};

const unsigned char oled64x128_initbuf[] PROGMEM ={
0x00, 0xae, 0xd5, 0x51, 0x20, 0xa8, 0x3f, 0xdc, 0x00, 0xd3, 0x60, 0xad, 0x80, 0xa6, 0xa4, 0xa0, 0xc0, 0x81, 0x40, 0xd9, 0x22, 0xdb, 0x35, 0xaf
};

const unsigned char oled132_initbuf[] PROGMEM = {0x00,0xae,0x02,0x10,0x40,0x81,0xa0,0xc0,0xa6,0xa8,0x3f,0xd3,0x00,0xd5,0x80,0xd9,0xf1,0xda,0x12,0xdb,0x40,0x20,0x02,0xa4,0xa6};

const unsigned char oled64_initbuf[] PROGMEM ={0x00,0xae,0xa8,0x3f,0xd3,0x00,0x40,0xa1,0xc8,
      0xda,0x12,0x81,0xff,0xa4,0xa6,0xd5,0x80,0x8d,0x14,
      0xaf,0x20,0x02};

const unsigned char oled32_initbuf[] PROGMEM  = {
0x00,0xae,0xd5,0x80,0xa8,0x1f,0xd3,0x00,0x40,0x8d,0x14,0xa1,0xc8,0xda,0x02,
0x81,0x7f,0xd9,0xf1,0xdb,0x40,0xa4,0xa6,0xaf};

const unsigned char oled72_initbuf[] PROGMEM ={0x00,0xae,0xa8,0x3f,0xd3,0x00,0x40,0xa1,0xc8,
      0xda,0x12,0x81,0xff,0xad,0x30,0xd9,0xf1,0xa4,0xa6,0xd5,0x80,0x8d,0x14,
      0xaf,0x20,0x02};

const unsigned char uc1701_initbuf[] PROGMEM  = {0xe2, 0x40, 0xa0, 0xc8, 0xa2, 0x2c, 0x2e, 0x2f, 0xf8, 0x00, 0x23, 0x81, 0x28, 0xac, 0x00, 0xa6};

const unsigned char hx1230_initbuf[] PROGMEM  = {0x2f, 0x90, 0xa6, 0xa4, 0xaf, 0x40, 0xb0, 0x10, 0x00};
const unsigned char nokia5110_initbuf[] PROGMEM  = {0x21, 0xa4, 0xb1, 0x04,0x14,0x20,0x0c};

static void obdCachedFlush(OBDISP *pOBD, int bRender);
static void obdCachedWrite(OBDISP *pOBD, uint8_t *pData, uint8_t u8Len, int bRender);
void obdSetPosition(OBDISP *pOBD, int x, int y, int bRender);
void obdWriteCommand(OBDISP *pOBD, unsigned char c);
void obdWriteDataBlock(OBDISP *pOBD, unsigned char *ucBuf, int iLen, int bRender);
//static void SPI_BitBang(OBDISP *pOBD, uint8_t *pData, int iLen, uint8_t iMOSIPin, uint8_t iSCKPin);

//
// Draw the contents of a memory buffer onto a display
// The sub-window will be clipped if it specifies too large an area
// for the destination display. The source OBDISP structure must have
// a valid back buffer defined
// The top and bottom destination edges will be drawn on byte boundaries (8 rows)
// The source top/bot edges can be on pixel boundaries
// This can be used for partial screen updates
//
void obdDumpWindow(OBDISP *pOBDSrc, OBDISP *pOBDDest, int srcx, int srcy, int destx, int desty, int width, int height)
{
uint8_t *s,ucTemp[32]; // temp buffer to gather source pixels
int x, y, tx, i;
int iPitch;

   if (pOBDSrc == NULL || pOBDDest == NULL || pOBDSrc->ucScreen == NULL)
      return; // invalid pointers
   if (width > pOBDDest->width)
      width = pOBDDest->width;
   if (height > pOBDDest->height)
      height = pOBDDest->height;
   iPitch = pOBDSrc->width;
   for (y=0; y<height; y+=8)
   {
      obdSetPosition(pOBDDest, destx, (desty+y)/8, 1);
      for (x=0; x<width; x+=32)
      {
         tx = 32;
         if (width-x < 32) tx = width-x;
         s = &pOBDSrc->ucScreen[((srcy+y)/8)*iPitch + srcx+x];
	 if (srcy & 7) // need to shift the bits to get 8 rows of src data
         {
            uint8_t uc, ucShift = srcy & 7;
            for (i=0; i<tx; i++)
            { // combine current and next line to capture 8 pixels
               uc = s[0] >> ucShift;
               uc |= s[iPitch] << (7-ucShift);
               ucTemp[i] = uc;
            }
            obdCachedWrite(pOBDDest, ucTemp, tx, 1);
         }
         else
         { // simpler case
            obdCachedWrite(pOBDDest, s, tx, 1); // just copy it
         }
      } // for x
   } // for y
   obdCachedFlush(pOBDDest, 1);
} /* obdDumpWindow() */

//
// Write a single line to a Sharp memory LCD
// You must provide the exact number of bytes needed for a complete line
// e.g. for the 144x168 display, pSrc must provide 144 pixels (18 bytes) 
//
void obdWriteLCDLine(OBDISP *pOBD, uint8_t *pSrc, int iLine)
{
    int x;
    uint8_t c, ucInvert, *d, ucStart;
    uint8_t ucLineBuf[54]; // 400 pixels is max supported width = 50 bytes + 4
    int iPitch = pOBD->width / 8;
    static int iVCOM = 0;

//    if (pOBD == NULL || pSrc == NULL || pOBD->type < SHARP_144x168)
//        return; // invalid request
    if (iLine < 0 || iLine >= pOBD->height)
        return;
    
      ucInvert = (pOBD->invert) ? 0x00 : 0xff;
      digitalWrite(pOBD->iCSPin, HIGH); // active high

      ucStart = 0x80; // write command
      iVCOM++;
      if (iVCOM & 0x100) // flip it every 256 lines
        ucStart |= 0x40; // VCOM bit
      ucLineBuf[1] = ucStart;
      // this code assumes I2C, so the first byte is ignored
      _I2CWrite(pOBD, ucLineBuf, 2); // write command(01) + vcom(02)

     d = &ucLineBuf[2];
     ucLineBuf[1] = pgm_read_byte(&ucMirror[iLine+1]); // current line number
     for (x=0; x<iPitch; x++)
     {
         c = pSrc[0] ^ ucInvert; // we need to brute-force invert it
         *d++ = pgm_read_byte(&ucMirror[c]);
         pSrc++;
     } // for x
    // write this line to the display
    ucLineBuf[iPitch+2] = 0; // end of line
    _I2CWrite(pOBD, ucLineBuf, iPitch+3);
    ucLineBuf[1] = 0;
    _I2CWrite(pOBD, ucLineBuf, 2); // final transfer
    digitalWrite(pOBD->iCSPin, LOW); // de-activate
} /* obdWriteLCDLine() */

//
// Turn the display on or off
//
void obdPower(OBDISP *pOBD, int bOn)
{
uint8_t ucCMD;

  if (pOBD->type == LCD_NOKIA5110)
    ucCMD = (bOn) ? 0x20 : 0x24;
  else // all other supported displays
    ucCMD = (bOn) ? 0xaf : 0xae;
  obdWriteCommand(pOBD, ucCMD);
} /* obdPower() */

// Controls the LED backlight
void obdBacklight(OBDISP *pOBD, int bOn)
{
  if (pOBD->iLEDPin != 0xff)
  {
     digitalWrite(pOBD->iLEDPin, (bOn) ? HIGH:LOW);
  }
} /* obdBacklight() */

//
// Send the command sequence to power up the LCDs
static void LCDPowerUp(OBDISP *pOBD)
{
#ifndef MEMORY_ONLY
    int iLen;
    uint8_t *s, uc[32];
    obdSetDCMode(pOBD, MODE_COMMAND);
    digitalWrite(pOBD->iCSPin, LOW);
    if (pOBD->type == LCD_UC1701 || pOBD->type == LCD_UC1609)
    {
        s = (uint8_t *)uc1701_initbuf;
        iLen = sizeof(uc1701_initbuf);
    }
    else if (pOBD->type == LCD_HX1230)
    {
        s = (uint8_t *)hx1230_initbuf;
        iLen = sizeof(hx1230_initbuf);
    }
    else // Nokia 5110
    {
        s = (uint8_t *)nokia5110_initbuf;
        iLen = sizeof(nokia5110_initbuf);
    }
    memcpy_P(uc, s, iLen);
#ifdef _LINUX_
    AIOWriteSPI(pOBD->bbi2c.file_i2c, uc, iLen);    
#else
    if (pOBD->iMOSIPin == 0xff)
       SPI.transfer(uc, iLen);
    else
       SPI_BitBang(pOBD, uc, iLen, pOBD->iMOSIPin, pOBD->iCLKPin);
#endif
    delay(100);
    obdWriteCommand(pOBD, 0xa5);
    delay(100);
    obdWriteCommand(pOBD, 0xa4);
    obdWriteCommand(pOBD, 0xaf);
    digitalWrite(pOBD->iCSPin, HIGH);
    obdSetDCMode(pOBD, MODE_DATA);
#endif // MEMORY_ONLY
} /* LCDPowerUp() */

//
// Initialize the display controller on an SPI bus
//
void obdSPIInit(OBDISP *pOBD, int iType, int iDC, int iCS, int iReset, int iMOSI, int iCLK, int iLED, int bFlip, int bInvert, int bBitBang, int32_t iSpeed)
{
#ifndef MEMORY_ONLY
uint8_t uc[32], *s;
int iLen;

  pOBD->ucScreen = NULL; // start with no backbuffer; user must provide one later
  pOBD->iDCPin = iDC;
  pOBD->iCSPin = iCS;
  pOBD->iRSTPin = iReset;
  pOBD->iMOSIPin = iMOSI;
  pOBD->iCLKPin = iCLK;
  pOBD->iLEDPin = iLED;
  pOBD->type = iType;
  pOBD->flip = bFlip;
  pOBD->invert = bInvert;
  pOBD->wrap = 0; // default - disable text wrap
  pOBD->com_mode = COM_SPI; // communication mode
  if (pOBD->iDCPin != 0xff) // Note - not needed on Sharp Memory LCDs
  {
      pinMode(pOBD->iDCPin, OUTPUT);
      digitalWrite(pOBD->iDCPin, 0); // for some reason, command mode must be set or some OLEDs/LCDs won't initialize correctly even if set later
  }
  pinMode(pOBD->iCSPin, OUTPUT);
  digitalWrite(pOBD->iCSPin, 0); //(pOBD->type < SHARP_144x168)); // set to not-active
  if (bBitBang)
  {
      pinMode(iMOSI, OUTPUT);
      pinMode(iCLK, OUTPUT);
  }

  // Reset it
  if (iReset != -1)
  {
    pinMode(iReset, OUTPUT);
    digitalWrite(iReset, HIGH);
    delay(100);
    digitalWrite(iReset, LOW);
    delay(10);
    digitalWrite(iReset, HIGH);
    delay(100);
  }
  if (iLED != -1)
  {
      if (iType >= EPD42_400x300) // for EPD displays, LED = BUSY
         pinMode(iLED, INPUT);
      else
         pinMode(iLED, OUTPUT);
  }
// Initialize SPI
    if (!bBitBang) {
        pOBD->iMOSIPin = 0xff; // mark it as hardware SPI
#ifdef _LINUX_
	pOBD->bbi2c.file_i2c = AIOOpenSPI(SPI_BUS_NUMBER, iSpeed);
#else
        SPI.begin();
        SPI.beginTransaction(SPISettings(iSpeed, MSBFIRST, SPI_MODE0));
#endif
	//  SPI.setClockDivider(16);
       //  SPI.setBitOrder(MSBFIRST);
        //  SPI.setDataMode(SPI_MODE0);
    }

  pOBD->width = 128; // assume 128x64
  pOBD->height = 64;
  if (iType == SHARP_144x168)
  {
      pOBD->width = 144;
      pOBD->height = 168;
      pOBD->iDCPin = 0xff; // no D/C wire on this display
  }
  else if (iType == SHARP_400x240)
  {
      pOBD->width = 400;
      pOBD->height = 240;
      pOBD->iDCPin = 0xff; // no D/C wire on this display
  }
  else if (iType == EPD42_400x300)
  {
      pOBD->width = 400;
      pOBD->height = 300;
      return; // nothing else to do yet
  }
  else if (iType == LCD_UC1609)
  {
      pOBD->width = 192;
      pOBD->height = 64;
  }
  else if (iType == LCD_HX1230)
  {
      pOBD->width = 96;
      pOBD->height = 68;
      pOBD->iDCPin = 0xff; // flag this as being 3-wire SPI
  }
  else if (iType == LCD_NOKIA5110)
  {
      pOBD->width = 84;
      pOBD->height = 48;
  }
  else if (iType == OLED_96x16)
  {
    pOBD->width = 96;
    pOBD->height = 16;
  }
  else if (iType == OLED_64x128)
  {
    pOBD->width = 64;
    pOBD->height = 128;
  }
  else if (iType == OLED_128x32)
    pOBD->height = 32;
  else if (iType == OLED_128x128)
    pOBD->height = 128;
  else if (iType == OLED_64x32)
  {
    pOBD->width = 64;
    pOBD->height = 32;
  }
  else if (iType == OLED_72x40)
  {
    pOBD->width = 72;
    pOBD->height = 40;
  }
  if (iType == OLED_128x32 || iType == OLED_96x16)
  {
     s = (uint8_t *)oled32_initbuf;
     iLen = sizeof(oled32_initbuf);
  }
  else if (iType == OLED_64x128)
  {
     s = (uint8_t *)oled64x128_initbuf;
     iLen = sizeof(oled64x128_initbuf);
  }
  else if (iType == OLED_128x128)
  {
     s = (uint8_t *)oled128_initbuf;
     iLen = sizeof(oled128_initbuf);
  }
  else if (iType == OLED_132x64) { // SH1106
     s = (uint8_t *)oled132_initbuf;
     iLen = sizeof(oled132_initbuf);
  }
  else if (iType < LCD_UC1701) // 128x64 and 64x32
  {
     s = (uint8_t *)oled64_initbuf;
     iLen = sizeof(oled64_initbuf);
  }
    // OLED
  if (iType < LCD_UC1701)
  {
      memcpy_P(uc, s, iLen); // do it from RAM
      _I2CWrite(pOBD, uc, iLen);
      delay(100); // on SPI display this delay is needed or the display
      // never sees the "display on" command at the end of the sequence
      obdWriteCommand(pOBD, 0xaf); // turn on display
      if (bInvert)
      {
        uc[0] = 0; // command
        uc[1] = 0xa7; // invert command
        _I2CWrite(pOBD, uc, 2);
      }
      if (bFlip) // rotate display 180
      {
        uc[0] = 0; // command
        uc[1] = 0xa0;
        _I2CWrite(pOBD, uc, 2);
        uc[0] = 0;
        uc[1] = 0xc0;
        _I2CWrite(pOBD, uc, 2);
      }
  } // OLED
  if (iType == LCD_UC1701 || iType == LCD_HX1230)
  {
      uint8_t cCOM = 0xc0;
      
      LCDPowerUp(pOBD);
      if (iType == LCD_HX1230)
      {
          obdSetContrast(pOBD, 0); // contrast of 0 looks good
          cCOM = 0xc8;
      }
      if (bFlip) // flip horizontal + vertical
      {
         obdWriteCommand(pOBD, 0xa1); // set SEG direction (A1 to flip horizontal)
         obdWriteCommand(pOBD, cCOM); // set COM direction (C0 to flip vert)
      }
      if (bInvert)
      {
         obdWriteCommand(pOBD, 0xa7); // set inverted pixel mode
      }
  }
  if (iType == LCD_UC1609)
  {
      LCDPowerUp(pOBD);
      return;
      obdWriteCommand(pOBD, 0xe2); // system reset
      delay(100);
      obdWriteCommand(pOBD, 0xa0); // set frame rate to 76fps
      obdWriteCommand(pOBD, 0xeb); // set BR
      obdWriteCommand(pOBD, 0x2f); // set Power Control
      obdWriteCommand(pOBD, 0xc4); // set LCD mapping control
      obdWriteCommand(pOBD, 0x81); // set PM
      obdWriteCommand(pOBD, 0x90); // set contrast to 144
      delay(100);
      obdWriteCommand(pOBD, 0xaf); // display enable
      if (bFlip) // flip horizontal + vertical
      {  
         obdWriteCommand(pOBD, 0xa1); // set SEG direction (A1 to flip horizontal)       
         obdWriteCommand(pOBD, 0xc2); // set COM direction (C0 to flip vert)
      }
      if (bInvert)
      {
         obdWriteCommand(pOBD, 0xa7); // set inverted pixel mode
      }
  } // UC1609
#endif // MEMORY_ONLY
} /* obdSPIInit() */
//
// Set the memory configuration to display the pixels at 0 or 180 degrees (flipped)
// Pass true (1) to flip 180, false (0) to set to 0
//
void obdSetFlip(OBDISP *pOBD, int iOnOff)
{
   if (pOBD == NULL) return;
   pOBD->flip = iOnOff;
   if (pOBD->type == LCD_UC1701 || pOBD->type == LCD_UC1609)
   {
      if (iOnOff) // rotate display 180
      {
         obdWriteCommand(pOBD, 0xa1); // SEG direction (A1 to flip horizontal) 
         obdWriteCommand(pOBD, 0xc0); // COM direction (C0 to flip vert)
      } else { // non-rotated
         obdWriteCommand(pOBD, 0xa0);       
         obdWriteCommand(pOBD, 0xc8);
      }
   } else { // OLEDs
      if (iOnOff)
      {
	 obdWriteCommand(pOBD, 0xa0);
	 obdWriteCommand(pOBD, 0xc0);
      } else {
	 obdWriteCommand(pOBD, 0xa1);
	 obdWriteCommand(pOBD, 0xc8);
      }
   }
} /* obdSetFlip() */
//
// Initializes the OLED controller into "page mode"
//
int obdI2CInit(OBDISP *pOBD, int iType, int iAddr, int bFlip, int bInvert, int bWire, int sda, int scl, int reset, int32_t iSpeed)
{
int rc = OLED_NOT_FOUND;
#ifndef MEMORY_ONLY
unsigned char uc[32];
uint8_t u8Len, *s;

  pOBD->ucScreen = NULL; // reset backbuffer; user must provide one later
  pOBD->type = iType;
  pOBD->flip = bFlip;
  pOBD->invert = bInvert;
  pOBD->wrap = 0; // default - disable text wrap
  pOBD->bbi2c.iSDA = sda;
  pOBD->bbi2c.iSCL = scl;
  pOBD->bbi2c.bWire = bWire;
  pOBD->com_mode = COM_I2C; // communication mode

  I2CInit(&pOBD->bbi2c, iSpeed); // on Linux, SDA = bus number, SCL = device address
#ifdef _LINUX_
  pOBD->oled_addr = (uint8_t)scl;
#else
  // Reset it
  if (reset != -1)
  {
    pinMode(reset, OUTPUT);
    digitalWrite(reset, HIGH);
    delay(50);
    digitalWrite(reset, LOW);
    delay(50);
    digitalWrite(reset, HIGH);
    delay(10);
  }
  // find the device address if requested
  if (iAddr == -1 || iAddr == 0 || iAddr == 0xff) // find it
  {
    I2CTest(&pOBD->bbi2c, 0x3c);
    if (I2CTest(&pOBD->bbi2c, 0x3c))
       pOBD->oled_addr = 0x3c;
    else if (I2CTest(&pOBD->bbi2c, 0x3d))
       pOBD->oled_addr = 0x3d;
    else
       return rc; // no display found!
  }
  else
  {
    pOBD->oled_addr = iAddr;
    I2CTest(&pOBD->bbi2c, iAddr);
    if (!I2CTest(&pOBD->bbi2c, iAddr))
       return rc; // no display found
  }
#endif
  // Detect the display controller (SSD1306, SH1107 or SH1106)
  uint8_t u = 0;
  I2CReadRegister(&pOBD->bbi2c, pOBD->oled_addr, 0x00, &u, 1); // read the status register
  u &= 0x0f; // mask off power on/off bit
  if ((u == 0x7 || u == 0xf) && pOBD->type == OLED_128x128) // SH1107
  { // A single SSD1306 display returned 7, so only act on it if the
    // user specified that they're working with a 128x128 display
    rc = OLED_SH1107_3C;
    bFlip = !bFlip; // SH1107 seems to have this reversed from the usual direction
  }
  else if (u == 0x8) // SH1106
  {
    rc = OLED_SH1106_3C;
    pOBD->type = OLED_132x64; // needs to be treated a little differently
  }
  else if (u == 3 || u == 6 || u == 7) // 7=128x64(rare),6=128x64 display, 3=smaller
  {
    rc = OLED_SSD1306_3C;
  }
  if (pOBD->oled_addr == 0x3d)
     rc++; // return the '3D' version of the type

  if (iType == OLED_128x32 || iType == OLED_96x16)
  {
      s = (uint8_t *)oled32_initbuf;
      u8Len = sizeof(oled32_initbuf);
  }
  else if (iType == OLED_128x128)
  {
      s = (uint8_t *)oled128_initbuf;
      u8Len = sizeof(oled128_initbuf);
  }
  else if (iType == OLED_72x40)
  {
      s = (uint8_t *)oled72_initbuf;
      u8Len = sizeof(oled72_initbuf);
  }
  else if (iType == OLED_64x128)
  {
      s = (uint8_t *)oled64x128_initbuf;
      u8Len = sizeof(oled64x128_initbuf);
  }
  else // 132x64, 128x64 and 64x32
  {
      s = (uint8_t *)oled64_initbuf;
      u8Len = sizeof(oled64_initbuf);
  }

    memcpy_P(uc, s, u8Len);
  _I2CWrite(pOBD, uc, u8Len);
  if (bInvert)
  {
    uc[0] = 0; // command
    uc[1] = 0xa7; // invert command
    _I2CWrite(pOBD,uc, 2);
  }
  if (bFlip) // rotate display 180
  {
    uc[0] = 0; // command
    uc[1] = 0xa0;
    _I2CWrite(pOBD,uc, 2);
    uc[1] = 0xc0;
    _I2CWrite(pOBD,uc, 2);
  }
  pOBD->width = 128; // assume 128x64
  pOBD->height = 64;
  if (iType == OLED_96x16)
  {
    pOBD->width = 96;
    pOBD->height = 16;
  }
  else if (iType == OLED_64x128)
  {
    pOBD->width = 64;
    pOBD->height = 128;
  }
  else if (iType == OLED_128x32)
    pOBD->height = 32;
  else if (iType == OLED_128x128)
    pOBD->height = 128;
  else if (iType == OLED_64x32)
  {
    pOBD->width = 64;
    pOBD->height = 32;
  }
  else if (iType == OLED_72x40)
  {
    pOBD->width = 72;
    pOBD->height = 40;
  }
#endif // MEMORY_ONLY
  return rc;
} /* obdInit() */
//
// Sends a command to turn on or off the OLED display
//
void oledPower(OBDISP *pOBD, uint8_t bOn)
{
    if (bOn)
      obdWriteCommand(pOBD, 0xaf); // turn on OLED
    else
      obdWriteCommand(pOBD, 0xae); // turn off OLED
} /* oledPower() */

#ifdef _LINUX_
void delay(int iDelay)
{
	usleep(iDelay * 1000);
} /* delay() */
void delayMicroseconds(int iDelay)
{
	usleep(iDelay);
} /* delayMicroseconds() */
#endif // _LINUX_
//
// Bit Bang the data on GPIO pins
//
void SPI_BitBang(OBDISP *pOBD, uint8_t *pData, int iLen, uint8_t iMOSIPin, uint8_t iSCKPin)
{
int i;
uint8_t c;
// We can access the GPIO ports much quicker on AVR by directly manipulating
// the port registers
#ifdef __AVR__
volatile uint8_t *outSCK, *outMOSI; // port registers for fast I/O
uint8_t port, bitSCK, bitMOSI; // bit mask for the chosen pins

    port = digitalPinToPort(iMOSIPin);
    outMOSI = portOutputRegister(port);
    bitMOSI = digitalPinToBitMask(iMOSIPin);
    port = digitalPinToPort(iSCKPin);
    outSCK = portOutputRegister(port);
    bitSCK = digitalPinToBitMask(iSCKPin);

#endif

   while (iLen)
   {
      c = *pData++;
      if (pOBD->iDCPin == 0xff) // 3-wire SPI, write D/C bit first
      {
#ifdef __AVR__
          if (pOBD->mode == MODE_DATA)
             *outMOSI |= bitMOSI;
          else
             *outMOSI &= ~bitMOSI;
          *outSCK |= bitSCK; // toggle clock
          *outSCK &= ~bitSCK; // no delay needed on SPI devices since AVR is slow
#else
          digitalWrite(iMOSIPin, (pOBD->mode == MODE_DATA));
          digitalWrite(iSCKPin, HIGH);
          delayMicroseconds(0);
	  digitalWrite(iSCKPin, LOW);
#endif
      }
      if (c == 0 || c == 0xff) // quicker for all bits equal
      {
#ifdef __AVR__
         if (c & 1)
            *outMOSI |= bitMOSI;
         else
            *outMOSI &= ~bitMOSI;
         for (i=0; i<8; i++)
         {
            *outSCK |= bitSCK;
            *outSCK &= ~bitSCK;
         }
#else
         digitalWrite(iMOSIPin, (c & 1));
         for (i=0; i<8; i++)
         {
            digitalWrite(iSCKPin, HIGH);
            delayMicroseconds(0);
            digitalWrite(iSCKPin, LOW);
         }
#endif
      }
      else
      {
         for (i=0; i<8; i++)
         {
#ifdef __AVR__
            if (c & 0x80) // MSB first
               *outMOSI |= bitMOSI;
            else
               *outMOSI &= ~bitMOSI;
            *outSCK |= bitSCK;
            c <<= 1;
            *outSCK &= ~bitSCK;
#else
            digitalWrite(iMOSIPin,  (c & 0x80) != 0); // MSB first
            digitalWrite(iSCKPin, HIGH);
            c <<= 1;
            delayMicroseconds(0);
            digitalWrite(iSCKPin, LOW);
#endif
        }
      }
      iLen--;
   }
} /* SPI_BitBang() */

// Sets the D/C pin to data or command mode
void obdSetDCMode(OBDISP *pOBD, int iMode)
{
  if (pOBD->iDCPin == 0xff) // 9-bit SPI
      pOBD->mode = (uint8_t)iMode;
  else // set the GPIO line
      digitalWrite(pOBD->iDCPin, (iMode == MODE_DATA));
} /* obdSetDCMode() */

static void obdWriteCommand2(OBDISP *pOBD, unsigned char c, unsigned char d)
{
unsigned char buf[4];

    if (pOBD->com_mode == COM_I2C) {// I2C device
        buf[0] = 0x00;
        buf[1] = c;
        buf[2] = d;
        _I2CWrite(pOBD, buf, 3);
    } else { // must be SPI
        obdWriteCommand(pOBD, c);
        obdWriteCommand(pOBD, d);
    }
} /* obdWriteCommand2() */

//
// Sets the brightness (0=off, 255=brightest)
//
void obdSetContrast(OBDISP *pOBD, unsigned char ucContrast)
{
  if (pOBD->type == LCD_HX1230)
  { // valid values are 0-31, so scale it
      ucContrast >>= 3;
      obdWriteCommand(pOBD, 0x80 + ucContrast);
  }
  else if (pOBD->type == LCD_NOKIA5110)
  {
      // we allow values of 0xb0-0xbf, so shrink the range
      ucContrast >>= 4;
      obdWriteCommand(pOBD, 0x21); // set advanced command mode
      obdWriteCommand(pOBD, 0xb0 | ucContrast);
      obdWriteCommand(pOBD, 0x20); // set simple command mode
  }
  else // OLEDs + UC1701
      obdWriteCommand2(pOBD, 0x81, ucContrast);
} /* obdSetContrast() */
// DEBUG - try 5000ms for now
#define EPD_BUSY_TIMEOUT 5000

static void EPDWaitBusy(OBDISP *pOBD)
{
#ifndef MEMORY_ONLY
int iTimeout = 0;

  while (1) {
     if (digitalRead(pOBD->iLEDPin) == HIGH)
         break;
     iTimeout += 10;
     if (iTimeout > EPD_BUSY_TIMEOUT)
         break; // DEBUG - timeout
     delay(10);
  }
#endif // MEMORY_ONLY
} /* EPDWaitBusy() */

//
// Init a full display update
//
static void EPDInitFullUpdate(OBDISP *pOBD)
{
uint8_t ucTemp[64];

  ucTemp[0] = 0x40; // data mode
  obdWriteCommand(pOBD, 0x00);
  ucTemp[1] = 0x3F; //300x400 B/W mode, LUT set by register
  _I2CWrite(pOBD, ucTemp, 2);
  obdWriteCommand(pOBD, 0x20); //vcom
  memcpy(&ucTemp[1], lut_vcom0_full, sizeof(lut_vcom0_full));
  _I2CWrite(pOBD, ucTemp, sizeof(lut_vcom0_full) + 1);
  obdWriteCommand(pOBD, 0x21); //ww --
  memcpy(&ucTemp[1], lut_ww_full, sizeof(lut_ww_full));
  _I2CWrite(pOBD, ucTemp, sizeof(lut_ww_full) + 1);
  obdWriteCommand(pOBD, 0x22); //bw r
  memcpy(&ucTemp[1], lut_bw_full, sizeof(lut_bw_full));
  _I2CWrite(pOBD, ucTemp, sizeof(lut_bw_full) + 1);
  obdWriteCommand(pOBD, 0x23); //wb w
  memcpy(&ucTemp[1], lut_wb_full, sizeof(lut_wb_full));
  _I2CWrite(pOBD, ucTemp, sizeof(lut_wb_full) + 1);
  obdWriteCommand(pOBD, 0x24); //bb b
  memcpy(&ucTemp[1], lut_bb_full, sizeof(lut_bb_full));
  _I2CWrite(pOBD, ucTemp, sizeof(lut_bb_full) + 1);

} /* EPDInitFullUpdate() */

//
// Wake up the EPD controller
//
static void EPDWakeUp(OBDISP *pOBD)
{
uint8_t ucTemp[8];

  ucTemp[0] = 0x40; // tell I2CWrite that it's a data write, not command
  if (pOBD->iRSTPin != -1)
  {
    digitalWrite(pOBD->iRSTPin, LOW);
    delay(10);
    digitalWrite(pOBD->iRSTPin, HIGH);
    delay(10);
  }
  obdWriteCommand(pOBD, 0x01); // POWER SETTING
  ucTemp[1] = 0x03;   // VDS_EN, VDG_EN internal
  ucTemp[2] = 0x00;   // VCOM_HV, VGHL_LV=16V
  ucTemp[3] = 0x2b;   // VDH=11V
  ucTemp[4] = 0x2b;   // VDL=11V
  ucTemp[5] = 0xff;   // VDHR
  _I2CWrite(pOBD, ucTemp, 6);
  obdWriteCommand(pOBD, 0x06); // boost soft start
  ucTemp[1] = 0x17;   // A
  ucTemp[2] = 0x17;   // B
  ucTemp[3] = 0x17;   // C
  _I2CWrite(pOBD, ucTemp, 4);
  obdWriteCommand(pOBD, 0x00); // panel setting
  ucTemp[1] = 0x3f;
  _I2CWrite(pOBD, ucTemp, 2);  // 300x400 B/W mode, LUT set by register
  obdWriteCommand(pOBD, 0x30); // PLL setting
  ucTemp[1] = 0x3a; // 3a 100HZ   29 150Hz 39 200HZ 31 171HZ
  _I2CWrite(pOBD, ucTemp, 2);
  obdWriteCommand(pOBD, 0x61); // resolution setting
  ucTemp[1] = (uint8_t)(pOBD->width >> 8);
  ucTemp[2] = (uint8_t)(pOBD->width);
  ucTemp[3] = (uint8_t)(pOBD->height >> 8);
  ucTemp[4] = (uint8_t)(pOBD->height);
  _I2CWrite(pOBD, ucTemp, 5);
  obdWriteCommand(pOBD, 0x82); // vcom_DC setting
  //IO.writeDataTransaction(0x08);   // -0.1 + 8 * -0.05 = -0.5V from demo
  ucTemp[1] = 0x12;   // -0.1 + 18 * -0.05 = -1.0V from OTP, slightly better
  _I2CWrite(pOBD, ucTemp, 2);
  //IO.writeDataTransaction(0x1c);   // -0.1 + 28 * -0.05 = -1.5V test, worse
  obdWriteCommand(pOBD, 0x50); // VCOM AND DATA INTERVAL SETTING
  //IO.writeDataTransaction(0x97);    // WBmode:VBDF 17|D7 VBDW 97 VBDB 57   WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7
  ucTemp[1] = 0xd7;  // border floating to avoid flashing
  _I2CWrite(pOBD, ucTemp, 2);
  obdWriteCommand(pOBD, 0x04);
  EPDWaitBusy(pOBD); // power on  
  EPDInitFullUpdate(pOBD);

} /* EPDWakeUp() */

static void EPDSleep(OBDISP *pOBD)
{
uint8_t ucTemp[4];

  ucTemp[0] = 0x40;
  obdWriteCommand(pOBD, 0x50); // border floating
  ucTemp[1] = 0x17; // power off
  _I2CWrite(pOBD, ucTemp, 2);
  obdWriteCommand(pOBD, 0x02); // power off
  EPDWaitBusy(pOBD);
  if (pOBD->iRSTPin != -1)
  {
    obdWriteCommand(pOBD, 0x07); // deep sleep
    ucTemp[1] = 0xa5;
    _I2CWrite(pOBD, ucTemp, 2);
  }

} /* EPDSleep() */

void EPDInitPartialUpdate(OBDISP *pOBD)
{
uint8_t ucTemp[64];

  memset(&ucTemp[1], 0, sizeof(ucTemp)-1); // tables need to be 44/42 bytes, so fill unused parts with 0
  ucTemp[0] = 0x40; // data mode
  obdWriteCommand(pOBD, 0x00);
  ucTemp[1] = 0x3F; //300x400 B/W mode, LUT set by register
  _I2CWrite(pOBD, ucTemp, 2);
  obdWriteCommand(pOBD, 0x20); //vcom
  memcpy(&ucTemp[1], lut_20_vcom0_partial, sizeof(lut_20_vcom0_partial));
  _I2CWrite(pOBD, ucTemp, 45);
  obdWriteCommand(pOBD, 0x21); //ww --
  memset(&ucTemp[1], 0, sizeof(ucTemp)-1);
  memcpy(&ucTemp[1], lut_21_ww_partial, sizeof(lut_21_ww_partial));
  _I2CWrite(pOBD, ucTemp, 43);
  obdWriteCommand(pOBD, 0x22); //bw r
  memset(&ucTemp[1], 0, sizeof(ucTemp)-1);
  memcpy(&ucTemp[1], lut_22_bw_partial, sizeof(lut_22_bw_partial));
  _I2CWrite(pOBD, ucTemp, 43);
  obdWriteCommand(pOBD, 0x23); //wb w
  memset(&ucTemp[1], 0, sizeof(ucTemp)-1);
  memcpy(&ucTemp[1], lut_23_wb_partial, sizeof(lut_23_wb_partial));
  _I2CWrite(pOBD, ucTemp, 43);
  obdWriteCommand(pOBD, 0x24); //bb b
  memset(&ucTemp[1], 0, sizeof(ucTemp)-1);
  memcpy(&ucTemp[1], lut_24_bb_partial, sizeof(lut_24_bb_partial));
  _I2CWrite(pOBD, ucTemp, 43);
} /* EPDInitPartialUpdate() */
//
// Set the boundaries of the partial update area
// start/end x/y
//
void EPDSetPartialArea(OBDISP *pOBD, int sx, int sy, int ex, int ey)
{
uint8_t ucTemp[12];

  ucTemp[0] = 0x40; // data operation
  sx &= 0xFFF8; // byte boundary
  ex = (ex - 1) | 0x7; // byte boundary - 1
  obdWriteCommand(pOBD, 0x90); // partial window
  ucTemp[1] = (uint8_t)(sx / 256);
  ucTemp[2] = (uint8_t)(sx & 256);
  ucTemp[3] = (uint8_t)(ex / 256);
  ucTemp[4] = (uint8_t)(ex & 256);
  ucTemp[5] = (uint8_t)(sy / 256);
  ucTemp[6] = (uint8_t)(sy & 256);
  ucTemp[7] = (uint8_t)(ey / 256);
  ucTemp[8] = (uint8_t)(ey & 256);
  ucTemp[9] = 0x01; // gates scan inside and outside the partial area (don't see any difference)
  _I2CWrite(pOBD, ucTemp, 10);
  delay(2);
  //IO.writeDataTransaction(0x00); // don't see any difference
  //return (7 + xe - x) / 8; // number of bytes to transfer
} /* EPDSetPartialArea() */
//
// Special case for e-ink displays
//
static void EPDDumpPartial(OBDISP *pOBD, uint8_t *pBuffer, int x, int y, int cx, int cy)
{
uint8_t ucLine[52];
int tx, ty;
uint8_t *s, *d, ucSrcMask, ucDstMask, uc;

   if (pBuffer == NULL)
      pBuffer = pOBD->ucScreen;

   x &= 0xfff8; // must start on a byte boundary
   cx = (cx+7) & 0xfff8; // must be whole bytes
   EPDWakeUp(pOBD);
   EPDInitPartialUpdate(pOBD);

#ifdef EXPERIMENT
// DEBUG - erase both RAM areas and refresh first
   obdWriteCommand(pOBD, 0x91); // partial in
   EPDSetPartialArea(pOBD, 0, 0, 399, 299);
   obdWriteCommand(pOBD, 0x13); // write white data
   ucLine[0] = 0x40; // data
   memset(&ucLine[1], 0xff, 50);
   for (ty=0; ty<400; ty++) {
      _I2CWrite(pOBD, ucLine, 51);
   }
   obdWriteCommand(pOBD, 0x10); // write "old" RAM area to same color
   for (ty=0; ty<400; ty++) {
      _I2CWrite(pOBD, ucLine, 51);
   }
   obdWriteCommand(pOBD, 0x12); // refresh
   EPDWaitBusy(pOBD);
   obdWriteCommand(pOBD, 0x92); // partial out

// end of experiment
#endif

   obdWriteCommand(pOBD, 0x91); // partial in
   EPDSetPartialArea(pOBD, x, y, x+cx-1, y+cy-1);
   obdWriteCommand(pOBD, 0x13); // write image data
   // Convert the bit direction and write the data to the EPD
   ucLine[0] = 0x40;
   for (ty=y; ty<y+cy; ty++) {
     ucSrcMask = 1 << (ty & 7);
     ucDstMask = 0x80;
     uc = 0xff;
     d = &ucLine[1];
     s = &pBuffer[(ty >> 3) * pOBD->width];
     for (tx=x; tx<x+cx; tx++) {
        if (s[tx] & ucSrcMask) // src pixel set
           uc &= ~ucDstMask;
        ucDstMask >>= 1;
        if (ucDstMask == 0) { // completed byte
           *d++ = uc;
           uc = 0xff;
           ucDstMask = 0x80;
        }
     } // for tx
     // this code assumes I2C, so the first byte sets the D/C mode
     _I2CWrite(pOBD, ucLine, 51);
   } // for ty
   delay(2);
   obdWriteCommand(pOBD, 0x92); // partial out
   obdWriteCommand(pOBD, 0x12); // display refresh
   EPDWaitBusy(pOBD);

// do it again
   obdWriteCommand(pOBD, 0x91); // partial in
   EPDSetPartialArea(pOBD, x, y, x+cx-1, y+cy-1);
   obdWriteCommand(pOBD, 0x13); // write image data
   // Convert the bit direction and write the data to the EPD
   ucLine[0] = 0x40;
   for (ty=y; ty<y+cy; ty++) {
     ucSrcMask = 1 << (ty & 7);
     ucDstMask = 0x80;
     uc = 0;
     d = &ucLine[1];
     s = &pBuffer[(ty >> 3) * pOBD->width];
     for (tx=x; tx<x+cx; tx++) {
        if (s[tx] & ucSrcMask) // src pixel set
           uc |= ucDstMask;
        ucDstMask >>= 1;
        if (ucDstMask == 0) { // completed byte
           *d++ = uc;
           uc = 0xff;
           ucDstMask = 0x80;
        }
     } // for tx
     // this code assumes I2C, so the first byte sets the D/C mode
     _I2CWrite(pOBD, ucLine, 51);
   } // for ty
   obdWriteCommand(pOBD, 0x92); // partial out
   obdWriteCommand(pOBD, 0x12); // display refresh
   EPDWaitBusy(pOBD);

   EPDSleep(pOBD);
} /* EPDDumpPartial() */

//
// Special case for e-ink displays
//
static void EPDDumpBuffer(OBDISP *pOBD, uint8_t *pBuffer)
{
uint8_t ucLine[52]; // send the data one line at a time
int x, y;
uint8_t *s, *d, ucSrcMask, ucDstMask, uc;

  if (pBuffer == NULL)
     pBuffer = pOBD->ucScreen;

  EPDWakeUp(pOBD);
  obdWriteCommand(pOBD, 0x13); // send buffer
  ucLine[0] = 0x40; // tell _I2CWrite that it's data
  // Convert the bit direction and write the data to the EPD
  for (y=0; y<pOBD->height; y++) {
     ucSrcMask = 1 << (y & 7);
     ucDstMask = 0x80;
     uc = 0xff;
     d = &ucLine[1];
     s = &pBuffer[(y >> 3) * pOBD->width];
     for (x=0; x<pOBD->width; x++) {
        if (s[x] & ucSrcMask) // src pixel set
           uc &= ~ucDstMask;
        ucDstMask >>= 1;
        if (ucDstMask == 0) { // completed byte
           *d++ = uc;
           uc = 0xff;
           ucDstMask = 0x80;
        }
     } // for x
     // this code assumes I2C, so the first byte sets the D/C mode
     _I2CWrite(pOBD, ucLine, 51);
  } // for y    
  obdWriteCommand(pOBD, 0x12); // display refresh
  EPDWaitBusy(pOBD);
  EPDSleep(pOBD);
} /* EPDDumpBuffer() */

//
// Special case for Sharp Memory LCD
//
static void SharpDumpBuffer(OBDISP *pOBD, uint8_t *pBuffer)
{
int x, y;
uint8_t c, ucInvert, *s, *d, ucStart;
uint8_t ucLineBuf[56];
int iPitch = pOBD->width / 8;
static uint8_t ucVCOM = 0;
int iBit;
uint8_t ucMask;

  ucInvert = (pOBD->invert) ? 0x00 : 0xff;
  digitalWrite(pOBD->iCSPin, HIGH); // active high
 
    ucLineBuf[0] = 0;
  ucStart = 0x80; // write command
  if (ucVCOM)
    ucStart |= 0x40; // VCOM bit
  ucLineBuf[1] = ucStart;
  // this code assumes I2C, so the first byte is ignored
  _I2CWrite(pOBD, ucLineBuf, 2); // write command(01) + vcom(02)
  ucVCOM = !ucVCOM; // need to toggle this each transaction

 // We need to flip and invert the image in code because the Sharp memory LCD
 // controller only has the simplest of commands for data writing
  if (pOBD->flip)
  {
     for (y=0; y<pOBD->height; y++) // we have to write the memory in the wrong direction
     {  
         ucMask = 0x80 >> (y & 7);
        s = &pBuffer[pOBD->width - 1 + (pOBD->width * ((pOBD->height - 1 - y) >> 3))]; // point to last line first
        d = &ucLineBuf[2];
        ucLineBuf[1] = pgm_read_byte(&ucMirror[y+1]); // current line number
        for (x=0; x<pOBD->width/8; x++)
        {  
           c = ucInvert; // we need to brute-force invert it
            for (iBit=7; iBit>=0; iBit--)
            {
                if (s[0] & ucMask)
                    c ^= (1 << iBit);
                s--;
            }
           *d++ = c;
        } // for y
        // write this line to the display
        ucLineBuf[iPitch+2] = 0; // end of line
        _I2CWrite(pOBD, ucLineBuf, iPitch+3);
     } // for x
  }
  else // normal orientation
  {
     for (y=0; y<pOBD->height; y++) // we have to write the memory in the wrong direction
     {
        ucMask = 1 << (y & 7);
        s = &pBuffer[pOBD->width * (y >> 3)]; // point to last line first
        d = &ucLineBuf[2];
        
        ucLineBuf[1] = pgm_read_byte(&ucMirror[y+1]); // current line number
        for (x=0; x<pOBD->width/8; x++)
        {
            c = ucInvert;
            for (iBit=7; iBit>=0; iBit--)
            {
                if (s[0] & ucMask)
                    c ^= (1 << iBit);
                s++;
            }
           *d++ = c;
        } // for y
        // write this line to the display
        ucLineBuf[iPitch+2] = 0; // end of line
        _I2CWrite(pOBD, ucLineBuf, iPitch+3);
     } // for x
  }
  ucLineBuf[1] = 0;
  _I2CWrite(pOBD, ucLineBuf, 2); // final transfer
  digitalWrite(pOBD->iCSPin, LOW); // de-activate
} /* SharpDumpBuffer() */
//
// Dump a partial screen to an e-ink display
//
void obdDumpPartial(OBDISP *pOBD, int startx, int starty, int width, int height)
{
   EPDDumpPartial(pOBD, NULL, startx, starty, width, height);
} /* obdDumpPartial() */
//
// Dump a screen's worth of data directly to the display
// Try to speed it up by comparing the new bytes with the existing buffer
//
void obdDumpBuffer(OBDISP *pOBD, uint8_t *pBuffer)
{
int x, y, iPitch;
int iLines, iCols;
uint8_t bNeedPos;
uint8_t *pSrc = pOBD->ucScreen;
    
  iPitch = pOBD->width;
  if (pOBD->type == LCD_VIRTUAL) // wrong function for this type of display
    return;
  if (pBuffer == NULL) // dump the internal buffer if none is given
    pBuffer = pOBD->ucScreen;
  if (pBuffer == NULL)
    return; // no backbuffer and no provided buffer
  
  if (pOBD->type == EPD42_400x300)
  {
     EPDDumpBuffer(pOBD, pBuffer);
     return;
  }
  if (pOBD->type >= SHARP_144x168) // special case for Sharp Memory LCDs
  {
    SharpDumpBuffer(pOBD, pBuffer);
    return;
  }
  iLines = pOBD->height >> 3;
  iCols = pOBD->width >> 4;
  for (y=0; y<iLines; y++)
  {
    bNeedPos = 1; // start of a new line means we need to set the position too
    for (x=0; x<iCols; x++) // wiring library has a 32-byte buffer, so send 16 bytes so that the data prefix (0x40) can fit
    {
      if (pOBD->ucScreen == NULL || pBuffer == pSrc || memcmp(pSrc, pBuffer, 16) != 0) // doesn't match, need to send it
      {
        if (bNeedPos) // need to reposition output cursor?
        {
           bNeedPos = 0;
           obdCachedFlush(pOBD, 1);
           obdSetPosition(pOBD, x*16, y, 1);
        }
        obdCachedWrite(pOBD, pBuffer, 16, 1);
      }
      else
      {
         bNeedPos = 1; // we're skipping a block, so next time will need to set the new position
      }
      pSrc += 16;
      pBuffer += 16;
    } // for x
    pSrc += (iPitch - pOBD->width); // for narrow displays, skip to the next line
    pBuffer += (iPitch - pOBD->width);
  } // for y
  obdCachedFlush(pOBD, 1);
} /* obdDumpBuffer() */

//
// Menu functions are not (yet) supported on Linux
//
#ifndef _LINUX_
// A valid CW or CCW move returns 1 or -1, invalid returns 0.
static int obdMenuReadRotary(SIMPLEMENU *sm) {
static int8_t rot_enc_table[] = {0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0};
uint8_t c;
int rc = 0;


  sm->prevNextCode <<= 2;
  if (digitalRead(sm->u8Dn) == sm->iPressed)
     sm->prevNextCode |= 0x02;
  if (digitalRead(sm->u8Up) == sm->iPressed)
     sm->prevNextCode |= 0x01;
  sm->prevNextCode &= 0x0f;

   // If valid then store as 16 bit data.
   if  (rot_enc_table[sm->prevNextCode]) {
      sm->store <<= 4;
      sm->store |= sm->prevNextCode;
      c = sm->store & 0xff;
      //if (store==0xd42b) return 1;
      //if (store==0xe817) return -1;
      if ((c & 0xf) == 2) rc = -1;
      else if ((c & 0xf) == 1) rc = 1;
   }
//   Serial.printf("store = 0x%04x, val = %d\n", sm->store, rc);
   return rc;
} /* obdMenuReadRotary() */

//
// Initialize the simple menu structure
//
int obdMenuInit(OBDISP *pOBD, SIMPLEMENU *sm, char **pText, int iFontSize, int bCenter, int btnUp, int btnDn, int btnEnter, int iPressedState, int bIsRotary)
{
  int iLen;
  if (sm == NULL || pText == NULL) return 0;
  sm->pOBD = pOBD;
  sm->u8Up = btnUp; // pin numbers of the action buttons
  sm->u8Dn = btnDn; // or rotary A line
  sm->u8Enter = btnEnter; // or rotary B line
  sm->bIsRotary = bIsRotary;
  sm->u8BtnState = 0; // no active buttons to start
  sm->iPressed = iPressedState; // active state of a pressed button
  sm->bCenter = bCenter;
  sm->iFontSize = iFontSize;
  sm->pMenuText = pText;
  sm->iMenuIndex = 0; // start at first item
  sm->iDispX = 128; // DEBUG
  sm->iDispY = 64; // DEBUG
  sm->bOneButton = (btnDn == -1 && btnEnter == -1); // only 1 button defined
  sm->pfnCallback = NULL;
  sm->prevNextCode = 0;
  sm->store = 0;
  iLen = 0;
  while (pText[iLen] != NULL) {
    iLen++;
  }
  sm->iMenuLen = iLen-1; // don't count the title text
  return 1; // success
} /* obdMenuInit() */

//
// Get the text and centering position for
// a specific menu item
// returns the X position
//
static int obdMenuGetItem(SIMPLEMENU *sm, int iItem, char *szText)
{
  int x, cx, len;

  if (iItem > sm->iMenuLen)
     return -1; // invalid request

  if (sm->iFontSize == FONT_6x8)
     cx = 6;
  else if (sm->iFontSize == FONT_8x8)
     cx = 8;
  else cx = 16;
  strcpy(szText, sm->pMenuText[iItem]);
  if (sm->pfnCallback && iItem > 0) // don't add callback for menu title
  {
    strcat(szText, " ");
    strcat(szText, (*sm->pfnCallback)(iItem-1));
  }
  x = 0;
  if (sm->bCenter || iItem == 0) // always center the menu title
  {
    len = strlen(szText);
     x = (sm->iDispX - (len * cx)) / 2;
  }
  return x;
} /* obdMenuGetItem() */

//
// Erase the display and show the given menu
//
void obdMenuShow(SIMPLEMENU *sm, int iItem)
{
int i, x, iCount, iStart = 0;
int iFirst, iLast;
char szTemp[64];

  iCount = (sm->iDispY / 8) - 1; // DEBUG - number of visible menu lines
  iFirst = iLast = iItem;
  if (iItem == -1) // show the entire menu
  {
    obdFill(sm->pOBD, 0, 0);
    x = obdMenuGetItem(sm, 0, szTemp); // get the title text
    obdMenuShowItem(sm->pOBD, x, 0, szTemp, 0, 0, sm->iFontSize, 0); // show title
    iFirst = 0;
    iLast = iCount-1;
  }
  if (sm->iMenuIndex >= iCount) // needs to scroll up
     iStart = sm->iMenuIndex - (iCount-1);
  if (sm->iMenuIndex < 0 || sm->iMenuIndex+iCount > sm->iMenuLen) { // invalid
     sm->iMenuIndex = 0;
     iStart = 0;
  }

  for (i=iFirst; i<=iLast && i+iStart < sm->iMenuLen; i++) // draw the visible menu lines
  {
    x = obdMenuGetItem(sm, i + iStart + 1, szTemp);
    if (x >= 0) // display if valid
       obdMenuShowItem(sm->pOBD, x, i+1, szTemp, (i+iStart == sm->iMenuIndex), (iFirst==iLast), sm->iFontSize, (iFirst==iLast));
  }
  if (iItem == -1) // now the display it in one shot
    obdDumpBuffer(sm->pOBD, NULL);
} /* obdMenuShow() */

//
// Set a callback function to return custom info/status
// for each menu item
//
void obdMenuSetCallback(SIMPLEMENU *sm, SIMPLECALLBACK pfnCallBack)
{
  if (sm != NULL)
    sm->pfnCallback = pfnCallBack;
} /* obdMenuSetCallback() */
//
// Display the text of a single menu item
// optionally erases what's under it to prevent left-over text when the length changes
//
void obdMenuShowItem(OBDISP *pOBD, int x, int y, char *szText, int bInvert, int bErase, int iFontSize, int bRender)
{
  static char *szBlank = (char *)"                      ";
  if (bErase)
    obdWriteString(pOBD, 0, 0, y, szBlank, iFontSize, 0, bRender); // erase old info
  obdWriteString(pOBD, 0, x, y, szText, iFontSize, bInvert, bRender);
} /* obdMenuShowItem() */

//
// Flash a menu item when it is selected
//
static void obdMenuFlash(SIMPLEMENU *sm, int iItem)
{
int x, y, i, iCount;
char szTemp[64];

  iCount = (sm->iDispY / 8) - 1; // DEBUG - number of visible menu lines
  y = iItem+1;
  if (y > iCount) // max bottom line
     y = iCount;

  x = obdMenuGetItem(sm, iItem+1, szTemp);
  if (x < 0) return; // invalid request

  for (i=0; i<3; i++)
  {
    obdMenuShowItem(sm->pOBD, x, y, szTemp, 0, 0, sm->iFontSize, 1); // show non-inverted
    delay(200);
    obdMenuShowItem(sm->pOBD, x, y, szTemp, 1, 0, sm->iFontSize, 1); // show inverted
    delay(200);
  }
} /* obdMenuFlash() */

//
// Change the menu index incrementally
// redraws the minimum amount of screen to show the new info
// (this prevents flicker/flash and saves battery life)
// returns the new menu index
//
int obdMenuDelta(SIMPLEMENU *sm, int iDelta)
{
  int i, x, iNewIndex, iCount;
  int iStart1, iStart2;
  char szTemp[64];

  if (iDelta == 0) return sm->iMenuIndex; // nothing to do
  
  iNewIndex = sm->iMenuIndex + iDelta;
  if (!sm->bOneButton && (iNewIndex < 0 || iNewIndex >= sm->iMenuLen)) // no change possible, exit
     return sm->iMenuIndex; // unchanged
  // If we are using a single button, wrap around the ends
  if (iNewIndex < 0) iNewIndex = (sm->iMenuLen - 1);
  else if (iNewIndex > sm->iMenuLen-1) iNewIndex = 0;
     
  iCount = (sm->iDispY / 8) - 1; // DEBUG - number of visible menu lines
  iStart1 = iStart2 = 0;
  if (sm->iMenuIndex > iCount-1)
     iStart1 = sm->iMenuIndex - (iCount-1);
  if (iNewIndex > iCount-1) // needs to scroll up
     iStart2 = iNewIndex - (iCount-1);
  if (iStart1 != iStart2) // need to redraw all items
  {
    for (i=0; i<iCount; i++)
    {
      x = obdMenuGetItem(sm, i+iStart2+1, szTemp);
      if (x >= 0)
         obdMenuShowItem(sm->pOBD, x, i+1, szTemp, (i+iStart2 == iNewIndex), 1, sm->iFontSize, 0);
    }
    obdDumpBuffer(sm->pOBD, NULL);
  }
  else // need to redraw only the new and old items
  {
      i = sm->iMenuIndex - iStart1;
      x = obdMenuGetItem(sm, sm->iMenuIndex+1, szTemp);
      if (x >= 0)
         obdMenuShowItem(sm->pOBD, x, i+1, szTemp, 0, 0, sm->iFontSize, 1);
      i = iNewIndex - iStart2;
      x = obdMenuGetItem(sm, iNewIndex+1, szTemp);
      if (x >= 0)
         obdMenuShowItem(sm->pOBD, x, i+1, szTemp, 1, 0, sm->iFontSize, 1);
  }
  sm->iMenuIndex = iNewIndex;
  return iNewIndex;
} /* obdMenuDelta() */

//
// With the given setup, check for button presses
// and act accordingly
// returns -1 for normal interactions and the menu item index if the user presses the ENTER button
//
// time in milliseconds for a long press
#define MENU_LONG_PRESS 600
int obdMenuRun(SIMPLEMENU *sm)
{
uint8_t buttons = 0;
unsigned long ul;
int iDelta, rc = -1;

  if (sm->bIsRotary) { // read the rotary encoder
    if (digitalRead(sm->u8Enter) == sm->iPressed) {
       buttons |= 1; // pressed
       if (buttons != sm->u8BtnState)
          rc = sm->iMenuIndex; // user pressed ENTER, return current menu index
    } else { // check for rotary encoder activity
      iDelta = obdMenuReadRotary(sm);
      obdMenuDelta(sm, iDelta);
    }
    sm->u8BtnState = buttons;
  } else {
// check the button states
    if (digitalRead(sm->u8Up) == sm->iPressed)
       buttons |= 1;
    if (buttons != sm->u8BtnState) // something changed
    {
      if (sm->bOneButton) // different logic for a single button system
      {
        if (sm->u8BtnState == 0 && buttons == 1) // up button just pressed
        {
           sm->ulPressTime = millis(); // record the press time
        }
        if (sm->u8BtnState == 1 && buttons == 0) // up button just released
        {
          ul = millis() - sm->ulPressTime;
          if (ul < MENU_LONG_PRESS) // short press = navigate menu
            obdMenuDelta(sm, 1);
          else // treat it like a long press
            rc = sm->iMenuIndex; // action
        }
      }
      else // 3 button setup (UP/DOWN/ENTER)
      {
        if (digitalRead(sm->u8Dn) == sm->iPressed)
           buttons |= 2;
        if (digitalRead(sm->u8Enter) == sm->iPressed)
           rc = sm->iMenuIndex; // user pressed ENTER, return current menu index
        if ((sm->u8BtnState & 1) == 0 && (buttons & 1) == 1) // Up button pressed
        {
          obdMenuDelta(sm, -1);
        }
        if ((sm->u8BtnState & 2) == 0 && (buttons & 2) == 2) // Down button pressed
        {
          obdMenuDelta(sm, 1);
        }
      }
      sm->u8BtnState = buttons; // save the latest state
    }
  }
  if (rc != -1) // selected
     obdMenuFlash(sm, sm->iMenuIndex);
  return rc;
} /* obdMenuRun() */
#endif // !_LINUX_
