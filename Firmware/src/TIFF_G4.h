#ifndef __TIFFG4__
#define __TIFFG4__
// DEBUG - define __LINUX__ to build on the tl32 SoC
#define __LINUX__
#if defined( __MACH__ ) || defined( __LINUX__ )
//#include <stdlib.h>
//#include <string.h>
#include <stdint.h>
//#include <stdio.h>
#define memcpy_P memcpy
#define PROGMEM
#define pgm_read_byte(s) *s
#define pgm_read_word(s) *(int16_t *)s
#else
#include <Arduino.h>
#ifndef PROGMEM
#define PROGMEM
#define memcpy_P memcpy
#define pgm_read_byte(p) *(uint8_t *)p
#define pgm_read_word(p) *(uint16_t *)p
#endif
#endif
//
// One Bit Graphics library
// Written by Larry Bank
// Copyright (c) 2020 BitBank Software, Inc.
// 
// Designed to decode and display 1-bpp CCITT G4
// encoded images at any scale
// with optional "scale-to-gray" enhancement
//

/* Defines and variables */
#define MAX_BUFFERED_PIXELS 1024
#define FILE_BUF_SIZE 2048
#define MAX_IMAGE_WIDTH 2600
#define FILE_HIGHWATER ((FILE_BUF_SIZE * 3) >> 2)
#define TIFF_TAG_SIZE 12
#define MAX_TIFF_TAGS 128
#define BITDIR_MSB_FIRST     1
#define BITDIR_LSB_FIRST     2

// Error codes returned by getLastError()
enum {
    TIFF_SUCCESS = 0,
    TIFF_INVALID_PARAMETER,
    TIFF_DECODE_ERROR,
    TIFF_UNSUPPORTED_FEATURE,
    TIFF_INVALID_FILE
};
//
// Output pixel types
// selecting 2 or 8-bpp output automatically selects
// scale-to-gray anti-aliasing
//
enum {
    TIFF_PIXEL_1BPP = 0,
    TIFF_PIXEL_2BPP,
    TIFF_PIXEL_4BPP
};

typedef struct tiff_file_tag
{
  int32_t iPos; // current file position
  int32_t iSize; // file size
  uint8_t *pData; // memory file pointer
  void * fHandle; // class pointer to File/SdFat or whatever you want
} TIFFFILE;

//
// Defines the output drawing window
//
typedef struct tiff_window_tag
{
    int x, y; // upper left corner of interest (source pixels)
    float fScale;
    uint32_t iScale; // 16:16 fixed scale factor (e.g. 0.5 = 0x8000)
    int iWidth, iHeight; // destination window size (for clipping purposes)
    uint8_t *p4BPP; // user-supplied buffer for 4-bpp grayscale output
    uint8_t ucPixelType;
} TIFFWINDOW;

typedef struct tiff_draw_tag
{
    int y; // current line
    int iScaledWidth; // width of the current line
    int iWidth, iHeight; // size of entire image in pixels
    uint8_t *pPixels; // 1 or 2-bit pixels
    uint8_t ucPixelType, ucLast;
} TIFFDRAW;

// Callback function prototypes
typedef int32_t (TIFF_READ_CALLBACK)(TIFFFILE *pFile, uint8_t *pBuf, int32_t iLen);
typedef int32_t (TIFF_SEEK_CALLBACK)(TIFFFILE *pFile, int32_t iPosition);
typedef void (TIFF_DRAW_CALLBACK)(TIFFDRAW *pDraw);
typedef void * (TIFF_OPEN_CALLBACK)(const char *szFilename, int32_t *pFileSize);
typedef void (TIFF_CLOSE_CALLBACK)(void *pHandle);

//
// our private structure to hold a TIFF image decode state
//
typedef struct tiff_image_tag
{
    int iWidth, iHeight; // image size
    int iError;
    int y; // last y value drawn
    int iVLCOff, iVLCSize;
    int iStripSize, iStripOffset;
    int iPitch; // width in bytes of output buffer
    uint32_t u32Accum; // fractional scaling accumulator
    uint8_t ucCompression, ucPhotometric, ucFillOrder;
    TIFF_READ_CALLBACK *pfnRead;
    TIFF_SEEK_CALLBACK *pfnSeek;
    TIFF_DRAW_CALLBACK *pfnDraw;
    TIFF_OPEN_CALLBACK *pfnOpen;
    TIFF_CLOSE_CALLBACK *pfnClose;
    TIFFFILE TIFFFile;
    TIFFWINDOW window;
    int16_t CurFlips[MAX_IMAGE_WIDTH];
    int16_t RefFlips[MAX_IMAGE_WIDTH];
    uint8_t ucPixels[MAX_BUFFERED_PIXELS];
    uint8_t ucFileBuf[FILE_BUF_SIZE]; // holds temp data and pixel stack
} TIFFIMAGE;

#ifdef __cplusplus
//
// The TIFFG4 class wraps portable C code which does the actual work
//
class TIFFG4
{
  public:
    int openTIFF(uint8_t *pData, int iDataSize, TIFF_DRAW_CALLBACK *pfnDraw);
    int openTIFF(const char *szFilename, TIFF_OPEN_CALLBACK *pfnOpen, TIFF_CLOSE_CALLBACK *pfnClose, TIFF_READ_CALLBACK *pfnRead, TIFF_SEEK_CALLBACK *pfnSeek, TIFF_DRAW_CALLBACK *pfnDraw);
    int openRAW(int iWidth, int iHeight, int iFillOrder, uint8_t *pData, int iDataSize, TIFF_DRAW_CALLBACK *pfnDraw);
    void close();
    void setDrawParameters(uint32_t scale, int iPixelType, int iStartX, int iStartY, int iWidth, int iHeight, uint8_t *p4BPPBuf);
    int decode();
    int getWidth();
    int getHeight();
    int getLastError();

  private:
    TIFFIMAGE _tiff;
};
#else
    int TIFF_openTIFFRAM(TIFFIMAGE *pImage, uint8_t *pData, int iDatasize, TIFF_DRAW_CALLBACK *pfnDraw);
#ifdef __LINUX__
    int TIFF_openTIFFFile(TIFFIMAGE *pImage, const char *szFilename, TIFF_DRAW_CALLBACK *pfnDraw);
#else
int TIFF_openTIFFFile(TIFFIMAGE *pImage, const char *szFilename, TIFF_OPEN_CALLBACK *pfnOpen, TIFF_CLOSE_CALLBACK *pfnClose, TIFF_READ_CALLBACK *pfnRead, TIFF_SEEK_CALLBACK *pfnSeek, TIFF_DRAW_CALLBACK *pfnDraw);
#endif
    int TIFF_openRAW(TIFFIMAGE *pImage, int iWidth, int iHeight, int iFillOrder, uint8_t *pData, int iDataSize, TIFF_DRAW_CALLBACK *pfnDraw);
    void TIFF_close(TIFFIMAGE *pImage);
    void TIFF_setDrawParameters(TIFFIMAGE *pImage, uint32_t scale, int iPixelType, int iStartX, int iStartY, int iWidth, int iHeight, uint8_t *p4BPPBuf);
    int TIFF_decode(TIFFIMAGE *pImage);
    int TIFF_getWidth(TIFFIMAGE *pImage);
    int TIFF_getHeight(TIFFIMAGE *pImage);
    int TIFF_getLastError(TIFFIMAGE *pImage);
#endif

// Due to unaligned memory causing an exception, we have to do these macros the slow way
#define MOTOLONG(p) (((*p)<<24UL) + ((*(p+1))<<16UL) + ((*(p+2))<<8UL) + (*(p+3)))
#define TOP_BIT 0x80000000
#define MAX_VALUE 0xffffffff
#define LONGWHITECODEMASK 0x2000000
#define LONGBLACKCODEMASK 0x10000000
// Must be a 32-bit target processor
#define REGISTER_WIDTH 32

#define CLIMBWHITE_NEW(pBuf, ulBitOff, ulBits, sCode) \
    { uint32_t ul; int iLen = 64; sCode = 0; while (iLen > 63) \
    { if (ulBitOff > (REGISTER_WIDTH-17)) \
      { pBuf += (ulBitOff>>3); ulBitOff &= 7; ulBits = MOTOLONG(pBuf); } \
      if ((ulBits << ulBitOff) < LONGWHITECODEMASK) \
                  { ul = (ulBits >> ((REGISTER_WIDTH-14) - ulBitOff)) & 0x3fe; ulBitOff += pgm_read_word(&black_l[ul]); iLen = pgm_read_word(&black_l[ul+1]);} \
                else {ul = (ulBits >> ((REGISTER_WIDTH - 10) - ulBitOff)) & 0x3fe; ulBitOff += pgm_read_word(&white_s[ul]); iLen = pgm_read_word(&white_s[ul+1]);} \
     sCode += iLen; }}

#define CLIMBBLACK_NEW(pBuf, ulBitOff, ulBits, sCode) \
    { uint32_t ul; int iLen = 64; sCode = 0; while (iLen > 63) \
    { if (ulBitOff > (REGISTER_WIDTH-15)) \
      { pBuf += (ulBitOff>>3); ulBitOff &= 7; ulBits = MOTOLONG(pBuf); } \
      if ((ulBits << ulBitOff) < LONGBLACKCODEMASK) \
         { ul = (ulBits >> ((REGISTER_WIDTH-14) - ulBitOff)) & 0x3fe; ulBitOff += pgm_read_word(&black_l[ul]); iLen = pgm_read_word(&black_l[ul+1]);} \
        else {ul = (ulBits >> ((REGISTER_WIDTH - 7) - ulBitOff)) & 0x7e; ulBitOff += pgm_read_word(&black_s[ul]); iLen = pgm_read_word(&black_s[ul+1]);} \
     sCode += iLen; }}

#endif // __ONEBITGFX__
