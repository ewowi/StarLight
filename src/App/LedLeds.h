/*
   @title     StarLeds
   @file      LedLeds.h
   @date      20240227
   @repo      https://github.com/MoonModules/StarLeds
   @Authors   https://github.com/MoonModules/StarLeds/commits/main
   @Copyright © 2024 Github StarLeds Commit Authors
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
  p_Multiply,
  p_TiltPanRoll,
  p_DistanceFromPoint,
  p_Preset1,
  p_Random,
  p_Reverse,
  p_Mirror,
  p_Grouping,
  p_Spacing,
  p_Kaleidoscope,
  p_Pinwheel,
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



//StarLeds implementation of segment.data
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

  //sets the sharedData pointer back to 0 so loop effect can go through it
  void loop() {
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

struct PhysMap {
  union {
    std::vector<unsigned16> * indexes;
    CRGB color;
    uint16_t indexP[2];
    byte type[4]; //type[3] == 63 for indexes pointers 
  }; // 4 bytes

  PhysMap() {
    indexes = nullptr; //all zero's
    setInitType();
  }

  void setInitType() {
    type[3] = 255;
  }
  void setColorType() {
    type[3] = 254;
  }
  void setOneIndexType() {
    type[3] = 253;
  }

  bool isInit() {
    return type[3] == 255;
  }
  bool isColor() {
    return type[3] == 254;
  }
  bool isOneIndex() {
    return type[3] == 253;
  }
  bool isMultipleIndexes() {
    return type[3] < 253;
  }
  bool isOneOrMoreIndex() {
    return type[3] <= 253;
  }

  void setColor(CRGB color) {
    this->color = color;
    setColorType();
    // ppf("dev new color %d,%d,%d t: %d,%d,%d,%d\n", this->color.r, this->color.g, this->color.b, type[0], type[1], type[2], type[3]);
    //dev new color 245,0,10 t: 245,0,10,255
  }

  void addIndexP(uint16_t indexP) {
    if (isInit()) { //no indexes stored yet
      this->indexP[0] = indexP;
      // ppf("dev new indexP:%d type:%d,%d,%d,%d %d-%d\n", indexP, type[0], type[1], type[2], type[3], this->indexP[0], this->indexP[1]);
      //dev new indexP:672 type:160,2,0,255 672-65280
      setOneIndexType();
    } else if (isOneIndex()) { //move to indexes
      uint16_t oldIndex = this->indexP[0];
      indexes = new std::vector<unsigned16>; //overwrite the oneIndex
      // ppf("dev new indexes %d type:%d,%d,%d,%d p:%p\n", indexP, type[0], type[1], type[2], type[3], indexes);
      //dev new indexes 894 type:88,122,254,63 p:0x3ffe7a58
      if (!isMultipleIndexes()) //check if pointer is not setting the type[3] value
        ppf("dev new PhysMap type:%d t3:%d b:%d p:%p\n", type, type[3], type[3] & 0x80, indexes);
      else {
        indexes->push_back(oldIndex); //add the old to the indexes vector
      }
    }

    if (isMultipleIndexes()) {
      indexes->push_back(indexP); //add the new index to the indexesvector
    }
  }

}; // 4 bytes

class Leds {

public:

  Fixture *fixture;

  unsigned16 nrOfLeds = 64;  //amount of virtual leds (calculated by projection)

  Coord3D size = {8,8,1}; //not 0,0,0 to prevent div0 eg in Octopus2D

  uint16_t fx = -1;
  unsigned8 projectionNr = -1;
  unsigned8 effectDimension = -1;
  unsigned8 projectionDimension = -1;

  Coord3D startPos = {0,0,0}, endPos = {UINT16_MAX,UINT16_MAX,UINT16_MAX}; //default
  #ifdef STARBASE_USERMOD_MPU6050
    bool proGyro = false;
  #endif
  unsigned8 proTiltSpeed = 128;
  unsigned8 proPanSpeed = 128;
  unsigned8 proRollSpeed = 128;

  SharedData sharedData;

  std::vector<PhysMap> mappingTable;

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
    fadeToBlackBy(100);
    doMap = true; // so loop is not running while deleting
    for (PhysMap &map:mappingTable) {
      if (map.isMultipleIndexes()) {
        map.indexes->clear();
        delete map.indexes;
      }
    }
    mappingTable.clear();
  }

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

  CRGB getPixelColor(unsigned16 indexV);
  CRGB getPixelColor(Coord3D pixel) {return getPixelColor(XYZ(pixel));}

  void addPixelColor(unsigned16 indexV, CRGB color) {setPixelColor(indexV, getPixelColor(indexV) + color);}
  void addPixelColor(Coord3D pixel, CRGB color) {setPixelColor(pixel, getPixelColor(pixel) + color);}

  void fadeToBlackBy(unsigned8 fadeBy = 255);
  void fill_solid(const struct CRGB& color, bool noBlend = false);
  void fill_rainbow(unsigned8 initialhue, unsigned8 deltahue);

  //checks if a virtual pixel is mapped to a physical pixel (use with XY() or XYZ() to get the indexV)
  bool isMapped(unsigned16 indexV) {
    return indexV < mappingTable.size() && mappingTable[indexV].isOneOrMoreIndex();
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