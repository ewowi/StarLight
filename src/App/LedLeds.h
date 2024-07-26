/*
   @title     StarLight
   @file      LedLeds.h
   @date      20240720
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
// FastLED optional flags to configure drivers, see https://github.com/FastLED/FastLED/blob/master/src/platforms/esp/32
// RMT driver (default)
// #define FASTLED_ESP32_FLASH_LOCK 1    // temporarily disabled FLASH file access while driving LEDs (may prevent random flicker)
// #define FASTLED_RMT_BUILTIN_DRIVER 1  // in case your app needs to use RMT units, too (slower)
// I2S parallel driver
// #define FASTLED_ESP32_I2S true        // to use I2S parallel driver (instead of RMT)
// #define I2S_DEVICE 1                  // I2S driver: allows to still use I2S#0 for audio (only on esp32 and esp32-s3)
// #define FASTLED_I2S_MAX_CONTROLLERS 8 // 8 LED pins should be enough (default = 24)
#include "FastLED.h"

#include "LedFixture.h"

#include "../data/font/console_font_4x6.h"
#include "../data/font/console_font_5x8.h"
#include "../data/font/console_font_5x12.h"
#include "../data/font/console_font_6x8.h"
#include "../data/font/console_font_7x9.h"

#define NUM_VLEDS_Max 8192

enum ProjectionsE
{
  p_None,
  p_Default,
  p_Pinwheel,
  p_Multiply,
  p_TiltPanRoll,
  p_DistanceFromPoint,
  p_Preset1,
  p_Random,
  p_Reverse,
  p_Mirror,
  p_Grouping,
  p_Spacing,
  p_Transpose,
  p_Kaleidoscope,
  p_count // keep as last entry
};

//     sin8/cos8   sin16/cos16
//0:   128, 255    0 32645
//64:  255, 128    32645 0
//128: 128, 1      0 -32645
//192: 1, 127      -32645 0

static unsigned trigoCached = 1;
static unsigned trigoUnCached = 1;

struct Trigo {
  uint16_t period = 360; //default period 360
  Trigo(uint16_t period = 360) {this->period = period;}
  float sinValue[3]; uint16_t sinAngle[3] = {UINT16_MAX,UINT16_MAX,UINT16_MAX}; //caching of sinValue=sin(sinAngle) for tilt, pan and roll
  float cosValue[3]; uint16_t cosAngle[3] = {UINT16_MAX,UINT16_MAX,UINT16_MAX}; //caching of cosValue=cos(cosAngle) for tilt, pan and roll
  virtual float sinBase(uint16_t angle) {return sinf(M_TWOPI * angle / period);}
  virtual float cosBase(uint16_t angle) {return cosf(M_TWOPI * angle / period);}
  int16_t sin(int16_t factor, uint16_t angle, uint8_t cache012 = 0) {
    if (sinAngle[cache012] != angle) {sinAngle[cache012] = angle; sinValue[cache012] = sinBase(angle);trigoUnCached++;} else trigoCached++;
    return factor * sinValue[cache012];
  };
  int16_t cos(int16_t factor, uint16_t angle, uint8_t cache012 = 0) {
    if (cosAngle[cache012] != angle) {cosAngle[cache012] = angle; cosValue[cache012] = cosBase(angle);trigoUnCached++;} else trigoCached++;
    return factor * cosValue[cache012];
  };
  // https://msl.cs.uiuc.edu/planning/node102.html
  Coord3D pan(Coord3D in, Coord3D middle, uint16_t angle) {
    Coord3D inM = in - middle;
    Coord3D out;
    out.x = cos(inM.x, angle, 0) + sin(inM.z, angle, 0);
    out.y = inM.y;
    out.z = - sin(inM.x, angle, 0) + cos(inM.z, angle, 0);
    return out + middle;
  }
  Coord3D tilt(Coord3D in, Coord3D middle, uint16_t angle) {
    Coord3D inM = in - middle;
    Coord3D out;
    out.x = inM.x;
    out.y = cos(inM.y, angle, 1) - sin(inM.z, angle, 1);
    out.z = sin(inM.y, angle, 1) + cos(inM.z, angle, 1);
    return out + middle;
  }
  Coord3D roll(Coord3D in, Coord3D middle, uint16_t angle) {
    Coord3D inM = in - middle;
    Coord3D out;
    out.x = cos(inM.x, angle, 2) - sin(inM.y, angle, 2);
    out.y = sin(inM.x, angle, 2) + cos(inM.y, angle, 2);
    out.z = inM.z;
    return out + middle;
  }
  Coord3D rotate(Coord3D in, Coord3D middle, uint16_t tiltAngle, uint16_t panAngle, uint16_t rollAngle, uint16_t period = 360) {
    this->period = period;
    return roll(pan(tilt(in, middle, tiltAngle), middle, panAngle), middle, rollAngle);
  }
};

struct Trigo8: Trigo { //FastLed sin8 and cos8
  using Trigo::Trigo;
  float sinBase(uint16_t angle) {return (sin8(256.0f * angle / period) - 128) / 127.0f;}
  float cosBase(uint16_t angle) {return (cos8(256.0f * angle / period) - 128) / 127.0f;}
};
struct Trigo16: Trigo { //FastLed sin16 and cos16
  using Trigo::Trigo;
  float sinBase(uint16_t angle) {return sin16(65536.0f * angle / period) / 32645.0f;}
  float cosBase(uint16_t angle) {return cos16(65536.0f * angle / period) / 32645.0f;}
};

static Trigo trigoTiltPanRoll(255); // Trigo8 is hardly any faster (27 vs 28 fps) (spanXY=28)

class Fixture; //forward


//StarLight implementation of segment.data
class SharedData {

  private:
    byte *data;
    unsigned16 index = 0;
    unsigned16 bytesAllocated = 0;

  public:

  SharedData() {
    // ppf("SharedData constructor %d %d\n", index, bytesAllocated);
  }
  ~SharedData() {
    // ppf("SharedData destructor WIP %d %d\n", index, bytesAllocated);
    // free(data);
  }

  void reset() {
    memset(data, 0, bytesAllocated);
    index = 0;
  }

  //sets the effectData pointer back to 0 so loop effect can go through it
  void begin() {
    index = 0;
  }

  //returns the next pointer to a specified type (length for arrays)
  template <typename Type>
  Type * readWrite(int length = 1) {
    size_t newIndex = index + length * sizeof(Type);
    if (newIndex > bytesAllocated) {
      size_t newSize = bytesAllocated + (1 + ( newIndex - bytesAllocated)/1024) * 1024; // add a multitude of 1024 bytes
      ppf("bind add more %d->%d %d->%d\n", index, newIndex, bytesAllocated, newSize);
      if (bytesAllocated == 0)
        data = (byte*) malloc(newSize);
      else
        data = (byte*)realloc(data, newSize);
      bytesAllocated = newSize;
    }
    // ppf("bind %d->%d %d\n", index, newIndex, bytesAllocated);
    Type * returnValue  = reinterpret_cast<Type *>(data + index);
    index = newIndex; //add consumed amount of bytes, index is next byte which will be pointed to
    return returnValue;
  }

  //returns the next pointer initialized by a value (length for arrays not supported yet)
  template <typename Type>
  Type * write(Type initValue) {
    Type * returnValue =  readWrite<Type>();
    *returnValue = initValue;
    return returnValue;
  }

  //returns the next value (length for arrays not supported yet)
  template <typename Type>
  Type read() {
    Type *result = readWrite<Type>(); //not supported for arrays yet
    return *result;
  }

};

enum mapType {
  m_color,
  m_onePixel,
  m_morePixels,
  m_colorPal,
  m_count //keep as last entry
};

class Leds; //forward


struct PhysMap {
  union {
    struct {                 //condensed rgb, keep in GBR order!!! exceptional cases when no palette. e.g. solid?
      uint8_t g:5;           //32
      uint8_t b:3;           //8
      uint8_t r:6;           //64
      byte mapType:2;         //2 bits (4)
    }; //16 bits
    uint16_t indexP: 14;   //16384 one physical pixel (type==1) index to ledsP array
    uint16_t indexes:14;  //16384 multiple physical pixels (type==2) index in std::vector<std::vector<unsigned16>> mappingTableIndexes;
    struct {                  //no physical pixel (type==0) palette (all linearblend)
      uint8_t palIndex:8;     //8 bits (256)
      uint8_t palBri:6;       //6 bits (64)
    }; // 14 bits
  }; // 2 bytes

  PhysMap() {
    mapType = m_color; // the default until indexP is added
  }

  void addIndexP(Leds &leds, uint16_t indexP);

}; // 2 bytes

class Projection; //forward for cached virtual class methods!

class Leds {

public:

  Fixture *fixture;

  unsigned16 nrOfLeds = 64;  //amount of virtual leds (calculated by projection)

  Coord3D size = {8,8,1}; //not 0,0,0 to prevent div0 eg in Octopus2D

  uint16_t fx = -1;
  unsigned8 projectionNr = -1;

  //using cached virtual class methods! 4 bytes each - thats for now the price we pay for speed
  void (Projection::*setupCached)(Leds &, Coord3D &, Coord3D &, Coord3D &, Coord3D &, uint16_t &) = nullptr;
  void (Projection::*adjustXYZCached)(Leds &, Coord3D &) = nullptr;

  unsigned8 effectDimension = -1;
  unsigned8 projectionDimension = -1;

  Coord3D startPos = {0,0,0}, endPos = {UINT16_MAX,UINT16_MAX,UINT16_MAX}; //default
  Coord3D midPos = {0,0,0};
  #ifdef STARBASE_USERMOD_MPU6050
    bool proGyro = false;
  #endif
  unsigned8 proTiltSpeed = 128;
  unsigned8 proPanSpeed = 128;
  unsigned8 proRollSpeed = 128;

  SharedData effectData;
  SharedData projectionData;

  std::vector<PhysMap> mappingTable;
  std::vector<std::vector<unsigned16>> mappingTableIndexes;


  unsigned16 indexVLocal = 0; //set in operator[], used by operator=

  bool doMap = false;

  CRGBPalette16 palette;

  unsigned16 XY(unsigned16 x, unsigned16 y) {
    return XYZ(x, y, 0);
  }

  unsigned16 XYZUnprojected(Coord3D pixel) {
    if (pixel >= 0 && pixel < size)
      return pixel.x + pixel.y * size.x + pixel.z * size.x * size.y;
    else
      return UINT16_MAX;
  }

  unsigned16 XYZ(unsigned16 x, unsigned16 y, unsigned16 z) {
    return XYZ({x, y, z});
  }

  unsigned16 XYZ(Coord3D pixel);

  Leds(Fixture &fixture) {
    ppf("Leds constructor (PhysMap:%d)\n", sizeof(PhysMap));
    this->fixture = &fixture;
  }

  ~Leds() {
    ppf("Leds destructor\n");
    fadeToBlackBy();
    doMap = true; // so loop is not running while deleting
    for (PhysMap &map:mappingTable) {
      mappingTableIndexes.clear();
    }
    mappingTable.clear();
  }

  void triggerMapping();

  // indexVLocal stored to be used by other operators
  Leds& operator[](unsigned16 indexV) {
    indexVLocal = indexV;
    return *this;
  }

  Leds& operator[](Coord3D pos) {
    indexVLocal = XYZ(pos.x, pos.y, pos.z);
    return *this;
  }

  // CRGB& operator[](unsigned16 indexV) {
  //   // indexVLocal = indexV;
  //   CRGB x = getPixelColor(indexV);
  //   return x;
  // }

  // uses indexVLocal and color to call setPixelColor
  Leds& operator=(const CRGB color) {
    setPixelColor(indexVLocal, color);
    return *this;
  }

  Leds& operator+=(const CRGB color) {
    setPixelColor(indexVLocal, getPixelColor(indexVLocal) + color);
    return *this;
  }
  Leds& operator|=(const CRGB color) {
    // setPixelColor(indexVLocal, color);
    setPixelColor(indexVLocal, getPixelColor(indexVLocal) | color);
    return *this;
  }

  // Leds& operator+(const CRGB color) {
  //   setPixelColor(indexVLocal, getPixelColor(indexVLocal) + color);
  //   return *this;
  // }


  // maps the virtual led to the physical led(s) and assign a color to it
  void setPixelColor(unsigned16 indexV, CRGB color, unsigned8 blendAmount = UINT8_MAX);
  void setPixelColor(Coord3D pixel, CRGB color, unsigned8 blendAmount = UINT8_MAX) {setPixelColor(XYZ(pixel), color, blendAmount);}

  // temp methods until all effects have been converted to Palette / 2 byte mapping mode
  void setPixelColorPal(unsigned16 indexV, uint8_t palIndex, uint8_t palBri = 255, unsigned8 blendAmount = UINT8_MAX);
  void setPixelColorPal(Coord3D pixel, uint8_t palIndex, uint8_t palBri = 255, unsigned8 blendAmount = UINT8_MAX) {setPixelColorPal(XYZ(pixel), palIndex, palBri, blendAmount);}

  CRGB getPixelColor(unsigned16 indexV);
  CRGB getPixelColor(Coord3D pixel) {return getPixelColor(XYZ(pixel));}

  void addPixelColor(unsigned16 indexV, CRGB color) {setPixelColor(indexV, getPixelColor(indexV) + color);}
  void addPixelColor(Coord3D pixel, CRGB color) {setPixelColor(pixel, getPixelColor(pixel) + color);}

  void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, CRGB color) {
    if (x0 >= size.x || x1 >= size.x || y0 >= size.y || y1 >= size.y) return;
    const int16_t dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    const int16_t dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int16_t err = (dx>dy ? dx : -dy)/2, e2;
    for (;;) {
      setPixelColor(XY(x0,y0), color);
      if (x0==x1 && y0==y1) break;
      e2 = err;
      if (e2 >-dx) { err -= dy; x0 += sx; }
      if (e2 < dy) { err += dx; y0 += sy; }
    }
  }

  void fadeToBlackBy(unsigned8 fadeBy = 255);
  void fill_solid(const struct CRGB& color, bool noBlend = false);
  void fill_rainbow(unsigned8 initialhue, unsigned8 deltahue);

  //checks if a virtual pixel is mapped to a physical pixel (use with XY() or XYZ() to get the indexV)
  bool isMapped(unsigned16 indexV) {
    return indexV < mappingTable.size() && (mappingTable[indexV].mapType == m_onePixel || mappingTable[indexV].mapType == m_morePixels);
  }

  void blur1d(fract8 blur_amount)
  {
    uint8_t keep = 255 - blur_amount;
    uint8_t seep = blur_amount >> 1;
    CRGB carryover = CRGB::Black;
    for( uint16_t i = 0; i < nrOfLeds; ++i) {
        CRGB cur = getPixelColor(i);
        CRGB part = cur;
        part.nscale8( seep);
        cur.nscale8( keep);
        cur += carryover;
        if( i) addPixelColor(i-1, part);
        setPixelColor(i, cur);
        carryover = part;
    }
  }

  void blur2d(fract8 blur_amount)
  {
      blurRows(size.x, size.y, blur_amount);
      blurColumns(size.x, size.y, blur_amount);
  }

  void blurRows(unsigned8 width, unsigned8 height, fract8 blur_amount)
  {
  /*    for (forUnsigned8 row = 0; row < height; row++) {
          CRGB* rowbase = leds + (row * width);
          blur1d( rowbase, width, blur_amount);
      }
  */
      // blur rows same as columns, for irregular matrix
      stackUnsigned8 keep = 255 - blur_amount;
      stackUnsigned8 seep = blur_amount >> 1;
      for (forUnsigned8 row = 0; row < height; row++) {
          CRGB carryover = CRGB::Black;
          for (forUnsigned8 i = 0; i < width; i++) {
              CRGB cur = getPixelColor(XY(i,row));
              CRGB part = cur;
              part.nscale8( seep);
              cur.nscale8( keep);
              cur += carryover;
              if( i) addPixelColor(XY(i-1,row), part);
              setPixelColor(XY(i,row), cur);
              carryover = part;
          }
      }
  }

  // blurColumns: perform a blur1d on each column of a rectangular matrix
  void blurColumns(unsigned8 width, unsigned8 height, fract8 blur_amount)
  {
      // blur columns
      stackUnsigned8 keep = 255 - blur_amount;
      stackUnsigned8 seep = blur_amount >> 1;
      for (forUnsigned8 col = 0; col < width; ++col) {
          CRGB carryover = CRGB::Black;
          for (forUnsigned8 i = 0; i < height; ++i) {
              CRGB cur = getPixelColor(XY(col,i));
              CRGB part = cur;
              part.nscale8( seep);
              cur.nscale8( keep);
              cur += carryover;
              if( i) addPixelColor(XY(col,i-1), part);
              setPixelColor(XY(col,i), cur);
              carryover = part;
          }
      }
  }

  //shift is used by drawText indicating which letter it is drawing
  void drawCharacter(unsigned char chr, int x = 0, int16_t y = 0, unsigned8 font = 0, CRGB col = CRGB::Red, unsigned16 shiftPixel = 0, unsigned16 shiftChr = 0) {
    if (chr < 32 || chr > 126) return; // only ASCII 32-126 supported
    chr -= 32; // align with font table entries

    Coord3D fontSize;
    switch (font%5) {
      case 0: fontSize.x = 4; fontSize.y = 6; break;
      case 1: fontSize.x = 5; fontSize.y = 8; break;
      case 2: fontSize.x = 5; fontSize.y = 12; break;
      case 3: fontSize.x = 6; fontSize.y = 8; break;
      case 4: fontSize.x = 7; fontSize.y = 9; break;
    }

    Coord3D chrPixel;
    for (chrPixel.y = 0; chrPixel.y<fontSize.y; chrPixel.y++) { // character height
      Coord3D pixel;
      pixel.z = 0;
      pixel.y = y + chrPixel.y;
      if (pixel.y >= 0 && pixel.y < size.y) {
        byte bits = 0;
        switch (font%5) {
          case 0: bits = pgm_read_byte_near(&console_font_4x6[(chr * fontSize.y) + chrPixel.y]); break;
          case 1: bits = pgm_read_byte_near(&console_font_5x8[(chr * fontSize.y) + chrPixel.y]); break;
          case 2: bits = pgm_read_byte_near(&console_font_5x12[(chr * fontSize.y) + chrPixel.y]); break;
          case 3: bits = pgm_read_byte_near(&console_font_6x8[(chr * fontSize.y) + chrPixel.y]); break;
          case 4: bits = pgm_read_byte_near(&console_font_7x9[(chr * fontSize.y) + chrPixel.y]); break;
        }

        for (chrPixel.x = 0; chrPixel.x<fontSize.x; chrPixel.x++) {
          //x adjusted by: chr in text, scroll value, font column
          pixel.x = (x + shiftChr * fontSize.x + shiftPixel + (fontSize.x-1) - chrPixel.x)%size.x;
          if ((pixel.x >= 0 && pixel.x < size.x) && ((bits>>(chrPixel.x+(8-fontSize.x))) & 0x01)) { // bit set & drawing on-screen
            setPixelColor(pixel, col);
          }
        }
      }
    }
  }

  void drawText(const char * text, int x = 0, int16_t y = 0, unsigned8 font = 0, CRGB col = CRGB::Red, unsigned16 shiftPixel = 0) {
    const int numberOfChr = strlen(text); //Core  1 panic'ed (LoadProhibited). Exception was unhandled. - /builds/idf/crosstool-NG/.build/HOST-x86_64-apple-darwin12/xtensa-esp32-elf/src/newlib/newlib/libc/machine/xtensa/strlen.S:82
    for (int shiftChr = 0; shiftChr < numberOfChr; shiftChr++) {
      drawCharacter(text[shiftChr], x, y, font, col, shiftPixel, shiftChr);
    }
  }

};