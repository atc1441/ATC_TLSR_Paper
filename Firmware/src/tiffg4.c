//
// TIFF_G4
// A TIFF / 1-bpp image library
// written by Larry Bank
// bitbank@pobox.com
// Arduino port started 8/10/2020
// Original code written 20 years ago :)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "TIFF_G4.h"
#include <string.h>
#include <stdint.h>
// forward references
static int TIFFInit(TIFFIMAGE *pTIFF);
static int TIFFParseInfo(TIFFIMAGE *pPage);
static void TIFFGetMoreData(TIFFIMAGE *pPage);
static int Decode(TIFFIMAGE *pImage);

// Scale to gray tables
//
// Top 4 bits = top line, bottom 4 bits = bottom line
// This 4x2 block gets translated into a 2x1 block of 2-bpp pixels
// There are really 5 gray levels that we have to fit in 2 bits
// so the compromise is to merge levels 2 and 3
// 2x2 black pixel count vs gray level
// 0 -> 0
// 1 -> 1
// 2 -> 2
// 3 -> 2
// 4 -> 3
const uint8_t ucGray2BPP[256] PROGMEM =
{   0x00,0x01,0x01,0x02,0x04,0x05,0x05,0x06,0x04,0x05,0x05,0x06,0x08,0x09,0x09,0x0a,
0x01,0x02,0x02,0x02,0x05,0x06,0x06,0x06,0x05,0x06,0x06,0x06,0x09,0x0a,0x0a,0x0a,
0x01,0x02,0x02,0x02,0x05,0x06,0x06,0x06,0x05,0x06,0x06,0x06,0x09,0x0a,0x0a,0x0a,
0x02,0x02,0x02,0x03,0x06,0x06,0x06,0x07,0x06,0x06,0x06,0x07,0x0a,0x0a,0x0a,0x0b,
0x04,0x05,0x05,0x06,0x08,0x09,0x09,0x0a,0x08,0x09,0x09,0x0a,0x08,0x09,0x09,0x0a,
0x05,0x06,0x06,0x06,0x09,0x0a,0x0a,0x0a,0x09,0x0a,0x0a,0x0a,0x09,0x0a,0x0a,0x0a,
0x05,0x06,0x06,0x06,0x09,0x0a,0x0a,0x0a,0x09,0x0a,0x0a,0x0a,0x09,0x0a,0x0a,0x0a,
0x06,0x06,0x06,0x07,0x0a,0x0a,0x0a,0x0b,0x0a,0x0a,0x0a,0x0b,0x0a,0x0a,0x0a,0x0b,
0x04,0x05,0x05,0x06,0x08,0x09,0x09,0x0a,0x08,0x09,0x09,0x0a,0x08,0x09,0x09,0x0a,
0x05,0x06,0x06,0x06,0x09,0x0a,0x0a,0x0a,0x09,0x0a,0x0a,0x0a,0x09,0x0a,0x0a,0x0a,
0x05,0x06,0x06,0x06,0x09,0x0a,0x0a,0x0a,0x09,0x0a,0x0a,0x0a,0x09,0x0a,0x0a,0x0a,
0x06,0x06,0x06,0x07,0x0a,0x0a,0x0a,0x0b,0x0a,0x0a,0x0a,0x0b,0x0a,0x0a,0x0a,0x0b,
0x08,0x09,0x09,0x0a,0x08,0x09,0x09,0x0a,0x08,0x09,0x09,0x0a,0x0c,0x0d,0x0d,0x0e,
0x09,0x0a,0x0a,0x0a,0x09,0x0a,0x0a,0x0a,0x09,0x0a,0x0a,0x0a,0x0d,0x0e,0x0e,0x0e,
0x09,0x0a,0x0a,0x0a,0x09,0x0a,0x0a,0x0a,0x09,0x0a,0x0a,0x0a,0x0d,0x0e,0x0e,0x0e,
0x0a,0x0a,0x0a,0x0b,0x0a,0x0a,0x0a,0x0b,0x0a,0x0a,0x0a,0x0b,0x0e,0x0e,0x0e,0x0f};
//
// 4-bpp output renders 5 distinct levels of gray
// the white pixel count determines the output brightness
//
const uint8_t  ucGray4BPP[256] PROGMEM = {0,4,4,8,0x40,0x44,0x44,0x48,0x40,0x44,0x44,0x48,0x80,0x84,0x84,0x88,              // 0-15
4,8,8,7,0x44,0x48,0x48,0x47,0x44,0x48,0x48,0x47,0x84,0x88,0x88,0x87,              // 16-31
4,8,8,7,0x44,0x48,0x48,0x47,0x44,0x48,0x48,0x47,0x84,0x88,0x88,0x87,              // 32-47
8,7,7,0x0f,0x48,0x47,0x47,0x4f,0x48,0x47,0x47,0x4f,0x88,0x87,0x87,0x8f,           // 48-63
0x40,0x44,0x44,0x48,0x80,0x84,0x84,0x88,0x80,0x84,0x84,0x88,0x70,0x74,0x74,0x78,  // 64-79
0x44,0x48,0x48,0x47,0x84,0x88,0x88,0x87,0x84,0x88,0x88,0x87,0x74,0x78,0x78,0x77,  // 80-95
0x44,0x48,0x48,0x47,0x84,0x88,0x88,0x87,0x84,0x88,0x88,0x87,0x74,0x78,0x78,0x77,  // 96-111
0x48,0x47,0x47,0x4f,0x88,0x87,0x87,0x8f,0x88,0x87,0x87,0x8f,0x78,0x77,0x77,0x7f,  // 112-127
0x40,0x44,0x44,0x48,0x80,0x84,0x84,0x88,0x80,0x84,0x84,0x88,0x70,0x74,0x74,0x78,  // 128-143
0x44,0x48,0x48,0x47,0x84,0x88,0x88,0x87,0x84,0x88,0x88,0x87,0x74,0x78,0x78,0x77,  // 144-159
0x44,0x48,0x48,0x47,0x84,0x88,0x88,0x87,0x84,0x88,0x88,0x87,0x74,0x78,0x78,0x77,  // 160-175
0x48,0x47,0x47,0x4f,0x88,0x87,0x87,0x8f,0x88,0x87,0x87,0x8f,0x78,0x77,0x77,0x7f,  // 176-191
0x80,0x84,0x84,0x88,0x70,0x74,0x74,0x78,0x70,0x74,0x74,0x78,0xf0,0xf4,0xf4,0xf8,  // 192-207
0x84,0x88,0x88,0x87,0x74,0x78,0x78,0x77,0x74,0x78,0x78,0x77,0xf4,0xf8,0xf8,0xf7,  // 208-223
0x84,0x88,0x88,0x87,0x74,0x78,0x78,0x77,0x74,0x78,0x78,0x77,0xf4,0xf8,0xf8,0xf7,  // 224-239
0x88,0x87,0x87,0x8f,0x78,0x77,0x77,0x7f,0x78,0x77,0x77,0x7f,0xf8,0xf7,0xf7,0xff}; // 240-255
/* Table of byte flip values to mirror-image incoming CCITT data */
static const uint8_t ucMirror[256] PROGMEM =
     {0, 128, 64, 192, 32, 160, 96, 224, 16, 144, 80, 208, 48, 176, 112, 240,
      8, 136, 72, 200, 40, 168, 104, 232, 24, 152, 88, 216, 56, 184, 120, 248,
      4, 132, 68, 196, 36, 164, 100, 228, 20, 148, 84, 212, 52, 180, 116, 244,
      12, 140, 76, 204, 44, 172, 108, 236, 28, 156, 92, 220, 60, 188, 124, 252,
      2, 130, 66, 194, 34, 162, 98, 226, 18, 146, 82, 210, 50, 178, 114, 242,
      10, 138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218, 58, 186, 122, 250,
      6, 134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246,
      14, 142, 78, 206, 46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254,
      1, 129, 65, 193, 33, 161, 97, 225, 17, 145, 81, 209, 49, 177, 113, 241,
      9, 137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217, 57, 185, 121, 249,
      5, 133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245,
      13, 141, 77, 205, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253,
      3, 131, 67, 195, 35, 163, 99, 227, 19, 147, 83, 211, 51, 179, 115, 243,
      11, 139, 75, 203, 43, 171, 107, 235, 27, 155, 91, 219, 59, 187, 123, 251,
      7, 135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215, 55, 183, 119, 247,
      15, 143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127, 255};
/*
 The code tree that follows has: bit_length, decode routine
 These codes are for Group 4 (MMR) decoding

 01 = vertneg1, 11h = vert1, 20h = horiz, 30h = pass, 12h = vert2
 02 = vertneg2, 13h = vert3, 03 = vertneg3, 90h = trash
*/

static const uint8_t code_table[128] PROGMEM =
        {0x90, 0, 0x40, 0,       /* trash, uncompr mode - codes 0 and 1 */
         3, 7,                   /* V(-3) pos = 2 */
         0x13, 7,                /* V(3)  pos = 3 */
         2, 6, 2, 6,             /* V(-2) pos = 4,5 */
         0x12, 6, 0x12, 6,       /* V(2)  pos = 6,7 */
         0x30, 4, 0x30, 4, 0x30, 4, 0x30, 4,    /* pass  pos = 8->F */
         0x30, 4, 0x30, 4, 0x30, 4, 0x30, 4,
         0x20, 3, 0x20, 3, 0x20, 3, 0x20, 3,    /* horiz pos = 10->1F */
         0x20, 3, 0x20, 3, 0x20, 3, 0x20, 3,
         0x20, 3, 0x20, 3, 0x20, 3, 0x20, 3,
         0x20, 3, 0x20, 3, 0x20, 3, 0x20, 3,
/* V(-1) pos = 20->2F */
         1, 3, 1, 3, 1, 3, 1, 3,
         1, 3, 1, 3, 1, 3, 1, 3,
         1, 3, 1, 3, 1, 3, 1, 3,
         1, 3, 1, 3, 1, 3, 1, 3,
         0x11, 3, 0x11, 3, 0x11, 3, 0x11, 3,   /* V(1)   pos = 30->3F */
         0x11, 3, 0x11, 3, 0x11, 3, 0x11, 3,
         0x11, 3, 0x11, 3, 0x11, 3, 0x11, 3,
         0x11, 3, 0x11, 3, 0x11, 3, 0x11, 3};

/*
 Here are the Huffman address codes for run lengths
 first the short white codes (first 4 bits != 0)
*/
static const int16_t white_s[1024] PROGMEM =
        {-1,-1,-1,-1,-1,-1,-1,-1,8,29,8,29,8,30,8,30,
        8,45,8,45,8,46,8,46,7,22,7,22,7,22,7,22,
        7,23,7,23,7,23,7,23,8,47,8,47,8,48,8,48,
        6,13,6,13,6,13,6,13,6,13,6,13,6,13,6,13,
        7,20,7,20,7,20,7,20,8,33,8,33,8,34,8,34,
        8,35,8,35,8,36,8,36,8,37,8,37,8,38,8,38,
        7,19,7,19,7,19,7,19,8,31,8,31,8,32,8,32,
        6,1,6,1,6,1,6,1,6,1,6,1,6,1,6,1,
        6,12,6,12,6,12,6,12,6,12,6,12,6,12,6,12,
        8,53,8,53,8,54,8,54,7,26,7,26,7,26,7,26,
        8,39,8,39,8,40,8,40,8,41,8,41,8,42,8,42,
        8,43,8,43,8,44,8,44,7,21,7,21,7,21,7,21,
        7,28,7,28,7,28,7,28,8,61,8,61,8,62,8,62,
        8,63,8,63,8,0,8,0,8,320,8,320,8,384,8,384,
        5,10,5,10,5,10,5,10,5,10,5,10,5,10,5,10,
        5,10,5,10,5,10,5,10,5,10,5,10,5,10,5,10,
        5,11,5,11,5,11,5,11,5,11,5,11,5,11,5,11,
        5,11,5,11,5,11,5,11,5,11,5,11,5,11,5,11,
        7,27,7,27,7,27,7,27,8,59,8,59,8,60,8,60,
        9,1472,9,1536,9,1600,9,1728,7,18,7,18,7,18,7,18,
        7,24,7,24,7,24,7,24,8,49,8,49,8,50,8,50,
        8,51,8,51,8,52,8,52,7,25,7,25,7,25,7,25,
        8,55,8,55,8,56,8,56,8,57,8,57,8,58,8,58,
        6,192,6,192,6,192,6,192,6,192,6,192,6,192,6,192,
        6,1664,6,1664,6,1664,6,1664,6,1664,6,1664,6,1664,6,1664,
        8,448,8,448,8,512,8,512,9,704,9,768,8,640,8,640,
        8,576,8,576,9,832,9,896,9,960,9,1024,9,1088,9,1152,
        9,1216,9,1280,9,1344,9,1408,7,256,7,256,7,256,7,256,
        4,2,4,2,4,2,4,2,4,2,4,2,4,2,4,2,
        4,2,4,2,4,2,4,2,4,2,4,2,4,2,4,2,
        4,2,4,2,4,2,4,2,4,2,4,2,4,2,4,2,
        4,2,4,2,4,2,4,2,4,2,4,2,4,2,4,2,
        4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,
        4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,
        4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,
        4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,
        5,128,5,128,5,128,5,128,5,128,5,128,5,128,5,128,
        5,128,5,128,5,128,5,128,5,128,5,128,5,128,5,128,
        5,8,5,8,5,8,5,8,5,8,5,8,5,8,5,8,
        5,8,5,8,5,8,5,8,5,8,5,8,5,8,5,8,
        5,9,5,9,5,9,5,9,5,9,5,9,5,9,5,9,
        5,9,5,9,5,9,5,9,5,9,5,9,5,9,5,9,
        6,16,6,16,6,16,6,16,6,16,6,16,6,16,6,16,
        6,17,6,17,6,17,6,17,6,17,6,17,6,17,6,17,
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        4,5,4,5,4,5,4,5,4,5,4,5,4,5,4,5,
        4,5,4,5,4,5,4,5,4,5,4,5,4,5,4,5,
        4,5,4,5,4,5,4,5,4,5,4,5,4,5,4,5,
        4,5,4,5,4,5,4,5,4,5,4,5,4,5,4,5,
        6,14,6,14,6,14,6,14,6,14,6,14,6,14,6,14,
        6,15,6,15,6,15,6,15,6,15,6,15,6,15,6,15,
        5,64,5,64,5,64,5,64,5,64,5,64,5,64,5,64,
        5,64,5,64,5,64,5,64,5,64,5,64,5,64,5,64,
        4,6,4,6,4,6,4,6,4,6,4,6,4,6,4,6,
        4,6,4,6,4,6,4,6,4,6,4,6,4,6,4,6,
        4,6,4,6,4,6,4,6,4,6,4,6,4,6,4,6,
        4,6,4,6,4,6,4,6,4,6,4,6,4,6,4,6,
        4,7,4,7,4,7,4,7,4,7,4,7,4,7,4,7,
        4,7,4,7,4,7,4,7,4,7,4,7,4,7,4,7,
        4,7,4,7,4,7,4,7,4,7,4,7,4,7,4,7,
        4,7,4,7,4,7,4,7,4,7,4,7,4,7,4,7};

/* the short black codes (first 4 bits != 0) */
static const int16_t black_s[128] PROGMEM =
       {-1,-1,-1,-1,-1,-1,-1,-1,6,9,6,8,5,7,5,7,
        4,6,4,6,4,6,4,6,4,5,4,5,4,5,4,5,
        3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1,
        3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
        2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,
        2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,
        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2};

/* The long black codes (first 4 bits == 0) */
#define EOL -9999   /* End of line */
#define EO1D -9998  /* End of 1D coding */
static const int16_t black_l[1024] PROGMEM =
    {1,0,1,0,12,EOL,12,EOL,1,-1,1,-1,1,-1,1,-1,
     1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,
     1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,12,EO1D,12,EO1D,
     1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,
     11,1792,11,1792,11,1792,11,1792,12,1984,12,1984,12,2048,12,2048,
     12,2112,12,2112,12,2176,12,2176,12,2240,12,2240,12,2304,12,2304,
     11,1856,11,1856,11,1856,11,1856,11,1920,11,1920,11,1920,11,1920,
     12,2368,12,2368,12,2432,12,2432,12,2496,12,2496,12,2560,12,2560,
     10,18,10,18,10,18,10,18,10,18,10,18,10,18,10,18,
     12,52,12,52,13,640,13,704,13,768,13,832,12,55,12,55,
     12,56,12,56,13,1280,13,1344,13,1408,13,1472,12,59,12,59,
     12,60,12,60,13,1536,13,1600,11,24,11,24,11,24,11,24,
     11,25,11,25,11,25,11,25,13,1664,13,1728,12,320,12,320,
     12,384,12,384,12,448,12,448,13,512,13,576,12,53,12,53,
     12,54,12,54,13,896,13,960,13,1024,13,1088,13,1152,13,1216,
     10,64,10,64,10,64,10,64,10,64,10,64,10,64,10,64,
     8,13,8,13,8,13,8,13,8,13,8,13,8,13,8,13,
     8,13,8,13,8,13,8,13,8,13,8,13,8,13,8,13,
     8,13,8,13,8,13,8,13,8,13,8,13,8,13,8,13,
     8,13,8,13,8,13,8,13,8,13,8,13,8,13,8,13,
     11,23,11,23,11,23,11,23,12,50,12,50,12,51,12,51,
     12,44,12,44,12,45,12,45,12,46,12,46,12,47,12,47,
     12,57,12,57,12,58,12,58,12,61,12,61,12,256,12,256,
     10,16,10,16,10,16,10,16,10,16,10,16,10,16,10,16,
     10,17,10,17,10,17,10,17,10,17,10,17,10,17,10,17,
     12,48,12,48,12,49,12,49,12,62,12,62,12,63,12,63,
     12,30,12,30,12,31,12,31,12,32,12,32,12,33,12,33,
     12,40,12,40,12,41,12,41,11,22,11,22,11,22,11,22,
     8,14,8,14,8,14,8,14,8,14,8,14,8,14,8,14,
     8,14,8,14,8,14,8,14,8,14,8,14,8,14,8,14,
     8,14,8,14,8,14,8,14,8,14,8,14,8,14,8,14,
     8,14,8,14,8,14,8,14,8,14,8,14,8,14,8,14,
     7,10,7,10,7,10,7,10,7,10,7,10,7,10,7,10,
     7,10,7,10,7,10,7,10,7,10,7,10,7,10,7,10,
     7,10,7,10,7,10,7,10,7,10,7,10,7,10,7,10,
     7,10,7,10,7,10,7,10,7,10,7,10,7,10,7,10,
     7,10,7,10,7,10,7,10,7,10,7,10,7,10,7,10,
     7,10,7,10,7,10,7,10,7,10,7,10,7,10,7,10,
     7,10,7,10,7,10,7,10,7,10,7,10,7,10,7,10,
     7,10,7,10,7,10,7,10,7,10,7,10,7,10,7,10,
     7,11,7,11,7,11,7,11,7,11,7,11,7,11,7,11,
     7,11,7,11,7,11,7,11,7,11,7,11,7,11,7,11,
     7,11,7,11,7,11,7,11,7,11,7,11,7,11,7,11,
     7,11,7,11,7,11,7,11,7,11,7,11,7,11,7,11,
     7,11,7,11,7,11,7,11,7,11,7,11,7,11,7,11,
     7,11,7,11,7,11,7,11,7,11,7,11,7,11,7,11,
     7,11,7,11,7,11,7,11,7,11,7,11,7,11,7,11,
     7,11,7,11,7,11,7,11,7,11,7,11,7,11,7,11,
     9,15,9,15,9,15,9,15,9,15,9,15,9,15,9,15,
     9,15,9,15,9,15,9,15,9,15,9,15,9,15,9,15,
     12,128,12,128,12,192,12,192,12,26,12,26,12,27,12,27,
     12,28,12,28,12,29,12,29,11,19,11,19,11,19,11,19,
     11,20,11,20,11,20,11,20,12,34,12,34,12,35,12,35,
     12,36,12,36,12,37,12,37,12,38,12,38,12,39,12,39,
     11,21,11,21,11,21,11,21,12,42,12,42,12,43,12,43,
     10,0,10,0,10,0,10,0,10,0,10,0,10,0,10,0,
     7,12,7,12,7,12,7,12,7,12,7,12,7,12,7,12,
     7,12,7,12,7,12,7,12,7,12,7,12,7,12,7,12,
     7,12,7,12,7,12,7,12,7,12,7,12,7,12,7,12,
     7,12,7,12,7,12,7,12,7,12,7,12,7,12,7,12,
     7,12,7,12,7,12,7,12,7,12,7,12,7,12,7,12,
     7,12,7,12,7,12,7,12,7,12,7,12,7,12,7,12,
     7,12,7,12,7,12,7,12,7,12,7,12,7,12,7,12,
     7,12,7,12,7,12,7,12,7,12,7,12,7,12,7,12};
//
// Helper functions for memory based images
//
static int32_t readMem(TIFFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
    int32_t iBytesRead;

    iBytesRead = iLen;
    if ((pFile->iSize - pFile->iPos) < iLen)
       iBytesRead = pFile->iSize - pFile->iPos;
    if (iBytesRead <= 0)
       return 0;
    memcpy_P(pBuf, &pFile->pData[pFile->iPos], iBytesRead);
    pFile->iPos += iBytesRead;
    return iBytesRead;
} /* readMem() */

static int32_t seekMem(TIFFFILE *pFile, int32_t iPosition)
{
    if (iPosition < 0) iPosition = 0;
    else if (iPosition >= pFile->iSize) iPosition = pFile->iSize-1;
    pFile->iPos = iPosition;
    return iPosition;
} /* seekMem() */
#if defined( __LINUX__ ) || defined( __MCUXPRESSO )

static void closeFile(void *handle)
{
   // fclose((FILE *)handle);
} /* closeFile() */

static int32_t seekFile(TIFFFILE *pFile, int32_t iPosition)
{
//    if (iPosition < 0) iPosition = 0;
//    else if (iPosition >= pFile->iSize) iPosition = pFile->iSize-1;
//    pFile->iPos = iPosition;
//    fseek((FILE *)pFile->fHandle, iPosition, SEEK_SET);
    return iPosition;
} /* seekFile() */

static int32_t readFile(TIFFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
    int32_t iBytesRead;

    iBytesRead = iLen;
//    if ((pFile->iSize - pFile->iPos) < iLen)
//       iBytesRead = pFile->iSize - pFile->iPos;
//    if (iBytesRead <= 0)
//       return 0;
//    iBytesRead = (int)fread(pBuf, 1, iBytesRead, (FILE *)pFile->fHandle);
//    pFile->iPos += iBytesRead;
    return iBytesRead;
} /* readFile() */

#endif // __LINUX__

#ifndef __cplusplus
//
// C API
//
int TIFF_openTIFFRAM(TIFFIMAGE *pImage, uint8_t *pData, int iDatasize, TIFF_DRAW_CALLBACK *pfnDraw)
{
    memset(pImage, 0, sizeof(TIFFIMAGE));
    pImage->pfnRead = readMem;
    pImage->pfnSeek = seekMem;
    pImage->pfnDraw = pfnDraw;
    pImage->pfnOpen = NULL;
    pImage->pfnClose = NULL;
    pImage->TIFFFile.iSize = iDatasize;
    pImage->TIFFFile.pData = pData;
    return TIFFInit(pImage);
} /* openTIFFRAM() */

#ifdef __LINUX__
int TIFF_openTIFFFile(TIFFIMAGE *pImage, const char *szFilename, TIFF_DRAW_CALLBACK *pfnDraw)
{
    memset(pImage, 0, sizeof(TIFFIMAGE));
    pImage->pfnRead = readFile;
    pImage->pfnSeek = seekFile;
    pImage->pfnDraw = pfnDraw;
    pImage->pfnClose = closeFile;
//    pImage->TIFFFile.fHandle = fopen(szFilename, "r+b");
//    if (pImage->TIFFFile.fHandle == NULL)
//       return 0;
//    fseek((FILE *)pImage->TIFFFile.fHandle, 0, SEEK_END);
//    pImage->TIFFFile.iSize = (int)ftell((FILE *)pImage->TIFFFile.fHandle);
//    fseek((FILE *)pImage->TIFFFile.fHandle, 0, SEEK_SET);
    return TIFFInit(pImage);

} /* openTIFFFile() */
#else
int TIFF_openTIFFFile(TIFFIMAGE *pImage, const char *szFilename, TIFF_OPEN_CALLBACK *pfnOpen, TIFF_CLOSE_CALLBACK *pfnClose, TIFF_READ_CALLBACK *pfnRead, TIFF_SEEK_CALLBACK *pfnSeek, TIFF_DRAW_CALLBACK *pfnDraw)
{
    memset(pImage, 0, sizeof(TIFFIMAGE));
    pImage->pfnRead = pfnRead;
    pImage->pfnSeek = pfnSeek;
    pImage->pfnDraw = pfnDraw;
    pImage->pfnOpen = pfnOpen;
    pImage->pfnClose = pfnClose;
    pImage->TIFFFile.fHandle = (*pfnOpen)(szFilename, &pImage->TIFFFile.iSize);
    if (pImage->TIFFFile.fHandle == NULL)
       return 0;
    return TIFFInit(pImage);

} /* openTIFFFile() */
#endif

int TIFF_openRAW(TIFFIMAGE *pImage, int iWidth, int iHeight, int iFillOrder, uint8_t *pData, int iDataSize, TIFF_DRAW_CALLBACK *pfnDraw)
{
    memset(pImage, 0, sizeof(TIFFIMAGE));
    pImage->pfnRead = readMem;
    pImage->pfnSeek = seekMem;
    pImage->pfnDraw = pfnDraw;
    pImage->pfnOpen = NULL;
    pImage->pfnClose = NULL;
    pImage->TIFFFile.iSize = iDataSize;
    pImage->TIFFFile.pData = pData;
    pImage->iWidth = iWidth;
    pImage->iHeight = iHeight;
    pImage->ucFillOrder = (uint8_t)iFillOrder;
    return 1;

} /* openRAW() */

void TIFF_close(TIFFIMAGE *pImage)
{
    if (pImage->pfnClose)
        (*pImage->pfnClose)(pImage->TIFFFile.fHandle);
} /* close() */

void TIFF_setDrawParameters(TIFFIMAGE *pImage, uint32_t scale, int iPixelType, int iStartX, int iStartY, int iWidth, int iHeight, uint8_t *p4BPPBuf)
{
    pImage->window.iScale = scale;
    pImage->window.x = iStartX;
    pImage->window.y = iStartY; // upper left corner of interest (source pixels)
    pImage->window.iWidth = iWidth; // width of destination window (for clipping purposes)
    pImage->window.iHeight = iHeight;
    pImage->window.p4BPP = p4BPPBuf; // user-supplied buffer for 4-bpp grayscale output
    pImage->window.ucPixelType = (uint8_t)iPixelType;

} /* setDrawParameters() */

int TIFF_decode(TIFFIMAGE *pImage)
{
    return Decode(pImage);
} /* decode() */

int TIFF_getWidth(TIFFIMAGE *pImage)
{
    return pImage->iWidth;
} /* getWidth() */

int TIFF_getHeight(TIFFIMAGE *pImage)
{
    return pImage->iHeight;
} /* getHeight() */

int TIFF_getLastError(TIFFIMAGE *pImage)
{
    return pImage->iError;
} /* getLastError() */

#endif // !__cplusplus
//
// The following functions are written in plain C and have no
// 3rd party dependencies, not even the C runtime library
//
//
// Initialize a TIFF file and callback access from a file on SD or memory
// returns 1 for success, 0 for failure
// Fills in the basic image info fields of the TIFFIMAGE structure
//
static int TIFFInit(TIFFIMAGE *pImage)
{
    return TIFFParseInfo(pImage); // gather info for image
} /* TIFFInit() */

//
// TIFFSHORT
// read a 16-bit unsigned integer from the given pointer
// and interpret the data as big endian (Motorola) or little endian (Intel)
//
static uint16_t TIFFSHORT(unsigned char *p, int bMotorola)
{
    unsigned short s;

    if (bMotorola)
        s = *p * 0x100 + *(p+1); // big endian (AKA Motorola byte order)
    else
        s = *p + *(p+1)*0x100; // little endian (AKA Intel byte order)
    return s;
} /* TIFFSHORT() */
//
// TIFFLONG
// read a 32-bit unsigned integer from the given pointer
// and interpret the data as big endian (Motorola) or little endian (Intel)
//
static uint32_t TIFFLONG(unsigned char *p, int bMotorola)
{
    uint32_t l;

    if (bMotorola)
        l = *p * 0x1000000 + *(p+1) * 0x10000 + *(p+2) * 0x100 + *(p+3); // big endian
    else
        l = *p + *(p+1) * 0x100 + *(p+2) * 0x10000 + *(p+3) * 0x1000000; // little endian
    return l;
} /* TIFFLONG() */
//
// TIFFVALUE
// read an integer value encoded in a TIFF TAG (12-byte structure)
// and interpret the data as big endian (Motorola) or little endian (Intel)
//
static int TIFFVALUE(unsigned char *p, int bMotorola)
{
    int i, iType;
    
    iType = TIFFSHORT(p+2, bMotorola);
    /* If pointer to a list of items, must be a long */
    if (TIFFSHORT(p+4, bMotorola) > 1)
    {
        iType = 4;
    }
    switch (iType)
    {
        case 3: /* Short */
            i = TIFFSHORT(p+8, bMotorola);
            break;
        case 4: /* Long */
        case 7: // undefined (treat it as a long since it's usually a multibyte buffer)
            i = TIFFLONG(p+8, bMotorola);
            break;
        case 6: // signed byte
            i = (signed char)p[8];
            break;
        case 2: /* ASCII */
        case 5: /* Unsigned Rational */
        case 10: /* Signed Rational */
            i = TIFFLONG(p+8, bMotorola);
            break;
        default: /* to suppress compiler warning */
            i = 0;
            break;
    }
    return i;
    
} /* TIFFVALUE() */

static int TIFFParseInfo(TIFFIMAGE *pPage)
{
    int iBytesRead;
    int i;
    uint8_t bMotorola, *s = pPage->ucFileBuf;
    uint16_t usTagCount;
    int iStripCount, IFD, iTag, iBpp = 1, iSamples = 1;
//    int iT6Options = 0;

    pPage->ucFillOrder = BITDIR_MSB_FIRST; // default to MSB first
    iBytesRead = (*pPage->pfnRead)(&pPage->TIFFFile, s, 8);
    if (iBytesRead != 8)
    {
        pPage->iError = TIFF_INVALID_FILE;
        return 0;
    }
    if (s[0] != s[1] || (s[0] != 'M' && s[0] != 'I'))
    {
        pPage->iError = TIFF_INVALID_FILE;
        return 0; // not a TIFF file
    }
    bMotorola = (s[0] == 'M');
    IFD = TIFFLONG(&s[4], bMotorola); // get IFD
    if (IFD > pPage->TIFFFile.iSize - (2 + 4*TIFF_TAG_SIZE)) // bad value
    {
        pPage->iError = TIFF_INVALID_FILE;
        return 0;
    }
    (*pPage->pfnSeek)(&pPage->TIFFFile, IFD);
    iBytesRead = (*pPage->pfnRead)(&pPage->TIFFFile, s, 2); // get tag count
    usTagCount = TIFFSHORT(s, bMotorola);
    if (iBytesRead != 2 || usTagCount < 4 || usTagCount > MAX_TIFF_TAGS)
    {
        pPage->iError = TIFF_INVALID_FILE; // something corrupt/wrong
        return 0;
    }
    // read just enough for the tag data
    iBytesRead = (*pPage->pfnRead)(&pPage->TIFFFile, s, TIFF_TAG_SIZE * usTagCount);
    if (iBytesRead != usTagCount * TIFF_TAG_SIZE)
    {
        pPage->iError = TIFF_INVALID_FILE; // something corrupt/wrong
        return 0;
    }
    for (i=0; i<usTagCount; i++)
    {
        iTag = TIFFSHORT(s, bMotorola);
        switch (iTag)
        {
            case 256: // width
                pPage->iWidth = TIFFVALUE(s, bMotorola);
                break;
            case 257: // height
                pPage->iHeight = TIFFVALUE(s, bMotorola);
                break;
            case 258: // bits per sample
                iBpp = TIFFVALUE(s, bMotorola);
                break;
            case 259: // compression
                pPage->ucCompression = (uint8_t)TIFFVALUE(s, bMotorola);
                break;
            case 262: // photometric value
                pPage->ucPhotometric = (uint8_t)TIFFVALUE(s, bMotorola);
                break;
            case 266: // fill order
                pPage->ucFillOrder = (uint8_t)TIFFVALUE(s, bMotorola);
                break;
            case 273: // strip info
                pPage->iStripOffset = TIFFVALUE(s, bMotorola);
                iStripCount = TIFFLONG(s + 4, bMotorola);
                if (iStripCount != 1)
                { // single strip files only (for now)
                    pPage->iError = TIFF_UNSUPPORTED_FEATURE;
                    return 0;
                }
                break;
            case 277: // samples per pixel
                iSamples = TIFFVALUE(s, bMotorola);
                break;
            case 278: // rows per strip
                //iRowsPerStrip = TIFFVALUE(s, bMotorola);
                break;
            case 279: // strip size
                pPage->iStripSize = TIFFVALUE(s, bMotorola);
                break;
//            case 293: // T6 option flags
//                iT6Options = TIFFVALUE(s, bMotorola);
//                break;
        } // while
        s += TIFF_TAG_SIZE;
    }
    if (iSamples * iBpp != 1) // we only support 1-bpp images
    {
        pPage->iError = TIFF_UNSUPPORTED_FEATURE;
        return 0;
    }
    // Default output values
    pPage->window.iScale = 65536; // 1.0 scale
    pPage->window.x = 0;
    pPage->window.y = 0; // upper left corner of interest (source pixels)
    pPage->window.iWidth = pPage->iWidth; // dest window size
    pPage->window.iHeight = pPage->iHeight;
    pPage->window.ucPixelType = TIFF_PIXEL_1BPP;

    return 1;
} /* TIFFParseInfo() */

//
// Read (and optionally bit flip) more VLC data for decoding
//
static void TIFFGetMoreData(TIFFIMAGE *pPage)
{
//    printf("Getting more data...\n");
    // move any existing data down
    if ((pPage->iVLCSize - pPage->iVLCOff) >= FILE_HIGHWATER)
        return; // buffer is already full; no need to read more data
    if (pPage->iVLCOff != 0)
    {
        memcpy(&pPage->ucFileBuf[0], &pPage->ucFileBuf[pPage->iVLCOff], pPage->iVLCSize - pPage->iVLCOff);
        pPage->iVLCSize -= pPage->iVLCOff;
        pPage->iVLCOff = 0;
    }
    if (pPage->TIFFFile.iPos < pPage->TIFFFile.iSize && pPage->iVLCSize < FILE_HIGHWATER)
    {
        int i, iBytesRead;
        // Try to read enough to fill the buffer
        iBytesRead = (*pPage->pfnRead)(&pPage->TIFFFile, &pPage->ucFileBuf[pPage->iVLCSize], FILE_BUF_SIZE - pPage->iVLCSize); // max length we can read
        // flip bit direction if needed
        if (pPage->ucFillOrder == BITDIR_LSB_FIRST)
        {
            for (i=0; i<iBytesRead; i++)
            {
                uint8_t c = pPage->ucFileBuf[i+pPage->iVLCSize];
                pPage->ucFileBuf[i+pPage->iVLCSize] = pgm_read_byte(&ucMirror[c]);
            }
        }
        pPage->iVLCSize += iBytesRead;
    }
} /* TIFFGetMoreData() */
//
// Width is the doubled pixel width
// Convert 1-bpp into 2-bit grayscale
//
static void Scale2Gray(uint8_t *source, int width, int iPitch)
{
    int x;
    uint8_t ucPixels, c, d, *dest;
    
    dest = source; // write the new pixels over the old to save memory
    
    for (x=0; x<width/8; x+=2) /* Convert a pair of lines to gray */
    {
        c = source[x];  // first 4x2 block
        d = source[x+iPitch];
        /* two lines of 8 pixels are converted to one line of 4 pixels */
        ucPixels = (ucGray2BPP[(unsigned char)((c & 0xf0) | (d >> 4))] << 4);
        ucPixels |= (ucGray2BPP[(unsigned char)((c << 4) | (d & 0x0f))]);
        *dest++ = ucPixels;
        c = source[x+1];  // next 4x2 block
        d = source[x+iPitch+1];
        ucPixels = (ucGray2BPP[(unsigned char)((c & 0xf0) | (d >> 4))])<<4;
        ucPixels |= ucGray2BPP[(unsigned char)((c << 4) | (d & 0x0f))];
        *dest++ = ucPixels;
    }
    if (width & 4) // 2 more pixels to do
    {
        c = source[x];
        d = source[x + iPitch];
        ucPixels = (ucGray2BPP[(unsigned char) ((c & 0xf0) | (d >> 4))]) << 4;
        ucPixels |= (ucGray2BPP[(unsigned char) ((c << 4) | (d & 0x0f))]);
        dest[0] = ucPixels;
    }
} /* Scale2Gray() */
//
// Width is the doubled pixel width
// Convert 1-bpp into 4-bit grayscale
//
static void Scale2Gray4BPP(uint8_t *source, uint8_t *dest, int width, int iPitch)
{
    int x;
    uint8_t c, d;

    for (x=0; x<width/8; x+=2) /* Convert a pair of lines to gray */
    {
        c = source[x];  // first 4x2 block
        d = source[x+iPitch];
        /* two lines of 8 pixels are converted to one line of 4 pixels */
        *dest++ = ucGray4BPP[(unsigned char)((c & 0xf0) | (d >> 4))]; // first pair
        *dest++ = ucGray4BPP[(unsigned char)((c << 4) | (d & 0x0f))];
        c = source[x+1];  // next 4x2 block
        d = source[x+iPitch+1];
        *dest++ = ucGray4BPP[(unsigned char)((c & 0xf0) | (d >> 4))]; // second pair
        *dest++ = ucGray4BPP[(unsigned char)((c << 4) | (d & 0x0f))];
    }
    if (width & 4) // 2 more pixels to do
    {
        c = source[x];
        d = source[x + iPitch];
        *dest++ = ucGray4BPP[(unsigned char) ((c & 0xf0) | (d >> 4))];
        *dest++ = ucGray4BPP[(unsigned char)((c << 4) | (d & 0x0f))];

    }
} /* Scale2Gray4BPP() */

static void TIFFDrawLine(TIFFIMAGE *pPage, int y, int16_t *pCurFlips)
{
    int x, len, run, sx, srun;
    uint8_t lBit, rBit, *p;
    int iStart = 0, xright = pPage->iWidth;
    uint32_t u32ScaleFactor;
    uint8_t *pDest;
    TIFFDRAW obgd;

    u32ScaleFactor = pPage->window.iScale;
    obgd.iWidth = pPage->iWidth; // original image size
    obgd.iHeight = pPage->iHeight;
    obgd.iScaledWidth = (pPage->iWidth * u32ScaleFactor) >> 16;
    iStart = pPage->window.x;
    
    if (y == pPage->window.y) // start first line at white
    {
        pPage->u32Accum = 0;
        pPage->y = 0; // old Y value
        pPage->iPitch = (pPage->iWidth+7)>>3;
        if (u32ScaleFactor != 0x10000)
            pPage->iPitch = (((pPage->iWidth * u32ScaleFactor) >> 16) + 7) >> 3;
        if (pPage->window.ucPixelType >= TIFF_PIXEL_2BPP)
        {
            pPage->iPitch *= 2; // scale-to-gray is 4x as much memory
            if (pPage->iPitch*2 < MAX_BUFFERED_PIXELS)
                memset(pPage->ucPixels, 0xff, pPage->iPitch*2); // start as 0xff (white)
        }
        else
        {
            if (pPage->iPitch < MAX_BUFFERED_PIXELS)
                memset(pPage->ucPixels, 0xff, pPage->iPitch); // start as 0xff (white)
        }
    }
    pDest = pPage->ucPixels;
    if (pPage->window.ucPixelType >= TIFF_PIXEL_2BPP)
       {
           u32ScaleFactor <<= 1; // double the scale
           if ((pPage->u32Accum >> 16) >= 1) // second line
               pDest = &pPage->ucPixels[pPage->iPitch];
       }
    pPage->u32Accum += u32ScaleFactor;
    if ((y < pPage->window.y) || (y & 1 && u32ScaleFactor < 0x4000))
        return; // no need to draw anything, if shrinking too tiny, skip every line
       x = 0;
       while (x < xright) // while the scaled x is within the window bounds
        {
            x = *pCurFlips++; // black starting point
            run = *pCurFlips++ - x; // get the black run
            x -= iStart;
          if (x >= xright || run == 0)
             break;
          if ((x + run) > 0) /* If the run is visible, draw it */
             {
             if (x < 0)
                {
                run += x; /* draw only visible part of run */
                x = 0;
                }
          /* Scale the starting x and run down to size */
             if ((x + run) > xright) /* Don't let it go off right edge */
                run = xright - x;
             sx = (x * u32ScaleFactor)>>16;
             srun = (run * u32ScaleFactor)>>16;
             if (srun < 1) /* Always draw at least one pixel */
                srun = 1;
             /* Draw this run */
             lBit = 0xff << (8 - (sx & 7));
             rBit = 0xff >> ((sx + srun) & 7);
             len = ((sx+srun)>>3) - (sx >> 3);
             p = &pDest[sx >> 3];
             if (len == 0)
                {
                lBit |= rBit;
                *p &= lBit;
                }
             else
                {
                *p++ &= lBit;
                while (len > 1)
                   {
                   *p++ = 0;
                   len--;
                   }
                *p = rBit;
                }
             }
          } /* while drawing line */
    obgd.ucLast = 0;
    if (y == pPage->iHeight-1) // last line, force a final draw
    {
        pPage->u32Accum = 0x20000;
        obgd.ucLast = 1;
    }
    obgd.ucPixelType = pPage->window.ucPixelType;
    if (obgd.ucPixelType >= TIFF_PIXEL_2BPP)
    {
        if ((pPage->u32Accum >> 16) >= 2)
        {
            // Time to output the two lines as scale-to-gray
            // Convert the stretched pixels to 2-bit grayscale
            if (obgd.ucPixelType == TIFF_PIXEL_2BPP)
            {
                obgd.pPixels = pPage->ucPixels;
                Scale2Gray(pPage->ucPixels, obgd.iWidth*2, pPage->iPitch);
            }
            else
            {
                obgd.pPixels = pPage->window.p4BPP; // need a larger buffer for 4-bit pixels
                Scale2Gray4BPP(pPage->ucPixels, pPage->window.p4BPP, obgd.iWidth*2, pPage->iPitch);
            }
            // When stretching the image, we may need to repeat lines
            while (pPage->u32Accum >= 0x20000)
            {
                obgd.y = pPage->y;
                (*pPage->pfnDraw)(&obgd); // callback
                pPage->y++;
                pPage->u32Accum -= 0x20000;
            }
            if (pPage->iPitch*2 < MAX_BUFFERED_PIXELS)
                memset(pPage->ucPixels, 0xff, pPage->iPitch*2); // start as 0xff (white)
        }
    }
    else if ((pPage->u32Accum >> 16) >= 1) // time to output the 1 line
    {
        obgd.pPixels = pPage->ucPixels;
        // When stretching the image, we may need to repeat lines
        while (pPage->u32Accum >= 0x10000)
        {
            obgd.y = pPage->y;
            (*pPage->pfnDraw)(&obgd); // callback
            pPage->y++;
            pPage->u32Accum -= 0x10000;
        }
        if (pPage->iPitch < MAX_BUFFERED_PIXELS)
            memset(pPage->ucPixels, 0xff, pPage->iPitch); // start as 0xff (white)
    }
    
} /* TIFFDrawLine() */
//
// Decompress the VLC data
//
static int Decode(TIFFIMAGE *pPage)
{
int i, y, xsize, tot_run, tot_run1 = 0;
int32_t sCode;
int16_t *t1, *pCur, *pRef;
int16_t *CurFlips, *RefFlips;
uint32_t lBits;
uint32_t ulBits, ulBitOff;
uint8_t *pBuf, *pBufEnd;

    xsize = pPage->iWidth; /* For performance reasons */
    CurFlips = pPage->CurFlips;
    RefFlips = pPage->RefFlips;
    
   /* Seed the current and reference line with XSIZE for V(0) codes */
    for (i=0; i<xsize-2; i++)
    {
        RefFlips[i] = xsize;
        CurFlips[i] = xsize;
    }
   /* Prefill both current and reference lines with 7fff to prevent it from
      walking off the end if the data gets bunged and the current X is > XSIZE
      3-16-94 */
    CurFlips[i] = RefFlips[i] = 0x7fff;
    CurFlips[i+1] = RefFlips[i+1] = 0x7fff;

    pPage->iVLCSize = pPage->iVLCOff = 0;
    (*pPage->pfnSeek)(&pPage->TIFFFile, pPage->iStripOffset); // start of data
    TIFFGetMoreData(pPage); // read first block of compressed data
    pBuf = pPage->ucFileBuf;
    pBufEnd = &pBuf[FILE_HIGHWATER];
    ulBitOff = 0;
//
// Some files may have leading 0's that would confuse the decoder
// Valid G4 data can't begin with a 0
//
   while (pBuf < pBufEnd && pBuf[0] == 0)
   { pBuf++; }

   ulBits = MOTOLONG(pBuf); // load 32 bits to start
    
   /* Decode the image */
   for (y=0; y < pPage->iHeight; y++)
      {
      signed int a0, a0_c, a0_p, b1;
//g4_restart:
//      iCur = iRef = 0; /* Point to start of current and reference line */
      pCur = CurFlips;
      pRef = RefFlips;
      a0 = -1;
      a0_c = 0; /* start just to left and white */
      if (pBuf >= pBufEnd) // time to read more data
      {
          pPage->iVLCOff = (int)(pBuf - pPage->ucFileBuf);
          TIFFGetMoreData(pPage);
          pBuf = pPage->ucFileBuf;
      }
      while (a0 < xsize)   /* Decode this line */
         {
         if (ulBitOff > (REGISTER_WIDTH-8)) // need at least 7 unused bits
            {
            pBuf += (ulBitOff >> 3);
            ulBitOff &= 7;
            ulBits = MOTOLONG(pBuf);
            }
         if ((int32_t)(ulBits << ulBitOff) < 0)  /* V(0) code */
            {
            a0 = *pRef++;
            ulBitOff++; // 1 bit
            a0_c = 1 - a0_c; /* color change */
            *pCur++ = a0;
            }
         else /* Slow method */
            {
            lBits = (ulBits >> ((REGISTER_WIDTH - 8) - ulBitOff)) & 0xfe; /* Only the first 7 bits are useful */
            sCode = pgm_read_byte(&code_table[lBits]); /* Get the code word */
            ulBitOff += pgm_read_byte(&code_table[lBits+1]); /* Get the code length */
            switch (sCode)
               {
               case 1: /* V(-1) */
               case 2: /* V(-2) */
               case 3: /* V(-3) */
                  a0 = *pRef - sCode;  /* A0 = B1 - x */
                  *pCur++ = a0;
                  if (pRef == RefFlips)
                     pRef += 2;
                  pRef--;
                  while (a0 >= *pRef)
                     pRef += 2;
                  a0_c = 1-a0_c; /* color change */
                  break;

               case 0x11: /* V(1) */
               case 0x12: /* V(2) */
               case 0x13: /* V(3) */
                  a0 = *pRef++;   /* A0 = B1 */
                  b1 = a0;
                  a0 += sCode & 7;      /* A0 = B1 + x */
                  if (b1 != xsize && a0 < xsize)
                     {
                     while (a0 >= *pRef)
                        pRef += 2;
                     }
                  if (a0 > xsize)
                     a0 = xsize;
                  a0_c = 1-a0_c; /* color change */
                  *pCur++ = a0;
                  break;

               case 0x20: /* Horizontal codes */
                  a0_p = a0;
                  if (a0 < 0)
                     a0_p = 0;
                  if (a0_c) /* Black case */
                     {
                     CLIMBBLACK_NEW(pBuf, ulBitOff, ulBits, sCode)
//                     sCode = ClimbBlack(&bb);
                     if (sCode < 0)
                        {
                        pPage->iError = TIFF_DECODE_ERROR;
                        goto pilreadg4z;
                        }
                     tot_run = sCode;
                     CLIMBWHITE_NEW(pBuf, ulBitOff, ulBits, sCode)
//                     sCode = ClimbWhite(&bb);
                     if (sCode < 0)
                        {
                        pPage->iError = TIFF_DECODE_ERROR;
                        goto pilreadg4z;
                        }
                     tot_run1 = sCode;
                     }
                  else  /* White case */
                     {
                      CLIMBWHITE_NEW(pBuf, ulBitOff, ulBits, sCode)
//                     sCode = ClimbWhite(&bb);
                     if (sCode < 0)
                        {
                        pPage->iError = TIFF_DECODE_ERROR;
                        goto pilreadg4z;
                        }
                     tot_run = sCode;
                     CLIMBBLACK_NEW(pBuf, ulBitOff, ulBits, sCode)
//                     sCode = ClimbBlack(&bb);
                     if (sCode < 0)
                        {
                        pPage->iError = TIFF_DECODE_ERROR;
                        goto pilreadg4z;
                        }
                     tot_run1 = sCode;
                     }
                  a0 = a0_p + tot_run;
                  *pCur++ = a0;
                  a0 += tot_run1;
                  if (a0 < xsize)
                     while (a0 >= *pRef)
                        pRef += 2;
                  *pCur++ = a0;
                  break;

               case 0x30: /* Pass code */
                  pRef++;         /* A0 = B2, iRef+=2 */
                  a0 = *pRef++;
                  break;
               case 0x40: /* Uncompressed mode */
                  lBits = ulBits << ulBitOff;
                  lBits &= 0xffc00000;
                  if (lBits != 0x3c00000)  /* If not entering uncompressed mode */
                     {
                     pPage->iError = TIFF_DECODE_ERROR;
                     goto pilreadg4z;
                     }
                  ulBitOff += 10;
                  tot_run = 0; /* Current run length */
                  if ((ulBits << ulBitOff) & TOP_BIT)
                     goto blkst; /* Black start */
            whtst:
                  if (/*iCur >= xsize || */pBuf > pBufEnd)    /* Something is wrong, stop */
                     {
                     pPage->iError = TIFF_DECODE_ERROR;
                     goto pilreadg4z;
                     }
                  tot_run1++;
                  ulBitOff++;
                  if (ulBitOff > (REGISTER_WIDTH - 8))
                     {
                     pBuf += (ulBitOff >> 3);
                     ulBitOff &= 7;
                     ulBits = MOTOLONG(pBuf);
                     }
                  lBits = ulBits << ulBitOff;
                  if ((lBits & TOP_BIT) == 0)
                     goto whtst;
        /* Check for end of mode stuff */
                  if (tot_run1 == 5)
                     {
                     tot_run += 5;
                     tot_run1 = -1;
                     goto whtst; /* Keep looking for white */
                     }
                  if (tot_run1 >= 6) /* End of uncomp data */
                     {
                     tot_run += tot_run1 - 6; /* Get the number of extra 0's */
                     if (tot_run) /* Something to store? */
                        {
                        a0 += tot_run;
                        *pCur++ = a0;
                        }
     /* Get the last bit to see what the next color is */
                     ulBitOff++;
                     if (ulBitOff > (REGISTER_WIDTH - 8))
                        {
                        pBuf += (ulBitOff >> 3);
                        ulBitOff &= 7;
                        ulBits = MOTOLONG(pBuf);
                        }
                     lBits = ulBits << ulBitOff;
                     lBits >>= (REGISTER_WIDTH - 1); /* Turn it into 0/1 for color */
                     if ((signed int)lBits != a0_c) /* If color changed, bump up ref line */
                        pRef++;
                     a0_c = sCode; /* This is the new color */
/* Re-align reference line with new position */
                     while (*pRef <= a0) // && iRef <= xsize) - DEBUG
                        pRef += 2;
                     break; /* Continue normal G4 decoding */
                     }
                  else
                     {
                     tot_run += tot_run1;
                     a0 += tot_run; /* Add to current x */
                     *pCur++ = a0;
                     tot_run = 0;
                     tot_run1 = 0;
                     }
            blkst:
                  if (/*iCur >= xsize || */ pBuf > pBufEnd)    /* Something is wrong, stop */
                     {
                     pPage->iError = TIFF_DECODE_ERROR;
                     goto pilreadg4z;
                     }
                  tot_run++;
                  ulBitOff++;
                  if (ulBitOff > (REGISTER_WIDTH - 8))
                     {
                     pBuf += (ulBitOff >> 3);
                     ulBitOff &= 7;
                     ulBits = MOTOLONG(pBuf);
                     }
                  lBits = ulBits << ulBitOff;
                  if ((lBits & TOP_BIT) == TOP_BIT)
                     goto blkst;
                  a0 += tot_run;
                  *pCur++ = a0;
                  tot_run = 0;
                  goto whtst;
               default: /* possible ERROR! */
                  /* A G4 page can end early with 2 EOL's */
                   CLIMBWHITE_NEW(pBuf, ulBitOff, ulBits, sCode)
//                  sCode = ClimbWhite(&bb);
                  if (sCode != EOL)
                     {
                     pPage->iError = TIFF_DECODE_ERROR;
                     goto pilreadg4z;
                     }
                   CLIMBWHITE_NEW(pBuf, ulBitOff, ulBits, sCode)
//                  sCode = ClimbWhite(&bb);
                  if (sCode != EOL)
                     {
                     pPage->iError = TIFF_DECODE_ERROR;
                     goto pilreadg4z;
                     }
                  goto pilreadg4z; /* Leave gracefully */
               } /* switch */
            } /* Slow climb */
         }
      /*--- Convert flips data into run lengths ---*/
      *pCur++ = xsize;  /* Terminate the line properly */
      *pCur++ = xsize;

        // Draw the current line
      TIFFDrawLine(pPage, y, CurFlips);
      /*--- Swap current and reference lines ---*/
      t1 = RefFlips;
      RefFlips = CurFlips;
      CurFlips = t1;
      } /* for */
  pilreadg4z:
//    pOutPage->iLinesDecoded = y; // tell caller how many lines successfully decoded
//   if (pPage->iOptions & PIL_CONVERT_IGNORE_ERRORS)
//      pPage->iError = 0; // suppress errors

   return (pPage->iError == 0);

} /* Decode() */
