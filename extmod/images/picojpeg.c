//------------------------------------------------------------------------------
// picojpeg.c v1.1 - Public domain, Rich Geldreich <richgel99@gmail.com>
// Nov. 27, 2010 - Initial release
// Feb. 9, 2013 - Added H1V2/H2V1 support, cleaned up macros, signed shift fixes 
// Also integrated and tested changes from Chris Phoenix <cphoenix@gmail.com>.
// Oct. 16, 2018 - Made reentrant, added scanline output, added RGB565 output
// Changes from Scott Wagner <scott.wagner@promethean-design.com>
//------------------------------------------------------------------------------
#include "picojpeg.h"
#include <stdio.h>
//------------------------------------------------------------------------------
// Set to 1 if right shifts on signed ints are always unsigned (logical) shifts
// When 1, arithmetic right shifts will be emulated by using a logical shift
// with special case code to ensure the sign bit is replicated.
#define PJPG_RIGHT_SHIFT_IS_ALWAYS_UNSIGNED 0

// Define PJPG_INLINE to "inline" if your C compiler supports explicit inlining
#define PJPG_INLINE

// Set RGB565_LITTLE_ENDIAN to 1 for little-endian byte order in RGB565 uint16 pixels
#if !defined RGB565_LITTLE_ENDIAN
#define RGB565_LITTLE_ENDIAN 1
#endif

//------------------------------------------------------------------------------
typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef signed char     int8;
typedef signed short    int16;
//------------------------------------------------------------------------------
#if PJPG_RIGHT_SHIFT_IS_ALWAYS_UNSIGNED
static int16 replicateSignBit16(int8 n)
{
   switch (n)
   {
      case 0:  return 0x0000;
      case 1:  return 0x8000;
      case 2:  return 0xC000;
      case 3:  return 0xE000;
      case 4:  return 0xF000;
      case 5:  return 0xF800;
      case 6:  return 0xFC00;
      case 7:  return 0xFE00;
      case 8:  return 0xFF00;
      case 9:  return 0xFF80;
      case 10: return 0xFFC0;
      case 11: return 0xFFE0;
      case 12: return 0xFFF0; 
      case 13: return 0xFFF8;
      case 14: return 0xFFFC;
      case 15: return 0xFFFE;
      default: return 0xFFFF;
   }
}
static PJPG_INLINE int16 arithmeticRightShiftN16(int16 x, int8 n) 
{
   int16 r = (uint16)x >> (uint8)n;
   if (x < 0)
      r |= replicateSignBit16(n);
   return r;
}
static PJPG_INLINE long arithmeticRightShift8L(long x) 
{
   long r = (unsigned long)x >> 8U;
   if (x < 0)
      r |= ~(~(unsigned long)0U >> 8U);
   return r;
}
#define PJPG_ARITH_SHIFT_RIGHT_N_16(x, n) arithmeticRightShiftN16(x, n)
#define PJPG_ARITH_SHIFT_RIGHT_8_L(x) arithmeticRightShift8L(x)
#else
#define PJPG_ARITH_SHIFT_RIGHT_N_16(x, n) ((x) >> (n))
#define PJPG_ARITH_SHIFT_RIGHT_8_L(x) ((x) >> 8)
#endif
//------------------------------------------------------------------------------
// Change as needed - the PJPG_MAX_WIDTH/PJPG_MAX_HEIGHT checks are only present
// to quickly detect bogus files.
#define PJPG_MAX_WIDTH 16384
#define PJPG_MAX_HEIGHT 16384
#define PJPG_MAXCOMPSINSCAN 3
//------------------------------------------------------------------------------
typedef enum
{
   M_SOF0  = 0xC0,
   M_SOF1  = 0xC1,
   M_SOF2  = 0xC2,
   M_SOF3  = 0xC3,

   M_SOF5  = 0xC5,
   M_SOF6  = 0xC6,
   M_SOF7  = 0xC7,

   M_JPG   = 0xC8,
   M_SOF9  = 0xC9,
   M_SOF10 = 0xCA,
   M_SOF11 = 0xCB,

   M_SOF13 = 0xCD,
   M_SOF14 = 0xCE,
   M_SOF15 = 0xCF,

   M_DHT   = 0xC4,

   M_DAC   = 0xCC,

   M_RST0  = 0xD0,
   M_RST1  = 0xD1,
   M_RST2  = 0xD2,
   M_RST3  = 0xD3,
   M_RST4  = 0xD4,
   M_RST5  = 0xD5,
   M_RST6  = 0xD6,
   M_RST7  = 0xD7,

   M_SOI   = 0xD8,
   M_EOI   = 0xD9,
   M_SOS   = 0xDA,
   M_DQT   = 0xDB,
   M_DNL   = 0xDC,
   M_DRI   = 0xDD,
   M_DHP   = 0xDE,
   M_EXP   = 0xDF,

   M_APP0  = 0xE0,
   M_APP15 = 0xEF,

   M_JPG0  = 0xF0,
   M_JPG13 = 0xFD,
   M_COM   = 0xFE,

   M_TEM   = 0x01,

   M_ERROR = 0x100,
   
   RST0    = 0xD0
} JPEG_MARKER;
//------------------------------------------------------------------------------
static const int8 ZAG[] = 
{
   0,  1,  8, 16,  9,  2,  3, 10,
   17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34,
   27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36,
   29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46,
   53, 60, 61, 54, 47, 55, 62, 63,
};
//------------------------------------------------------------------------------
#define PJPG_MAX_IN_BUF_SIZE 256

typedef struct HuffTableT {
    uint16 mMinCode[16];
    uint16 mMaxCode[16];
    uint8 mValPtr[16];
} HuffTable;

typedef struct PicoJpegT {
    int16 CoeffBuf[8*8];  // 128 bytes
    uint8 MCUBufR[256];   // 8*8*4 bytes * 3 = 768
    uint8 MCUBufG[256];
    uint8 MCUBufB[256];
    int16 Quant0[8*8];    // 256 bytes
    int16 Quant1[8*8];
    int16 LastDC[3];      // 6 bytes
    HuffTable HuffTab0;   // DC - 192
    uint8 HuffVal0[16];
    HuffTable HuffTab1;
    uint8 HuffVal1[16];
    HuffTable HuffTab2;   // AC - 672
    uint8 HuffVal2[256];
    HuffTable HuffTab3;
    uint8 HuffVal3[256];
    uint8 ValidHuffTables;
    uint8 ValidQuantTables;
    uint8 TemFlag;
    uint8 InBufOfs;
    uint8 InBufLeft;
    uint16 BitBuf;
    uint8 BitsLeft;
    uint16 ImageXSize;
    uint16 ImageYSize;
    uint8 CompsInFrame;
    uint8 CompIdent[3];
    uint8 CompHSamp[3];
    uint8 CompVSamp[3];
    uint8 CompQuant[3];
    uint16 RestartInterval;
    uint16 NextRestartNum;
    uint16 RestartsLeft;
    uint8 CompsInScan;
    uint8 CompList[3];
    uint8 CompDCTab[3]; // 0,1
    uint8 CompACTab[3]; // 0,1
    pjpeg_scan_type_t ScanType;
    uint8 MaxBlocksPerMCU;
    uint8 MaxMCUXSize;
    uint8 MaxMCUYSize;
    uint16 MaxMCUSPerRow;
    uint16 MaxMCUSPerCol;
    uint16 NumMCUSRemaining;
    uint8 MCUOrg[6];
    jsread_t *Jsread;
    void *pCallback_data;
    uint8 CallbackStatus;
    uint8 output_type;
    uint16 ImageXWindow;
    uint16 ImageYWindow;
    uint16 ImageXOffset;
    uint16 ImageYOffset;
    uint8 InBuf[PJPG_MAX_IN_BUF_SIZE];
} PicoJpeg;

//------------------------------------------------------------------------------
static void fillInBuf(PicoJpeg *pjp)
{
   int nread;
   // Reserve a few bytes at the beginning of the buffer for putting back ("stuffing") chars.
   pjp->InBufOfs = 4;
   pjp->InBufLeft = 0;

   nread = (*pjp->Jsread)(pjp->InBuf + pjp->InBufOfs, PJPG_MAX_IN_BUF_SIZE - pjp->InBufOfs, pjp->pCallback_data);
   if (nread < 0) {
      // The user provided need bytes callback has indicated an error, so record the error and continue trying to decode.
      // The highest level pjpeg entrypoints will catch the error and return the non-zero status.
      pjp->CallbackStatus = (unsigned char)(-nread);
   } else {
      pjp->InBufLeft = (unsigned char)nread;
   }
}

//------------------------------------------------------------------------------
static PJPG_INLINE uint8 getChar(PicoJpeg *pjp)
{
   if (!pjp->InBufLeft)
   {
      fillInBuf(pjp);
      if (!pjp->InBufLeft)
      {
         pjp->TemFlag = ~pjp->TemFlag;
         return pjp->TemFlag ? 0xFF : 0xD9;
      } 
   }
   
   pjp->InBufLeft--;
   return pjp->InBuf[pjp->InBufOfs++];
}
//------------------------------------------------------------------------------
static PJPG_INLINE void stuffChar(PicoJpeg *pjp, uint8 i)
{
   pjp->InBufOfs--;
   pjp->InBuf[pjp->InBufOfs] = i;
   pjp->InBufLeft++;
}
//------------------------------------------------------------------------------
static PJPG_INLINE uint8 getOctet(PicoJpeg *pjp, uint8 FFCheck)
{
   uint8 c = getChar(pjp);
      
   if ((FFCheck) && (c == 0xFF))
   {
      uint8 n = getChar(pjp);

      if (n)
      {
         stuffChar(pjp, n);
         stuffChar(pjp, 0xFF);
      }
   }

   return c;
}
//------------------------------------------------------------------------------
static uint16 getBits(PicoJpeg *pjp, uint8 numBits, uint8 FFCheck)
{
   uint8 origBits = numBits;
   uint16 ret = pjp->BitBuf;
   
   if (numBits > 8)
   {
      numBits -= 8;
      
      pjp->BitBuf <<= pjp->BitsLeft;
      
      pjp->BitBuf |= getOctet(pjp, FFCheck);
      
      pjp->BitBuf <<= (8 - pjp->BitsLeft);
      
      ret = (ret & 0xFF00) | (pjp->BitBuf >> 8);
   }
      
   if (pjp->BitsLeft < numBits)
   {
      pjp->BitBuf <<= pjp->BitsLeft;
      
      pjp->BitBuf |= getOctet(pjp, FFCheck);
      
      pjp->BitBuf <<= (numBits - pjp->BitsLeft);
                        
      pjp->BitsLeft = 8 - (numBits - pjp->BitsLeft);
   }
   else
   {
      pjp->BitsLeft = (uint8)(pjp->BitsLeft - numBits);
      pjp->BitBuf <<= numBits;
   }
   
   return ret >> (16 - origBits);
}
//------------------------------------------------------------------------------
static PJPG_INLINE uint16 getBits1(PicoJpeg *pjp, uint8 numBits)
{
   return getBits(pjp, numBits, 0);
}
//------------------------------------------------------------------------------
static PJPG_INLINE uint16 getBits2(PicoJpeg *pjp, uint8 numBits)
{
   return getBits(pjp, numBits, 1);
}
//------------------------------------------------------------------------------
static PJPG_INLINE uint8 getBit(PicoJpeg *pjp)
{
   uint8 ret = 0;
   if (pjp->BitBuf & 0x8000) 
      ret = 1;
   
   if (!pjp->BitsLeft)
   {
      pjp->BitBuf |= getOctet(pjp, 1);

      pjp->BitsLeft += 8;
   }
   
   pjp->BitsLeft--;
   pjp->BitBuf <<= 1;
   
   return ret;
}
//------------------------------------------------------------------------------
static uint16 getExtendTest(uint8 i)
{
   switch (i)
   {
      case 0: return 0;
      case 1: return 0x0001;
      case 2: return 0x0002;
      case 3: return 0x0004;
      case 4: return 0x0008;
      case 5: return 0x0010; 
      case 6: return 0x0020;
      case 7: return 0x0040;
      case 8:  return 0x0080;
      case 9:  return 0x0100;
      case 10: return 0x0200;
      case 11: return 0x0400;
      case 12: return 0x0800;
      case 13: return 0x1000;
      case 14: return 0x2000; 
      case 15: return 0x4000;
      default: return 0;
   }      
}
//------------------------------------------------------------------------------
static int16 getExtendOffset(uint8 i)
{ 
   switch (i)
   {
      case 0: return 0;
      case 1: return ((-1)<<1) + 1; 
      case 2: return ((-1)<<2) + 1; 
      case 3: return ((-1)<<3) + 1; 
      case 4: return ((-1)<<4) + 1; 
      case 5: return ((-1)<<5) + 1; 
      case 6: return ((-1)<<6) + 1; 
      case 7: return ((-1)<<7) + 1; 
      case 8: return ((-1)<<8) + 1; 
      case 9: return ((-1)<<9) + 1;
      case 10: return ((-1)<<10) + 1; 
      case 11: return ((-1)<<11) + 1; 
      case 12: return ((-1)<<12) + 1; 
      case 13: return ((-1)<<13) + 1; 
      case 14: return ((-1)<<14) + 1; 
      case 15: return ((-1)<<15) + 1;
      default: return 0;
   }
};
//------------------------------------------------------------------------------
static PJPG_INLINE int16 huffExtend(uint16 x, uint8 s)
{
   return ((x < getExtendTest(s)) ? ((int16)x + getExtendOffset(s)) : (int16)x);
}
//------------------------------------------------------------------------------
static PJPG_INLINE uint8 huffDecode(PicoJpeg *pjp, const HuffTable* pHuffTable, const uint8* pHuffVal)
{
   uint8 i = 0;
   uint8 j;
   uint16 code = getBit(pjp);

   // This func only reads a bit at a time, which on modern CPU's is not terribly efficient.
   // But on microcontrollers without strong integer shifting support this seems like a 
   // more reasonable approach.
   for ( ; ; )
   {
      uint16 maxCode;

      if (i == 16)
         return 0;

      maxCode = pHuffTable->mMaxCode[i];
      if ((code <= maxCode) && (maxCode != 0xFFFF))
         break;

      i++;
      code <<= 1;
      code |= getBit(pjp);
   }

   j = pHuffTable->mValPtr[i];
   j = (uint8)(j + (code - pHuffTable->mMinCode[i]));

   return pHuffVal[j];
}
//------------------------------------------------------------------------------
static void huffCreate(const uint8* pBits, HuffTable* pHuffTable)
{
   uint8 i = 0;
   uint8 j = 0;

   uint16 code = 0;
      
   for ( ; ; )
   {
      uint8 num = pBits[i];
      
      if (!num)
      {
         pHuffTable->mMinCode[i] = 0x0000;
         pHuffTable->mMaxCode[i] = 0xFFFF;
         pHuffTable->mValPtr[i] = 0;
      }
      else
      {
         pHuffTable->mMinCode[i] = code;
         pHuffTable->mMaxCode[i] = code + num - 1;
         pHuffTable->mValPtr[i] = j;
         
         j = (uint8)(j + num);
         
         code = (uint16)(code + num);
      }
      
      code <<= 1;
      
      i++;
      if (i > 15)
         break;
   }
}
//------------------------------------------------------------------------------
static HuffTable* getHuffTable(PicoJpeg *pjp, uint8 index)
{
   // 0-1 = DC
   // 2-3 = AC
   switch (index)
   {
      case 0: return &pjp->HuffTab0;
      case 1: return &pjp->HuffTab1;
      case 2: return &pjp->HuffTab2;
      case 3: return &pjp->HuffTab3;
      default: return 0;
   }
}
//------------------------------------------------------------------------------
static uint8* getHuffVal(PicoJpeg *pjp, uint8 index)
{
   // 0-1 = DC
   // 2-3 = AC
   switch (index)
   {
      case 0: return pjp->HuffVal0;
      case 1: return pjp->HuffVal1;
      case 2: return pjp->HuffVal2;
      case 3: return pjp->HuffVal3;
      default: return 0;
   }
}
//------------------------------------------------------------------------------
static uint16 getMaxHuffCodes(uint8 index)
{
   return (index < 2) ? 12 : 255;
}
//------------------------------------------------------------------------------
static uint8 readDHTMarker(PicoJpeg *pjp)
{
   uint8 bits[16];
   uint16 left = getBits1(pjp, 16);

   if (left < 2)
      return PJPG_BAD_DHT_MARKER;

   left -= 2;

   while (left)
   {
      uint8 i, tableIndex, index;
      uint8* pHuffVal;
      HuffTable* pHuffTable;
      uint16 count, totalRead;
            
      index = (uint8)getBits1(pjp, 8);
      
      if ( ((index & 0xF) > 1) || ((index & 0xF0) > 0x10) )
         return PJPG_BAD_DHT_INDEX;
      
      tableIndex = ((index >> 3) & 2) + (index & 1);
      
      pHuffTable = getHuffTable(pjp, tableIndex);
      pHuffVal = getHuffVal(pjp, tableIndex);
      
      pjp->ValidHuffTables |= (1 << tableIndex);
            
      count = 0;
      for (i = 0; i <= 15; i++)
      {
         uint8 n = (uint8)getBits1(pjp, 8);
         bits[i] = n;
         count = (uint16)(count + n);
      }
      
      if (count > getMaxHuffCodes(tableIndex))
         return PJPG_BAD_DHT_COUNTS;

      for (i = 0; i < count; i++)
         pHuffVal[i] = (uint8)getBits1(pjp, 8);

      totalRead = 1 + 16 + count;

      if (left < totalRead)
         return PJPG_BAD_DHT_MARKER;

      left = (uint16)(left - totalRead);

      huffCreate(bits, pHuffTable);
   }
      
   return 0;
}
//------------------------------------------------------------------------------
static void createWinogradQuant(int16* pQuant);

static uint8 readDQTMarker(PicoJpeg *pjp)
{
   uint16 left = getBits1(pjp, 16);

   if (left < 2)
      return PJPG_BAD_DQT_MARKER;

   left -= 2;

   while (left)
   {
      uint8 i;
      uint8 n = (uint8)getBits1(pjp, 8);
      uint8 prec = n >> 4;
      uint16 totalRead;

      n &= 0x0F;

      if (n > 1)
         return PJPG_BAD_DQT_TABLE;

      pjp->ValidQuantTables |= (n ? 2 : 1);         

      // read quantization entries, in zag order
      for (i = 0; i < 64; i++)
      {
         uint16 temp = getBits1(pjp, 8);

         if (prec)
            temp = (temp << 8) + getBits1(pjp, 8);

         if (n)
            pjp->Quant1[i] = (int16)temp;            
         else
            pjp->Quant0[i] = (int16)temp;            
      }
      
      createWinogradQuant(n ? pjp->Quant1 : pjp->Quant0);

      totalRead = 64 + 1;

      if (prec)
         totalRead += 64;

      if (left < totalRead)
         return PJPG_BAD_DQT_LENGTH;

      left = (uint16)(left - totalRead);
   }
   
   return 0;
}
//------------------------------------------------------------------------------
static uint8 readSOFMarker(PicoJpeg *pjp)
{
   uint8 i;
   uint16 left = getBits1(pjp, 16);

   if (getBits1(pjp, 8) != 8)   
      return PJPG_BAD_PRECISION;

   pjp->ImageYSize = getBits1(pjp, 16);

   if ((!pjp->ImageYSize) || (pjp->ImageYSize > PJPG_MAX_HEIGHT))
      return PJPG_BAD_HEIGHT;
   pjp->ImageYWindow = pjp->ImageYSize;
   pjp->ImageYOffset = 0;

   pjp->ImageXSize = getBits1(pjp, 16);

   if ((!pjp->ImageXSize) || (pjp->ImageXSize > PJPG_MAX_WIDTH))
      return PJPG_BAD_WIDTH;
   pjp->ImageXWindow = pjp->ImageXSize & (uint16)(~1);
   pjp->ImageXOffset = 0;

   pjp->CompsInFrame = (uint8)getBits1(pjp, 8);

   if (pjp->CompsInFrame > 3)
      return PJPG_TOO_MANY_COMPONENTS;

   if (left != (pjp->CompsInFrame + pjp->CompsInFrame + pjp->CompsInFrame + 8))
      return PJPG_BAD_SOF_LENGTH;

   if (pjp->CompsInFrame == 1) {
      if (pjp->output_type == PJPG_RGB888 ||
            pjp->output_type == PJPG_RGB565) {
         pjp->output_type = PJPG_GRAY8;
      } else if (pjp->output_type == PJPG_REDUCED_RGB888 ||
            pjp->output_type == PJPG_REDUCED_RGB565) {
         pjp->output_type = PJPG_REDUCED_GRAY8;
      }
   } else if (pjp->CompsInFrame == 3) {
      if (pjp->output_type == PJPG_GRAY8) {
         pjp->output_type = PJPG_RGB888;
      } else if (pjp->output_type == PJPG_REDUCED_GRAY8) {
         pjp->output_type = PJPG_REDUCED_RGB888;
      }
   }
   
   for (i = 0; i < pjp->CompsInFrame; i++)
   {
      pjp->CompIdent[i] = (uint8)getBits1(pjp, 8);
      pjp->CompHSamp[i] = (uint8)getBits1(pjp, 4);
      pjp->CompVSamp[i] = (uint8)getBits1(pjp, 4);
      pjp->CompQuant[i] = (uint8)getBits1(pjp, 8);
      
      if (pjp->CompQuant[i] > 1)
         return PJPG_UNSUPPORTED_QUANT_TABLE;
   }
   
   return 0;
}
//------------------------------------------------------------------------------
// Used to skip unrecognized markers.
static uint8 skipVariableMarker(PicoJpeg *pjp)
{
   uint16 left = getBits1(pjp, 16);

   if (left < 2)
      return PJPG_BAD_VARIABLE_MARKER;

   left -= 2;

   while (left)
   {
      getBits1(pjp, 8);
      left--;
   }
   
   return 0;
}
//------------------------------------------------------------------------------
// Read a define restart interval (DRI) marker.
static uint8 readDRIMarker(PicoJpeg *pjp)
{
   if (getBits1(pjp, 16) != 4)
      return PJPG_BAD_DRI_LENGTH;

   pjp->RestartInterval = getBits1(pjp, 16);
   
   return 0;
}
//------------------------------------------------------------------------------
// Read a start of scan (SOS) marker.
static uint8 readSOSMarker(PicoJpeg *pjp)
{
   uint8 i;
   uint16 left = getBits1(pjp, 16);
   uint8 spectral_start, spectral_end, successive_high, successive_low;

   pjp->CompsInScan = (uint8)getBits1(pjp, 8);

   left -= 3;

   if ( (left != (pjp->CompsInScan + pjp->CompsInScan + 3)) || (pjp->CompsInScan < 1) || (pjp->CompsInScan > PJPG_MAXCOMPSINSCAN) )
      return PJPG_BAD_SOS_LENGTH;
   
   for (i = 0; i < pjp->CompsInScan; i++)
   {
      uint8 cc = (uint8)getBits1(pjp, 8);
      uint8 c = (uint8)getBits1(pjp, 8);
      uint8 ci;
      
      left -= 2;
     
      for (ci = 0; ci < pjp->CompsInFrame; ci++)
         if (cc == pjp->CompIdent[ci])
            break;

      if (ci >= pjp->CompsInFrame)
         return PJPG_BAD_SOS_COMP_ID;

      pjp->CompList[i]    = ci;
      pjp->CompDCTab[ci] = (c >> 4) & 15;
      pjp->CompACTab[ci] = (c & 15);
   }

   spectral_start  = (uint8)getBits1(pjp, 8);
   spectral_end    = (uint8)getBits1(pjp, 8);
   successive_high = (uint8)getBits1(pjp, 4);
   successive_low  = (uint8)getBits1(pjp, 4);
   if (spectral_start != 0 || spectral_end != 63 || successive_high != 0 || successive_low != 0) {
      return PJPG_BAD_SOS_COMP_ID;
   }

   left -= 3;

   while (left)                  
   {
      getBits1(pjp, 8);
      left--;
   }
   
   return 0;
}
//------------------------------------------------------------------------------
static uint8 nextMarker(PicoJpeg *pjp)
{
   uint8 c;
   uint8 bytes = 0;

   do
   {
      do
      {
         bytes++;

         c = (uint8)getBits1(pjp, 8);

      } while (c != 0xFF);

      do
      {
         c = (uint8)getBits1(pjp, 8);

      } while (c == 0xFF);

   } while (c == 0);

   // If bytes > 0 here, there where extra bytes before the marker (not good).

   return c;
}
//------------------------------------------------------------------------------
// Process markers. Returns when an SOFx, SOI, EOI, or SOS marker is
// encountered.
static uint8 processMarkers(PicoJpeg *pjp, uint8* pMarker)
{
   for ( ; ; )
   {
      uint8 c = nextMarker(pjp);

      switch (c)
      {
         case M_SOF0:
         case M_SOF1:
         case M_SOF2:
         case M_SOF3:
         case M_SOF5:
         case M_SOF6:
         case M_SOF7:
         //      case M_JPG:
         case M_SOF9:
         case M_SOF10:
         case M_SOF11:
         case M_SOF13:
         case M_SOF14:
         case M_SOF15:
         case M_SOI:
         case M_EOI:
         case M_SOS:
         {
            *pMarker = c;
            return 0;
         }
         case M_DHT:
         {
            readDHTMarker(pjp);
            break;
         }
         // Sorry, no arithmetic support at this time. Dumb patents!
         case M_DAC:
         {
            return PJPG_NO_ARITHMITIC_SUPPORT;
         }
         case M_DQT:
         {
            readDQTMarker(pjp);
            break;
         }
         case M_DRI:
         {
            readDRIMarker(pjp);
            break;
         }
         //case M_APP0:  /* no need to read the JFIF marker */

         case M_JPG:
         case M_RST0:    /* no parameters */
         case M_RST1:
         case M_RST2:
         case M_RST3:
         case M_RST4:
         case M_RST5:
         case M_RST6:
         case M_RST7:
         case M_TEM:
         {
            return PJPG_UNEXPECTED_MARKER;
         }
         default:    /* must be DNL, DHP, EXP, APPn, JPGn, COM, or RESn or APP0 */
         {
            skipVariableMarker(pjp);
            break;
         }
      }
   }
//   return 0;
}
//------------------------------------------------------------------------------
// Finds the start of image (SOI) marker.
static uint8 locateSOIMarker(PicoJpeg *pjp)
{
   uint16 bytesleft;
   
   uint8 lastchar = (uint8)getBits1(pjp, 8);

   uint8 thischar = (uint8)getBits1(pjp, 8);

   /* ok if it's a normal JPEG file without a special header */

   if ((lastchar == 0xFF) && (thischar == M_SOI))
      return 0;

   bytesleft = 4096; //512;

   for ( ; ; )
   {
      if (--bytesleft == 0) {
         return PJPG_NOT_JPEG;
      }

      lastchar = thischar;

      thischar = (uint8)getBits1(pjp, 8);

      if (lastchar == 0xFF) 
      {
         if (thischar == M_SOI)
            break;
         else if (thischar == M_EOI) {	//getBits1 will keep returning M_EOI if we read past the end
            return PJPG_NOT_JPEG;
         }
      }
   }

   /* Check the next character after marker: if it's not 0xFF, it can't
   be the start of the next marker, so the file is bad */

   thischar = (uint8)((pjp->BitBuf >> 8) & 0xFF);

   if (thischar != 0xFF) {
      printf("PJPG_NOT_JPEG 894\n");
      return PJPG_NOT_JPEG;
   }

   return 0;
}
//------------------------------------------------------------------------------
// Find a start of frame (SOF) marker.
static uint8 locateSOFMarker(PicoJpeg *pjp)
{
   uint8 c;

   uint8 status = locateSOIMarker(pjp);
   if (status)
      return status;
   
   status = processMarkers(pjp, &c);
   if (status)
      return status;

   switch (c)
   {
      case M_SOF2:
      {
         // Progressive JPEG - not supported by picojpeg (would require too
         // much memory, or too many IDCT's for embedded systems).
         return PJPG_UNSUPPORTED_MODE;
      }
      case M_SOF0:  /* baseline DCT */
      {
         status = readSOFMarker(pjp);
         if (status)
            return status;
            
         break;
      }
      case M_SOF9:  
      {
         return PJPG_NO_ARITHMITIC_SUPPORT;
      }
      case M_SOF1:  /* extended sequential DCT */
      default:
      {
         return PJPG_UNSUPPORTED_MARKER;
      }
   }
   
   return 0;
}
//------------------------------------------------------------------------------
// Find a start of scan (SOS) marker.
static uint8 locateSOSMarker(PicoJpeg *pjp, uint8* pFoundEOI)
{
   uint8 c;
   uint8 status;

   *pFoundEOI = 0;
      
   status = processMarkers(pjp, &c);
   if (status)
      return status;

   if (c == M_EOI)
   {
      *pFoundEOI = 1;
      return 0;
   }
   else if (c != M_SOS)
      return PJPG_UNEXPECTED_MARKER;

   return readSOSMarker(pjp);
}
//------------------------------------------------------------------------------
// This method throws back into the stream any bytes that where read
// into the bit buffer during initial marker scanning.
static void fixInBuffer(PicoJpeg *pjp)
{
   /* In case any 0xFF's where pulled into the buffer during marker scanning */

   if (pjp->BitsLeft > 0)  
      stuffChar(pjp, (uint8)pjp->BitBuf);
   
   stuffChar(pjp, (uint8)(pjp->BitBuf >> 8));
   
   pjp->BitsLeft = 8;
   getBits2(pjp, 8);
   getBits2(pjp, 8);
}
//------------------------------------------------------------------------------
// Restart interval processing.
static uint8 processRestart(PicoJpeg *pjp)
{
   // Let's scan a little bit to find the marker, but not _too_ far.
   // 1536 is a "fudge factor" that determines how much to scan.
   uint16 i;
   uint8 c = 0;

   for (i = 1536; i > 0; i--)
      if (getChar(pjp) == 0xFF)
         break;

   if (i == 0)
      return PJPG_BAD_RESTART_MARKER;
   
   for ( ; i > 0; i--)
      if ((c = getChar(pjp)) != 0xFF)
         break;

   if (i == 0)
      return PJPG_BAD_RESTART_MARKER;

   // Is it the expected marker? If not, something bad happened.
   if (c != (pjp->NextRestartNum + M_RST0))
      return PJPG_BAD_RESTART_MARKER;

   // Reset each component's DC prediction values.
   pjp->LastDC[0] = 0;
   pjp->LastDC[1] = 0;
   pjp->LastDC[2] = 0;

   pjp->RestartsLeft = pjp->RestartInterval;

   pjp->NextRestartNum = (pjp->NextRestartNum + 1) & 7;

   // Get the bit buffer going again

   pjp->BitsLeft = 8;
   getBits2(pjp, 8);
   getBits2(pjp, 8);
   
   return 0;
}
//------------------------------------------------------------------------------
#if 0
// FIXME: findEOI() is not actually called at the end of the image 
// (it's optional, and probably not needed on embedded devices)
static uint8 findEOI(PicoJpeg *pjp)
{
   uint8 c;
   uint8 status;

   // Prime the bit buffer
   pjp->BitsLeft = 8;
   getBits1(pjp, 8);
   getBits1(pjp, 8);

   // The next marker _should_ be EOI
   status = processMarkers(pjp, &c);
   if (status)
      return status;
   else if (pjp->CallbackStatus)
      return pjp->CallbackStatus;
   
   //gTotalBytesRead -= in_buf_left;
   if (c != M_EOI)
      return PJPG_UNEXPECTED_MARKER;
   
   return 0;
}
#endif
//------------------------------------------------------------------------------
static uint8 checkHuffTables(PicoJpeg *pjp)
{
   uint8 i;

   for (i = 0; i < pjp->CompsInScan; i++)
   {
      uint8 compDCTab = pjp->CompDCTab[pjp->CompList[i]];
      uint8 compACTab = pjp->CompACTab[pjp->CompList[i]] + 2;
      
      if ( ((pjp->ValidHuffTables & (1 << compDCTab)) == 0) ||
           ((pjp->ValidHuffTables & (1 << compACTab)) == 0) )
         return PJPG_UNDEFINED_HUFF_TABLE;           
   }
   
   return 0;
}
//------------------------------------------------------------------------------
static uint8 checkQuantTables(PicoJpeg *pjp)
{
   uint8 i;

   for (i = 0; i < pjp->CompsInScan; i++)
   {
      uint8 compQuantMask = pjp->CompQuant[pjp->CompList[i]] ? 2 : 1;
      
      if ((pjp->ValidQuantTables & compQuantMask) == 0)
         return PJPG_UNDEFINED_QUANT_TABLE;
   }         

   return 0;         
}
//------------------------------------------------------------------------------
static uint8 initScan(PicoJpeg *pjp)
{
   uint8 foundEOI;
   uint8 status = locateSOSMarker(pjp, &foundEOI);
   if (status)
      return status;
   if (foundEOI)
      return PJPG_UNEXPECTED_MARKER;
   
   status = checkHuffTables(pjp);
   if (status)
      return status;

   status = checkQuantTables(pjp);
   if (status)
      return status;

   pjp->LastDC[0] = 0;
   pjp->LastDC[1] = 0;
   pjp->LastDC[2] = 0;

   if (pjp->RestartInterval)
   {
      pjp->RestartsLeft = pjp->RestartInterval;
      pjp->NextRestartNum = 0;
   }

   fixInBuffer(pjp);

   return 0;
}
//------------------------------------------------------------------------------
static uint8 initFrame(PicoJpeg *pjp)
{
   if (pjp->CompsInFrame == 1)
   {
      if ((pjp->CompHSamp[0] != 1) || (pjp->CompVSamp[0] != 1))
         return PJPG_UNSUPPORTED_SAMP_FACTORS;

      pjp->ScanType = PJPG_GRAYSCALE;

      pjp->MaxBlocksPerMCU = 1;
      pjp->MCUOrg[0] = 0;

      pjp->MaxMCUXSize     = 8;
      pjp->MaxMCUYSize     = 8;
   }
   else if (pjp->CompsInFrame == 3)
   {
      if ( ((pjp->CompHSamp[1] != 1) || (pjp->CompVSamp[1] != 1)) ||
         ((pjp->CompHSamp[2] != 1) || (pjp->CompVSamp[2] != 1)) )
         return PJPG_UNSUPPORTED_SAMP_FACTORS;

      if ((pjp->CompHSamp[0] == 1) && (pjp->CompVSamp[0] == 1))
      {
         pjp->ScanType = PJPG_YH1V1;

         pjp->MaxBlocksPerMCU = 3;
         pjp->MCUOrg[0] = 0;
         pjp->MCUOrg[1] = 1;
         pjp->MCUOrg[2] = 2;
                  
         pjp->MaxMCUXSize = 8;
         pjp->MaxMCUYSize = 8;
      }
      else if ((pjp->CompHSamp[0] == 1) && (pjp->CompVSamp[0] == 2))
      {
         pjp->ScanType = PJPG_YH1V2;

         pjp->MaxBlocksPerMCU = 4;
         pjp->MCUOrg[0] = 0;
         pjp->MCUOrg[1] = 0;
         pjp->MCUOrg[2] = 1;
         pjp->MCUOrg[3] = 2;

         pjp->MaxMCUXSize = 8;
         pjp->MaxMCUYSize = 16;
      }
      else if ((pjp->CompHSamp[0] == 2) && (pjp->CompVSamp[0] == 1))
      {
         pjp->ScanType = PJPG_YH2V1;

         pjp->MaxBlocksPerMCU = 4;
         pjp->MCUOrg[0] = 0;
         pjp->MCUOrg[1] = 0;
         pjp->MCUOrg[2] = 1;
         pjp->MCUOrg[3] = 2;

         pjp->MaxMCUXSize = 16;
         pjp->MaxMCUYSize = 8;
      }
      else if ((pjp->CompHSamp[0] == 2) && (pjp->CompVSamp[0] == 2))
      {
         pjp->ScanType = PJPG_YH2V2;

         pjp->MaxBlocksPerMCU = 6;
         pjp->MCUOrg[0] = 0;
         pjp->MCUOrg[1] = 0;
         pjp->MCUOrg[2] = 0;
         pjp->MCUOrg[3] = 0;
         pjp->MCUOrg[4] = 1;
         pjp->MCUOrg[5] = 2;

         pjp->MaxMCUXSize = 16;
         pjp->MaxMCUYSize = 16;
      }
      else
         return PJPG_UNSUPPORTED_SAMP_FACTORS;
   }
   else
      return PJPG_UNSUPPORTED_COLORSPACE;

   pjp->MaxMCUSPerRow = (pjp->ImageXSize + (pjp->MaxMCUXSize - 1)) >> ((pjp->MaxMCUXSize == 8) ? 3 : 4);
   pjp->MaxMCUSPerCol = (pjp->ImageYSize + (pjp->MaxMCUYSize - 1)) >> ((pjp->MaxMCUYSize == 8) ? 3 : 4);
   
   pjp->NumMCUSRemaining = pjp->MaxMCUSPerRow * pjp->MaxMCUSPerCol;
   
   return 0;
}
//----------------------------------------------------------------------------
// Winograd IDCT: 5 multiplies per row/col, up to 80 muls for the 2D IDCT

#define PJPG_DCT_SCALE_BITS 7

#define PJPG_DCT_SCALE (1U << PJPG_DCT_SCALE_BITS)

#define PJPG_DESCALE(x) PJPG_ARITH_SHIFT_RIGHT_N_16(((x) + (1U << (PJPG_DCT_SCALE_BITS - 1))), PJPG_DCT_SCALE_BITS)

#define PJPG_WFIX(x) ((x) * PJPG_DCT_SCALE + 0.5f)

#define PJPG_WINOGRAD_QUANT_SCALE_BITS 10

const uint8 gWinogradQuant[] = 
{
   128,  178,  178,  167,  246,  167,  151,  232,
   232,  151,  128,  209,  219,  209,  128,  101,
   178,  197,  197,  178,  101,   69,  139,  167,
   177,  167,  139,   69,   35,   96,  131,  151,
   151,  131,   96,   35,   49,   91,  118,  128,
   118,   91,   49,   46,   81,  101,  101,   81,
   46,   42,   69,   79,   69,   42,   35,   54,
   54,   35,   28,   37,   28,   19,   19,   10,
};   

// Multiply quantization matrix by the Winograd IDCT scale factors
static void createWinogradQuant(int16* pQuant)
{
   uint8 i;
   
   for (i = 0; i < 64; i++) 
   {
      long x = pQuant[i];
      x *= gWinogradQuant[i];
      pQuant[i] = (int16)((x + (1 << (PJPG_WINOGRAD_QUANT_SCALE_BITS - PJPG_DCT_SCALE_BITS - 1))) >> (PJPG_WINOGRAD_QUANT_SCALE_BITS - PJPG_DCT_SCALE_BITS));
   }
}

// These multiply helper functions are the 4 types of signed multiplies needed by the Winograd IDCT.
// A smart C compiler will optimize them to use 16x8 = 24 bit muls, if not you may need to tweak
// these functions or drop to CPU specific inline assembly.

// 1/cos(4*pi/16)
// 362, 256+106
static PJPG_INLINE int16 imul_b1_b3(int16 w)
{
   long x = (w * 362L);
   x += 128L;
   return (int16)(PJPG_ARITH_SHIFT_RIGHT_8_L(x));
}

// 1/cos(6*pi/16)
// 669, 256+256+157
static PJPG_INLINE int16 imul_b2(int16 w)
{
   long x = (w * 669L);
   x += 128L;
   return (int16)(PJPG_ARITH_SHIFT_RIGHT_8_L(x));
}

// 1/cos(2*pi/16)
// 277, 256+21
static PJPG_INLINE int16 imul_b4(int16 w)
{
   long x = (w * 277L);
   x += 128L;
   return (int16)(PJPG_ARITH_SHIFT_RIGHT_8_L(x));
}

// 1/(cos(2*pi/16) + cos(6*pi/16))
// 196, 196
static PJPG_INLINE int16 imul_b5(int16 w)
{
   long x = (w * 196L);
   x += 128L;
   return (int16)(PJPG_ARITH_SHIFT_RIGHT_8_L(x));
}

static PJPG_INLINE uint8 clamp(int16 s)
{
   if ((uint16)s > 255U)
   {
      if (s < 0) 
         return 0; 
      else if (s > 255) 
         return 255;
   }
      
   return (uint8)s;
}

static void idctRows(int16* pSrc)
{
   uint8 i;
            
   for (i = 0; i < 8; i++)
   {
      if ((pSrc[1] | pSrc[2] | pSrc[3] | pSrc[4] | pSrc[5] | pSrc[6] | pSrc[7]) == 0)
      {
         // Short circuit the 1D IDCT if only the DC component is non-zero
         int16 src0 = *pSrc;

         *(pSrc+1) = src0;
         *(pSrc+2) = src0;
         *(pSrc+3) = src0;
         *(pSrc+4) = src0;
         *(pSrc+5) = src0;
         *(pSrc+6) = src0;
         *(pSrc+7) = src0;
      }
      else
      {
         int16 src4 = *(pSrc+5);
         int16 src7 = *(pSrc+3);
         int16 x4  = src4 - src7;
         int16 x7  = src4 + src7;

         int16 src5 = *(pSrc+1);
         int16 src6 = *(pSrc+7);
         int16 x5  = src5 + src6;
         int16 x6  = src5 - src6;

         int16 tmp1 = imul_b5(x4 - x6);
         int16 stg26 = imul_b4(x6) - tmp1;

         int16 x24 = tmp1 - imul_b2(x4);

         int16 x15 = x5 - x7;
         int16 x17 = x5 + x7;

         int16 tmp2 = stg26 - x17;
         int16 tmp3 = imul_b1_b3(x15) - tmp2;
         int16 x44 = tmp3 + x24;

         int16 src0 = *(pSrc+0);
         int16 src1 = *(pSrc+4);
         int16 x30 = src0 + src1;
         int16 x31 = src0 - src1;

         int16 src2 = *(pSrc+2);
         int16 src3 = *(pSrc+6);
         int16 x12 = src2 - src3;
         int16 x13 = src2 + src3;

         int16 x32 = imul_b1_b3(x12) - x13;

         int16 x40 = x30 + x13;
         int16 x43 = x30 - x13;
         int16 x41 = x31 + x32;
         int16 x42 = x31 - x32;

         *(pSrc+0) = x40 + x17;
         *(pSrc+1) = x41 + tmp2;
         *(pSrc+2) = x42 + tmp3;
         *(pSrc+3) = x43 - x44;
         *(pSrc+4) = x43 + x44;
         *(pSrc+5) = x42 - tmp3;
         *(pSrc+6) = x41 - tmp2;
         *(pSrc+7) = x40 - x17;
      }
                  
      pSrc += 8;
   }      
}

static void idctCols(int16* pSrc)
{
   uint8 i;

   for (i = 0; i < 8; i++)
   {
      if ((pSrc[1*8] | pSrc[2*8] | pSrc[3*8] | pSrc[4*8] | pSrc[5*8] | pSrc[6*8] | pSrc[7*8]) == 0)
      {
         // Short circuit the 1D IDCT if only the DC component is non-zero
         uint8 c = clamp(PJPG_DESCALE(*pSrc) + 128);
         *(pSrc+0*8) = c;
         *(pSrc+1*8) = c;
         *(pSrc+2*8) = c;
         *(pSrc+3*8) = c;
         *(pSrc+4*8) = c;
         *(pSrc+5*8) = c;
         *(pSrc+6*8) = c;
         *(pSrc+7*8) = c;
      }
      else
      {
         int16 src4 = *(pSrc+5*8);
         int16 src7 = *(pSrc+3*8);
         int16 x4  = src4 - src7;
         int16 x7  = src4 + src7;

         int16 src5 = *(pSrc+1*8);
         int16 src6 = *(pSrc+7*8);
         int16 x5  = src5 + src6;
         int16 x6  = src5 - src6;

         int16 tmp1 = imul_b5(x4 - x6);
         int16 stg26 = imul_b4(x6) - tmp1;

         int16 x24 = tmp1 - imul_b2(x4);

         int16 x15 = x5 - x7;
         int16 x17 = x5 + x7;

         int16 tmp2 = stg26 - x17;
         int16 tmp3 = imul_b1_b3(x15) - tmp2;
         int16 x44 = tmp3 + x24;

         int16 src0 = *(pSrc+0*8);
         int16 src1 = *(pSrc+4*8);
         int16 x30 = src0 + src1;
         int16 x31 = src0 - src1;

         int16 src2 = *(pSrc+2*8);
         int16 src3 = *(pSrc+6*8);
         int16 x12 = src2 - src3;
         int16 x13 = src2 + src3;

         int16 x32 = imul_b1_b3(x12) - x13;

         int16 x40 = x30 + x13;
         int16 x43 = x30 - x13;
         int16 x41 = x31 + x32;
         int16 x42 = x31 - x32;

         // descale, convert to unsigned and clamp to 8-bit
         *(pSrc+0*8) = clamp(PJPG_DESCALE(x40 + x17)  + 128);
         *(pSrc+1*8) = clamp(PJPG_DESCALE(x41 + tmp2) + 128);
         *(pSrc+2*8) = clamp(PJPG_DESCALE(x42 + tmp3) + 128);
         *(pSrc+3*8) = clamp(PJPG_DESCALE(x43 - x44)  + 128);
         *(pSrc+4*8) = clamp(PJPG_DESCALE(x43 + x44)  + 128);
         *(pSrc+5*8) = clamp(PJPG_DESCALE(x42 - tmp3) + 128);
         *(pSrc+6*8) = clamp(PJPG_DESCALE(x41 - tmp2) + 128);
         *(pSrc+7*8) = clamp(PJPG_DESCALE(x40 - x17)  + 128);
      }

      pSrc++;      
   }      
}

/*----------------------------------------------------------------------------*/
static PJPG_INLINE uint8 addAndClamp(uint8 a, int16 b)
{
   b = a + b;
   
   if ((uint16)b > 255U)
   {
      if (b < 0)
         return 0;
      else if (b > 255)
         return 255;
   }
      
   return (uint8)b;
}
/*----------------------------------------------------------------------------*/
static PJPG_INLINE uint8 subAndClamp(uint8 a, int16 b)
{
   b = a - b;

   if ((uint16)b > 255U)
   {
      if (b < 0)
         return 0;
      else if (b > 255)
         return 255;
   }

   return (uint8)b;
}
/*----------------------------------------------------------------------------*/
// 103/256
//R = Y + 1.402 (Cr-128)

// 88/256, 183/256
//G = Y - 0.34414 (Cb-128) - 0.71414 (Cr-128)

// 198/256
//B = Y + 1.772 (Cb-128)
/*----------------------------------------------------------------------------*/
// Cb upsample and accumulate, 4x4 to 8x8
static void upsampleCb(PicoJpeg *pjp, uint8 srcOfs, uint8 dstOfs)
{
   // Cb - affects G and B
   uint8 x, y;
   int16* pSrc = pjp->CoeffBuf+ srcOfs;
   uint8* pDstG = pjp->MCUBufG + dstOfs;
   uint8* pDstB = pjp->MCUBufB + dstOfs;
   for (y = 0; y < 4; y++)
   {
      for (x = 0; x < 4; x++)
      {
         uint8 cb = (uint8)*pSrc++;
         int16 cbG, cbB;

         cbG = ((cb * 88U) >> 8U) - 44U;
         pDstG[0] = subAndClamp(pDstG[0], cbG);
         pDstG[1] = subAndClamp(pDstG[1], cbG);
         pDstG[8] = subAndClamp(pDstG[8], cbG);
         pDstG[9] = subAndClamp(pDstG[9], cbG);

         cbB = (cb + ((cb * 198U) >> 8U)) - 227U;
         pDstB[0] = addAndClamp(pDstB[0], cbB);
         pDstB[1] = addAndClamp(pDstB[1], cbB);
         pDstB[8] = addAndClamp(pDstB[8], cbB);
         pDstB[9] = addAndClamp(pDstB[9], cbB);

         pDstG += 2;
         pDstB += 2;
      }

      pSrc = pSrc - 4 + 8;
      pDstG = pDstG - 8 + 16;
      pDstB = pDstB - 8 + 16;
   }
}   
/*----------------------------------------------------------------------------*/
// Cb upsample and accumulate, 4x8 to 8x8
static void upsampleCbH(PicoJpeg *pjp, uint8 srcOfs, uint8 dstOfs)
{
   // Cb - affects G and B
   uint8 x, y;
   int16* pSrc = pjp->CoeffBuf+ srcOfs;
   uint8* pDstG = pjp->MCUBufG + dstOfs;
   uint8* pDstB = pjp->MCUBufB + dstOfs;
   for (y = 0; y < 8; y++)
   {
      for (x = 0; x < 4; x++)
      {
         uint8 cb = (uint8)*pSrc++;
         int16 cbG, cbB;

         cbG = ((cb * 88U) >> 8U) - 44U;
         pDstG[0] = subAndClamp(pDstG[0], cbG);
         pDstG[1] = subAndClamp(pDstG[1], cbG);

         cbB = (cb + ((cb * 198U) >> 8U)) - 227U;
         pDstB[0] = addAndClamp(pDstB[0], cbB);
         pDstB[1] = addAndClamp(pDstB[1], cbB);

         pDstG += 2;
         pDstB += 2;
      }

      pSrc = pSrc - 4 + 8;
   }
}   
/*----------------------------------------------------------------------------*/
// Cb upsample and accumulate, 8x4 to 8x8
static void upsampleCbV(PicoJpeg *pjp, uint8 srcOfs, uint8 dstOfs)
{
   // Cb - affects G and B
   uint8 x, y;
   int16* pSrc = pjp->CoeffBuf+ srcOfs;
   uint8* pDstG = pjp->MCUBufG + dstOfs;
   uint8* pDstB = pjp->MCUBufB + dstOfs;
   for (y = 0; y < 4; y++)
   {
      for (x = 0; x < 8; x++)
      {
         uint8 cb = (uint8)*pSrc++;
         int16 cbG, cbB;

         cbG = ((cb * 88U) >> 8U) - 44U;
         pDstG[0] = subAndClamp(pDstG[0], cbG);
         pDstG[8] = subAndClamp(pDstG[8], cbG);

         cbB = (cb + ((cb * 198U) >> 8U)) - 227U;
         pDstB[0] = addAndClamp(pDstB[0], cbB);
         pDstB[8] = addAndClamp(pDstB[8], cbB);

         ++pDstG;
         ++pDstB;
      }

      pDstG = pDstG - 8 + 16;
      pDstB = pDstB - 8 + 16;
   }
}   
/*----------------------------------------------------------------------------*/
// 103/256
//R = Y + 1.402 (Cr-128)

// 88/256, 183/256
//G = Y - 0.34414 (Cb-128) - 0.71414 (Cr-128)

// 198/256
//B = Y + 1.772 (Cb-128)
/*----------------------------------------------------------------------------*/
// Cr upsample and accumulate, 4x4 to 8x8
static void upsampleCr(PicoJpeg *pjp, uint8 srcOfs, uint8 dstOfs)
{
   // Cr - affects R and G
   uint8 x, y;
   int16* pSrc = pjp->CoeffBuf+ srcOfs;
   uint8* pDstR = pjp->MCUBufR+ dstOfs;
   uint8* pDstG = pjp->MCUBufG + dstOfs;
   for (y = 0; y < 4; y++)
   {
      for (x = 0; x < 4; x++)
      {
         uint8 cr = (uint8)*pSrc++;
         int16 crR, crG;

         crR = (cr + ((cr * 103U) >> 8U)) - 179;
         pDstR[0] = addAndClamp(pDstR[0], crR);
         pDstR[1] = addAndClamp(pDstR[1], crR);
         pDstR[8] = addAndClamp(pDstR[8], crR);
         pDstR[9] = addAndClamp(pDstR[9], crR);
         
         crG = ((cr * 183U) >> 8U) - 91;
         pDstG[0] = subAndClamp(pDstG[0], crG);
         pDstG[1] = subAndClamp(pDstG[1], crG);
         pDstG[8] = subAndClamp(pDstG[8], crG);
         pDstG[9] = subAndClamp(pDstG[9], crG);
         
         pDstR += 2;
         pDstG += 2;
      }

      pSrc = pSrc - 4 + 8;
      pDstR = pDstR - 8 + 16;
      pDstG = pDstG - 8 + 16;
   }
}   
/*----------------------------------------------------------------------------*/
// Cr upsample and accumulate, 4x8 to 8x8
static void upsampleCrH(PicoJpeg *pjp, uint8 srcOfs, uint8 dstOfs)
{
   // Cr - affects R and G
   uint8 x, y;
   int16* pSrc = pjp->CoeffBuf+ srcOfs;
   uint8* pDstR = pjp->MCUBufR+ dstOfs;
   uint8* pDstG = pjp->MCUBufG + dstOfs;
   for (y = 0; y < 8; y++)
   {
      for (x = 0; x < 4; x++)
      {
         uint8 cr = (uint8)*pSrc++;
         int16 crR, crG;

         crR = (cr + ((cr * 103U) >> 8U)) - 179;
         pDstR[0] = addAndClamp(pDstR[0], crR);
         pDstR[1] = addAndClamp(pDstR[1], crR);
         
         crG = ((cr * 183U) >> 8U) - 91;
         pDstG[0] = subAndClamp(pDstG[0], crG);
         pDstG[1] = subAndClamp(pDstG[1], crG);
         
         pDstR += 2;
         pDstG += 2;
      }

      pSrc = pSrc - 4 + 8;
   }
}   
/*----------------------------------------------------------------------------*/
// Cr upsample and accumulate, 8x4 to 8x8
static void upsampleCrV(PicoJpeg *pjp, uint8 srcOfs, uint8 dstOfs)
{
   // Cr - affects R and G
   uint8 x, y;
   int16* pSrc = pjp->CoeffBuf+ srcOfs;
   uint8* pDstR = pjp->MCUBufR+ dstOfs;
   uint8* pDstG = pjp->MCUBufG + dstOfs;
   for (y = 0; y < 4; y++)
   {
      for (x = 0; x < 8; x++)
      {
         uint8 cr = (uint8)*pSrc++;
         int16 crR, crG;

         crR = (cr + ((cr * 103U) >> 8U)) - 179;
         pDstR[0] = addAndClamp(pDstR[0], crR);
         pDstR[8] = addAndClamp(pDstR[8], crR);

         crG = ((cr * 183U) >> 8U) - 91;
         pDstG[0] = subAndClamp(pDstG[0], crG);
         pDstG[8] = subAndClamp(pDstG[8], crG);

         ++pDstR;
         ++pDstG;
      }

      pDstR = pDstR - 8 + 16;
      pDstG = pDstG - 8 + 16;
   }
} 
/*----------------------------------------------------------------------------*/
// Convert Y to RGB
static void copyY(PicoJpeg *pjp, uint8 dstOfs)
{
   uint8 i;
   uint8* pRDst = pjp->MCUBufR+ dstOfs;
   uint8* pGDst = pjp->MCUBufG + dstOfs;
   uint8* pBDst = pjp->MCUBufB + dstOfs;
   int16* pSrc = pjp->CoeffBuf;
   
   for (i = 64; i > 0; i--)
   {
      uint8 c = (uint8)*pSrc++;
      
      *pRDst++ = c;
      *pGDst++ = c;
      *pBDst++ = c;
   }
}
/*----------------------------------------------------------------------------*/
// Cb convert to RGB and accumulate
static void convertCb(PicoJpeg *pjp, uint8 dstOfs)
{
   uint8 i;
   uint8* pDstG = pjp->MCUBufG + dstOfs;
   uint8* pDstB = pjp->MCUBufB + dstOfs;
   int16* pSrc = pjp->CoeffBuf;

   for (i = 64; i > 0; i--)
   {
      uint8 cb = (uint8)*pSrc++;
      int16 cbG, cbB;

      cbG = ((cb * 88U) >> 8U) - 44U;
      *pDstG = subAndClamp(*pDstG, cbG);
      pDstG++;

      cbB = (cb + ((cb * 198U) >> 8U)) - 227U;
      *pDstB = addAndClamp(*pDstB, cbB);
      pDstB++;
   }
}
/*----------------------------------------------------------------------------*/
// Cr convert to RGB and accumulate
static void convertCr(PicoJpeg *pjp, uint8 dstOfs)
{
   uint8 i;
   uint8* pDstR = pjp->MCUBufR+ dstOfs;
   uint8* pDstG = pjp->MCUBufG + dstOfs;
   int16* pSrc = pjp->CoeffBuf;

   for (i = 64; i > 0; i--)
   {
      uint8 cr = (uint8)*pSrc++;
      int16 crR, crG;

      crR = (cr + ((cr * 103U) >> 8U)) - 179;
      *pDstR = addAndClamp(*pDstR, crR);
      pDstR++;

      crG = ((cr * 183U) >> 8U) - 91;
      *pDstG = subAndClamp(*pDstG, crG);
      pDstG++;
   }
}
/*----------------------------------------------------------------------------*/
static void transformBlock(PicoJpeg *pjp, uint8 mcuBlock)
{
   idctRows(pjp->CoeffBuf);
   idctCols(pjp->CoeffBuf);
   
   switch (pjp->ScanType)
   {
      case PJPG_GRAYSCALE:
      {
         // MCU size: 1, 1 block per MCU
         copyY(pjp, 0);
         break;
      }
      case PJPG_YH1V1:
      {
         // MCU size: 8x8, 3 blocks per MCU
         switch (mcuBlock)
         {
            case 0:
            {
               copyY(pjp, 0);
               break;
            }
            case 1:
            {
               convertCb(pjp, 0);
               break;
            }
            case 2:
            {
               convertCr(pjp, 0);
               break;
            }
         }

         break;
      }
      case PJPG_YH1V2:
      {
         // MCU size: 8x16, 4 blocks per MCU
         switch (mcuBlock)
         {
            case 0:
            {
               copyY(pjp, 0);
               break;
            }
            case 1:
            {
               copyY(pjp, 128);
               break;
            }
            case 2:
            {
               upsampleCbV(pjp, 0, 0);
               upsampleCbV(pjp, 4*8, 128);
               break;
            }
            case 3:
            {
               upsampleCrV(pjp, 0, 0);
               upsampleCrV(pjp, 4*8, 128);
               break;
            }
         }

         break;
      }        
      case PJPG_YH2V1:
      {
         // MCU size: 16x8, 4 blocks per MCU
         switch (mcuBlock)
         {
            case 0:
            {
               copyY(pjp, 0);
               break;
            }
            case 1:
            {
               copyY(pjp, 64);
               break;
            }
            case 2:
            {
               upsampleCbH(pjp, 0, 0);
               upsampleCbH(pjp, 4, 64);
               break;
            }
            case 3:
            {
               upsampleCrH(pjp, 0, 0);
               upsampleCrH(pjp, 4, 64);
               break;
            }
         }
         
         break;
      }        
      case PJPG_YH2V2:
      {
         // MCU size: 16x16, 6 blocks per MCU
         switch (mcuBlock)
         {
            case 0:
            {
               copyY(pjp, 0);
               break;
            }
            case 1:
            {
               copyY(pjp, 64);
               break;
            }
            case 2:
            {
               copyY(pjp, 128);
               break;
            }
            case 3:
            {
               copyY(pjp, 192);
               break;
            }
            case 4:
            {
               upsampleCb(pjp, 0, 0);
               upsampleCb(pjp, 4, 64);
               upsampleCb(pjp, 4*8, 128);
               upsampleCb(pjp, 4+4*8, 192);
               break;
            }
            case 5:
            {
               upsampleCr(pjp, 0, 0);
               upsampleCr(pjp, 4, 64);
               upsampleCr(pjp, 4*8, 128);
               upsampleCr(pjp, 4+4*8, 192);
               break;
            }
         }

         break;
      }         
   }      
}
//------------------------------------------------------------------------------
static void transformBlockReduce(PicoJpeg *pjp, uint8 mcuBlock)
{
   uint8 c = clamp(PJPG_DESCALE(pjp->CoeffBuf[0]) + 128);
   int16 cbG, cbB, crR, crG;

   switch (pjp->ScanType)
   {
      case PJPG_GRAYSCALE:
      {
         // MCU size: 1, 1 block per MCU
         pjp->MCUBufR[0] = c;
         break;
      }
      case PJPG_YH1V1:
      {
         // MCU size: 8x8, 3 blocks per MCU
         switch (mcuBlock)
         {
            case 0:
            {
               pjp->MCUBufR[0] = c;
               pjp->MCUBufG[0] = c;
               pjp->MCUBufB[0] = c;
               break;
            }
            case 1:
            {
               cbG = ((c * 88U) >> 8U) - 44U;
               pjp->MCUBufG[0] = subAndClamp(pjp->MCUBufG[0], cbG);

               cbB = (c + ((c * 198U) >> 8U)) - 227U;
               pjp->MCUBufB[0] = addAndClamp(pjp->MCUBufB[0], cbB);
               break;
            }
            case 2:
            {
               crR = (c + ((c * 103U) >> 8U)) - 179;
               pjp->MCUBufR[0] = addAndClamp(pjp->MCUBufR[0], crR);

               crG = ((c * 183U) >> 8U) - 91;
               pjp->MCUBufG[0] = subAndClamp(pjp->MCUBufG[0], crG);
               break;
            }
         }

         break;
      }
      case PJPG_YH1V2:
      {
         // MCU size: 8x16, 4 blocks per MCU
         switch (mcuBlock)
         {
            case 0:
            {
               pjp->MCUBufR[0] = c;
               pjp->MCUBufG[0] = c;
               pjp->MCUBufB[0] = c;
               break;
            }
            case 1:
            {
               pjp->MCUBufR[128] = c;
               pjp->MCUBufG[128] = c;
               pjp->MCUBufB[128] = c;
               break;
            }
            case 2:
            {
               cbG = ((c * 88U) >> 8U) - 44U;
               pjp->MCUBufG[0] = subAndClamp(pjp->MCUBufG[0], cbG);
               pjp->MCUBufG[128] = subAndClamp(pjp->MCUBufG[128], cbG);

               cbB = (c + ((c * 198U) >> 8U)) - 227U;
               pjp->MCUBufB[0] = addAndClamp(pjp->MCUBufB[0], cbB);
               pjp->MCUBufB[128] = addAndClamp(pjp->MCUBufB[128], cbB);

               break;
            }
            case 3:
            {
               crR = (c + ((c * 103U) >> 8U)) - 179;
               pjp->MCUBufR[0] = addAndClamp(pjp->MCUBufR[0], crR);
               pjp->MCUBufR[128] = addAndClamp(pjp->MCUBufR[128], crR);

               crG = ((c * 183U) >> 8U) - 91;
               pjp->MCUBufG[0] = subAndClamp(pjp->MCUBufG[0], crG);
               pjp->MCUBufG[128] = subAndClamp(pjp->MCUBufG[128], crG);

               break;
            }
         }
         break;
      }
      case PJPG_YH2V1:
      {
         // MCU size: 16x8, 4 blocks per MCU
         switch (mcuBlock)
         {
            case 0:
            {
               pjp->MCUBufR[0] = c;
               pjp->MCUBufG[0] = c;
               pjp->MCUBufB[0] = c;
               break;
            }
            case 1:
            {
               pjp->MCUBufR[64] = c;
               pjp->MCUBufG[64] = c;
               pjp->MCUBufB[64] = c;
               break;
            }
            case 2:
            {
               cbG = ((c * 88U) >> 8U) - 44U;
               pjp->MCUBufG[0] = subAndClamp(pjp->MCUBufG[0], cbG);
               pjp->MCUBufG[64] = subAndClamp(pjp->MCUBufG[64], cbG);

               cbB = (c + ((c * 198U) >> 8U)) - 227U;
               pjp->MCUBufB[0] = addAndClamp(pjp->MCUBufB[0], cbB);
               pjp->MCUBufB[64] = addAndClamp(pjp->MCUBufB[64], cbB);

               break;
            }
            case 3:
            {
               crR = (c + ((c * 103U) >> 8U)) - 179;
               pjp->MCUBufR[0] = addAndClamp(pjp->MCUBufR[0], crR);
               pjp->MCUBufR[64] = addAndClamp(pjp->MCUBufR[64], crR);

               crG = ((c * 183U) >> 8U) - 91;
               pjp->MCUBufG[0] = subAndClamp(pjp->MCUBufG[0], crG);
               pjp->MCUBufG[64] = subAndClamp(pjp->MCUBufG[64], crG);

               break;
            }
         }
         break;
      }
      case PJPG_YH2V2:
      {
         // MCU size: 16x16, 6 blocks per MCU
         switch (mcuBlock)
         {
            case 0:
            {
               pjp->MCUBufR[0] = c;
               pjp->MCUBufG[0] = c;
               pjp->MCUBufB[0] = c;
               break;
            }
            case 1:
            {
               pjp->MCUBufR[64] = c;
               pjp->MCUBufG[64] = c;
               pjp->MCUBufB[64] = c;
               break;
            }
            case 2:
            {
               pjp->MCUBufR[128] = c;
               pjp->MCUBufG[128] = c;
               pjp->MCUBufB[128] = c;
               break;
            }
            case 3:
            {
               pjp->MCUBufR[192] = c;
               pjp->MCUBufG[192] = c;
               pjp->MCUBufB[192] = c;
               break;
            }
            case 4:
            {
               cbG = ((c * 88U) >> 8U) - 44U;
               pjp->MCUBufG[0] = subAndClamp(pjp->MCUBufG[0], cbG);
               pjp->MCUBufG[64] = subAndClamp(pjp->MCUBufG[64], cbG);
               pjp->MCUBufG[128] = subAndClamp(pjp->MCUBufG[128], cbG);
               pjp->MCUBufG[192] = subAndClamp(pjp->MCUBufG[192], cbG);

               cbB = (c + ((c * 198U) >> 8U)) - 227U;
               pjp->MCUBufB[0] = addAndClamp(pjp->MCUBufB[0], cbB);
               pjp->MCUBufB[64] = addAndClamp(pjp->MCUBufB[64], cbB);
               pjp->MCUBufB[128] = addAndClamp(pjp->MCUBufB[128], cbB);
               pjp->MCUBufB[192] = addAndClamp(pjp->MCUBufB[192], cbB);

               break;
            }
            case 5:
            {
               crR = (c + ((c * 103U) >> 8U)) - 179;
               pjp->MCUBufR[0] = addAndClamp(pjp->MCUBufR[0], crR);
               pjp->MCUBufR[64] = addAndClamp(pjp->MCUBufR[64], crR);
               pjp->MCUBufR[128] = addAndClamp(pjp->MCUBufR[128], crR);
               pjp->MCUBufR[192] = addAndClamp(pjp->MCUBufR[192], crR);

               crG = ((c * 183U) >> 8U) - 91;
               pjp->MCUBufG[0] = subAndClamp(pjp->MCUBufG[0], crG);
               pjp->MCUBufG[64] = subAndClamp(pjp->MCUBufG[64], crG);
               pjp->MCUBufG[128] = subAndClamp(pjp->MCUBufG[128], crG);
               pjp->MCUBufG[192] = subAndClamp(pjp->MCUBufG[192], crG);

               break;
            }
         }
         break;
      }
   }
}
//------------------------------------------------------------------------------
static uint8 decodeNextMCU(PicoJpeg *pjp)
{
   uint8 status;
   uint8 mcuBlock;   

   if (pjp->RestartInterval) 
   {
      if (pjp->RestartsLeft == 0)
      {
         status = processRestart(pjp);
         if (status)
            return status;
      }
      pjp->RestartsLeft--;
   }      
   
   for (mcuBlock = 0; mcuBlock < pjp->MaxBlocksPerMCU; mcuBlock++)
   {
      uint8 componentID = pjp->MCUOrg[mcuBlock];
      uint8 compQuant = pjp->CompQuant[componentID];	
      uint8 compDCTab = pjp->CompDCTab[componentID];
      uint8 numExtraBits, compACTab, k;
      const int16* pQ = compQuant ? pjp->Quant1 : pjp->Quant0;
      uint16 r, dc;

      uint8 s = huffDecode(pjp, compDCTab ? &pjp->HuffTab1 : &pjp->HuffTab0,
              compDCTab ? pjp->HuffVal1 : pjp->HuffVal0);
      
      r = 0;
      numExtraBits = s & 0xF;
      if (numExtraBits)
         r = getBits2(pjp, numExtraBits);
      dc = huffExtend(r, s);
            
      dc = dc + pjp->LastDC[componentID];
      pjp->LastDC[componentID] = dc;
            
      pjp->CoeffBuf[0] = dc * pQ[0];

      compACTab = pjp->CompACTab[componentID];

      if (pjp->output_type == PJPG_REDUCED_GRAY8 ||
          pjp->output_type == PJPG_REDUCED_RGB888 ||
          pjp->output_type == PJPG_REDUCED_RGB565)
      {
         // Decode, but throw out the AC coefficients in reduce mode.
         for (k = 1; k < 64; k++)
         {
            s = huffDecode(pjp, compACTab ? &pjp->HuffTab3 : &pjp->HuffTab2,
                    compACTab ? pjp->HuffVal3 : pjp->HuffVal2);

            numExtraBits = s & 0xF;
            if (numExtraBits)
               getBits2(pjp, numExtraBits);

            r = s >> 4;
            s &= 15;

            if (s)
            {
               if (r)
               {
                  if ((k + r) > 63)
                     return PJPG_DECODE_ERROR;

                  k = (uint8)(k + r);
               }
            }
            else
            {
               if (r == 15)
               {
                  if ((k + 16) > 64)
                     return PJPG_DECODE_ERROR;

                  k += (16 - 1); // - 1 because the loop counter is k
               }
               else
                  break;
            }
         }

         transformBlockReduce(pjp, mcuBlock); 
      }
      else
      {
         // Decode and dequantize AC coefficients
         for (k = 1; k < 64; k++)
         {
            uint16 extraBits;

            s = huffDecode(pjp, compACTab ? &pjp->HuffTab3 : &pjp->HuffTab2,
                    compACTab ? pjp->HuffVal3 : pjp->HuffVal2);

            extraBits = 0;
            numExtraBits = s & 0xF;
            if (numExtraBits)
               extraBits = getBits2(pjp, numExtraBits);

            r = s >> 4;
            s &= 15;

            if (s)
            {
               int16 ac;

               if (r)
               {
                  if ((k + r) > 63)
                     return PJPG_DECODE_ERROR;

                  while (r)
                  {
                     pjp->CoeffBuf[ZAG[k++]] = 0;
                     r--;
                  }
               }

               ac = huffExtend(extraBits, s);
               
               pjp->CoeffBuf[ZAG[k]] = ac * pQ[k]; 
            }
            else
            {
               if (r == 15)
               {
                  if ((k + 16) > 64)
                     return PJPG_DECODE_ERROR;
                  
                  for (r = 16; r > 0; r--)
                     pjp->CoeffBuf[ZAG[k++]] = 0;
                  
                  k--; // - 1 because the loop counter is k
               }
               else
                  break;
            }
         }
         
         while (k < 64)
            pjp->CoeffBuf[ZAG[k++]] = 0;

         transformBlock(pjp, mcuBlock); 
      }
   }
   return 0;
}
//------------------------------------------------------------------------------
unsigned char pjpeg_decode_mcu(pjpeg_image_info_t *pInfo)
{
   uint8 status;
   PicoJpeg *pjp = pInfo->m_PJHandle;
   if (pjp->CallbackStatus)
      return pjp->CallbackStatus;
   
   if (!pjp->NumMCUSRemaining)
      return PJPG_NO_MORE_BLOCKS;
      
   status = decodeNextMCU(pjp);
   if ((status) || (pjp->CallbackStatus))
      return pjp->CallbackStatus ? pjp->CallbackStatus : status;
      
   pjp->NumMCUSRemaining--;
   
   return 0;
}

//------------------------------------------------------------------------------
unsigned char pjpeg_decode_init(pjpeg_image_info_t *pInfo, jsread_t *jsread, void *pCallback_data, void *storage, pjpeg_output_type_t output_type)
{
   uint8 status;
   uint8 *p;
   uint8 *e;
   PicoJpeg *pjp;
   if (storage == 0) {
       return PJPG_NOTENOUGHMEM;
   }
   pjp = storage;
   /* Clear storage block.  Could use memset, but we are avoiding using libraries. */
   for (p = (uint8 *)pjp, e = p + sizeof(PicoJpeg); p < e; p++) {
      *p = 0;
   }
   /* Clear Info block.  Could use memset, but we are avoiding using libraries. */
   for (p = (uint8 *)pInfo, e = p + sizeof(pjpeg_image_info_t); p < e; p++) {
      *p = 0;
   }
   pInfo->m_scanType = PJPG_GRAYSCALE;

   pjp->Jsread = jsread;
   pjp->pCallback_data = pCallback_data;
   pjp->CallbackStatus = 0;
   pjp->output_type = output_type;
    
   pjp->BitsLeft = 8;
   getBits1(pjp, 8);
   getBits1(pjp, 8);
   status = locateSOFMarker(pjp);
   if ((status) || (pjp->CallbackStatus))
      return pjp->CallbackStatus ? pjp->CallbackStatus : status;

   status = initFrame(pjp);
   if ((status) || (pjp->CallbackStatus))
      return pjp->CallbackStatus ? pjp->CallbackStatus : status;

   status = initScan(pjp);
   if ((status) || (pjp->CallbackStatus))
      return pjp->CallbackStatus ? pjp->CallbackStatus : status;

   pInfo->m_width = pjp->ImageXSize;
   pInfo->m_height = pjp->ImageYSize;
   pInfo->m_comps = pjp->CompsInFrame;
   pInfo->m_scanType = pjp->ScanType;
   pInfo->m_MCUSPerRow = pjp->MaxMCUSPerRow;
   pInfo->m_MCUSPerCol = pjp->MaxMCUSPerCol;
   pInfo->m_MCUWidth = pjp->MaxMCUXSize;
   pInfo->m_MCUHeight = pjp->MaxMCUYSize;
   pInfo->m_pMCUBufR = pjp->MCUBufR;
   pInfo->m_pMCUBufG = pjp->MCUBufG;
   pInfo->m_pMCUBufB = pjp->MCUBufB;
   pInfo->m_outputType = pjp->output_type;
   pInfo->m_PJHandle = pjp;

   return 0;

}

//------------------------------------------------------------------------------
// Streams JPEG image decoded from initialized info structure. Returns NULL on failure.
unsigned char pjpeg_decode_scanlines(pjpeg_image_info_t *pInfo, jswrite_t *jswrite, void *pCallback_data)
{
    int mcu_x;
    int mcu_y;
    int xcur;
    int preinsert;
    uint8 *pPixCur;
    uint8 *pPixStart;
    uint8 *pSrcR;
    uint8 *pSrcG;
    uint8 *pSrcB;
    uint8 status;
    int y, x;
    unsigned int pixsize;
    PicoJpeg *pjp = (PicoJpeg *)pInfo->m_PJHandle;

    switch(pInfo->m_outputType) {
        case PJPG_GRAY8:
        case PJPG_REDUCED_GRAY8:
            pixsize = 1;
            break;
        case PJPG_RGB565:
        case PJPG_REDUCED_RGB565:
            pixsize = 2;
            break;
        case PJPG_RGB888:
        case PJPG_REDUCED_RGB888:
            pixsize = 3;
            break;
        default:
            return PJPG_UNSUPPORTED_COLORSPACE;
    }
    if (pInfo->m_linebuf == 0) {
        return PJPG_NOTENOUGHMEM;
    }
    // If our decompress window is bigger than the image and this is the first
    // decode call, insert needed blank lines
    if (pjp->ImageYWindow > pjp->ImageYSize &&
            pjp->NumMCUSRemaining == pjp->MaxMCUSPerRow * pjp->MaxMCUSPerCol) {
        uint8 *p = (uint8 *)pInfo->m_linebuf;
        uint8 *e = p + pjpeg_get_line_buffer_size(pInfo);
        uint16 u = (pjp->ImageYWindow - pjp->ImageYSize)/2;
        pjp->ImageYWindow -= u;
        /* Clear line buffer.  Could use memset, but we are avoiding using libraries. */
        while (p < e) {
            *p++ = 0;
        }
        /* Write our blank leading lines */
        while (u-- > 0) {
            (*jswrite)(pInfo->m_linebuf, pInfo->m_width*pixsize, pCallback_data);
        }
    }
    preinsert = (pjp->ImageXWindow > pjp->ImageXSize) ?
            ((pjp->ImageXWindow - pjp->ImageXSize)/2)*pixsize : 0;
    for (mcu_y = 0; mcu_y < pInfo->m_MCUSPerCol; ++mcu_y) {
        for (mcu_x = 0; mcu_x < pInfo->m_MCUSPerRow; ++mcu_x) {
            if ((status = pjpeg_decode_mcu(pInfo)) != PJPG_OK) {
               return status;
            }
            pPixStart = (uint8*)pInfo->m_linebuf + (mcu_x*pInfo->m_MCUWidth - pjp->ImageXOffset)*pixsize;
            xcur = mcu_x*pInfo->m_MCUWidth;
            pSrcR = pInfo->m_pMCUBufR;
            pSrcG = pInfo->m_pMCUBufG;
            pSrcB = pInfo->m_pMCUBufB;
            for (y = 0; y < pInfo->m_MCUHeight; ++y) {
                pPixCur = pPixStart + y*pInfo->m_width*pixsize;
                // If we have to insert pixels before image
                if (mcu_x == 0 && preinsert > 0) {
                    int u;
                    for (u = 0; u < preinsert; u++) {
                        *(pPixCur + u) = 0;
                    }
                }
                pPixCur += preinsert;
                for (x = 0; x < pInfo->m_MCUWidth; ++x) {
                    if ((xcur + x < pjp->ImageXOffset) || // Pixel is outside window
                            (xcur + x >= pjp->ImageXOffset + pjp->ImageXWindow)) {
                        pSrcR++;
                        pSrcG++;
                        pSrcB++;
                        continue;
                    }
                    switch(pInfo->m_outputType) {
                        case PJPG_GRAY8:
                        case PJPG_REDUCED_GRAY8:
                            *pPixCur++ = *pSrcR++;
                            break;
                        case PJPG_RGB565:
                        case PJPG_REDUCED_RGB565:
                            #if RGB565_LITTLE_ENDIAN
                            *pPixCur++ = (uint8)(((*pSrcG << 3) & 0xE0) | ((*pSrcB++ >> 3) & 0x1F));
                            *pPixCur++ = (uint8)((*pSrcR++ & 0xF8) | ((*pSrcG++ >> 5) & 0x07));
                            #else
                            *pPixCur++ = (uint8)((*pSrcR++ & 0xF8) | ((*pSrcG >> 5) & 0x07));
                            *pPixCur++ = (uint8)(((*pSrcG++ << 3) & 0xE0) | ((*pSrcB++ >> 3) & 0x1F));
                            #endif
                            break;
                        case PJPG_RGB888:
                        case PJPG_REDUCED_RGB888:
                        default:
                            *pPixCur++ = *pSrcR++;
                            *pPixCur++ = *pSrcG++;
                            *pPixCur++ = *pSrcB++;
                            break;
                    }
                }
                // If we have to insert pixels after image
                if (mcu_x == pInfo->m_MCUSPerRow - 1 && preinsert > 0) {
                    int u;
                    for (u = 0; u < preinsert; u++) {
                        *pPixCur++ = 0;
                    }
                    // Add an extra pixel at the end if an odd number of padding pixels
                    if (((pjp->ImageXWindow - pjp->ImageXSize) & 1) != 0) {
                        for (u = 0; u < pixsize; u++) {
                            *pPixCur++ = 0;
                        }
                    }
                }
            }
        }
        pPixCur = pInfo->m_linebuf;
        for (y = 0; y < pInfo->m_MCUHeight; ++y) {
            if (pjp->ImageYOffset > 0) {  // We need to crop, so discard this line
                --pjp->ImageYOffset;
            } else if (pjp->ImageYWindow > 0) {
                (*jswrite)(pPixCur, pInfo->m_width*pixsize, pCallback_data);
                pPixCur += pInfo->m_width*pixsize;
                --pjp->ImageYWindow;
            }
        }
        // If our decompress window is bigger than the image and this is the last
        // decode call, insert needed blank lines
        if (pjp->NumMCUSRemaining == 0 && pjp->ImageYWindow > 0) {
            uint8 *p = (uint8 *)pInfo->m_linebuf;
            uint8 *e = p + pjpeg_get_line_buffer_size(pInfo);
            /* Clear line buffer.  Could use memset, but we are avoiding using libraries. */
            while (p < e) {
                *p++ = 0;
            }
            /* Write our blank trailing lines */
            while (pjp->ImageYWindow-- > 0) {
                (*jswrite)(pInfo->m_linebuf, pInfo->m_width*pixsize, pCallback_data);
            }
        }
    }
   return PJPG_OK;
}

//------------------------------------------------------------------------------
unsigned int pjpeg_get_storage_size(void) {
    return (unsigned int)sizeof(PicoJpeg);
}

//------------------------------------------------------------------------------
unsigned int pjpeg_get_line_buffer_size(pjpeg_image_info_t *pInfo) {
    unsigned int pixsize;
    switch(pInfo->m_outputType) {
        case PJPG_GRAY8:
        case PJPG_REDUCED_GRAY8:
            pixsize = 1;
            break;
        case PJPG_RGB565:
        case PJPG_REDUCED_RGB565:
            pixsize = 2;
            break;
        case PJPG_RGB888:
        case PJPG_REDUCED_RGB888:
        default:
            pixsize = 3;
            break;
    }
    return (unsigned int)(pInfo->m_width*pInfo->m_MCUHeight*pixsize);
}

//------------------------------------------------------------------------------
unsigned char pjpeg_set_window(pjpeg_image_info_t *pInfo, unsigned int width_pixels,
      unsigned int height_pixels, int left_offset_pixels, int top_offset_pixels) {
   PicoJpeg *pjp;
   if (pInfo == 0 || (pjp = pInfo->m_PJHandle) == 0 || pjp->ImageXSize == 0 || pjp->ImageYSize == 0) {
      return(PJPG_NOT_JPEG);
   }
   width_pixels &= ~1U;
   if ((uint16)width_pixels < pjp->ImageXSize) {
      if (left_offset_pixels < 0 || (uint16)left_offset_pixels > pjp->ImageXSize - (uint16)width_pixels) {
         pjp->ImageXOffset = (pjp->ImageXSize - (uint16)width_pixels)/2;
      } else {
         pjp->ImageXOffset = (uint16)left_offset_pixels;
      }
   } else {
      pjp->ImageXOffset = 0;
   }
   pjp->ImageXWindow = (uint16)width_pixels;
   pInfo->m_width = pjp->ImageXWindow;
   if ((uint16)height_pixels < pjp->ImageYSize) {
      if (top_offset_pixels < 0 || (uint16)top_offset_pixels > pjp->ImageYSize - (uint16)height_pixels) {
         pjp->ImageYOffset = (pjp->ImageYSize - (uint16)height_pixels)/2;
      } else {
         pjp->ImageYOffset = (uint16)top_offset_pixels;
      }
   } else {
      pjp->ImageYOffset = 0;
   }
   pjp->ImageYWindow = (uint16)height_pixels;
   pInfo->m_height = pjp->ImageYWindow;
   return PJPG_OK;
}

