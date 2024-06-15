/*
   @title     StarLeds
   @file      LedLeds.cpp
   @date      20240226
   @repo      https://github.com/MoonModules/StarLeds
   @Authors   https://github.com/MoonModules/StarLeds/commits/main
   @Copyright © 2024 Github StarLeds Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "LedLeds.h"
#include "../Sys/SysModSystem.h"

//convenience functions to call fastled functions out of the Leds namespace (there naming conflict)
void fastled_fadeToBlackBy(CRGB* leds, unsigned16 num_leds, unsigned8 fadeBy) {
  fadeToBlackBy(leds, num_leds, fadeBy);
}
void fastled_fill_solid( struct CRGB * targetArray, int numToFill, const struct CRGB& color) {
  fill_solid(targetArray, numToFill, color);
}
void fastled_fill_rainbow(struct CRGB * targetArray, int numToFill, unsigned8 initialhue, unsigned8 deltahue) {
  fill_rainbow(targetArray, numToFill, initialhue, deltahue);
}

unsigned16 Leds::XYZ(unsigned16 x, unsigned16 y, unsigned16 z) {
  if (projectionNr == p_TiltPanRoll || projectionNr == p_Preset1) {
    Coord3D result = Coord3D{x, y, z};
    if (proTiltSpeed) result = trigoTiltPanRoll.tilt(result, size/2, sys->now * 5 / (255 - proTiltSpeed));
    if (proPanSpeed) result = trigoTiltPanRoll.pan(result, size/2, sys->now * 5 / (255 - proPanSpeed));
    if (proRollSpeed) result = trigoTiltPanRoll.roll(result, size/2, sys->now * 5 / (255 - proRollSpeed));
    if (fixture->fixSize.z == 1) result.z = 0; // 3d effects will be flattened on 2D fixtures
    if (result >= 0 && result < size)
      return result.x + result.y * size.x + result.z * size.x * size.y;
    else 
      return UINT16_MAX;
  }
  else
    return x + y * size.x + z * size.x * size.y;
}

// maps the virtual led to the physical led(s) and assign a color to it
void Leds::setPixelColor(unsigned16 indexV, CRGB color, unsigned8 blendAmount) {
  if (indexV < mappingTable.size()) {
    if (mappingTable[indexV].size() > 0 && mappingTable[indexV][0] != UINT16_MAX) { //isMapped
      for (size_t p=0; p<mappingTable[indexV].size(); p++) {
        uint16_t indexP = mappingTable[indexV][p];
        fixture->ledsP[indexP] = blend(color, fixture->ledsP[indexP], blendAmount==UINT8_MAX?fixture->globalBlend:blendAmount);
      }
    }
    else {
      while (mappingTable[indexV].size() < 4) mappingTable[indexV].push_back(0); //create space for CRGB
      mappingTable[indexV][0] = UINT16_MAX; //not mapped, color stored
      mappingTable[indexV][1] = color.r;
      mappingTable[indexV][2] = color.g;
      mappingTable[indexV][3] = color.b;
    }
  }
  else if (indexV < NUM_LEDS_Max) //no projection
    fixture->ledsP[projectionNr==p_Random?random(fixture->nrOfLeds):indexV] = color;
  else if (indexV != UINT16_MAX) //assuming UINT16_MAX is set explicitly (e.g. in XYZ)
    ppf(" dev sPC V:%d >= %d", indexV, NUM_LEDS_Max);
}

CRGB Leds::getPixelColor(unsigned16 indexV) {
  if (indexV < mappingTable.size()) {
    if (mappingTable[indexV].size() > 0 && mappingTable[indexV][0] != UINT16_MAX) //isMapped
      return fixture->ledsP[mappingTable[indexV][0]]; //any would do as they are all the same
    else
      return CRGB(mappingTable[indexV][1], mappingTable[indexV][2], mappingTable[indexV][3]);
  }
  else if (indexV < NUM_LEDS_Max) //no mapping
    return fixture->ledsP[indexV];
  else {
    ppf(" dev gPC N: %d >= %d", indexV, NUM_LEDS_Max);
    return CRGB::Black;
  }
}

void Leds::fadeToBlackBy(unsigned8 fadeBy) {
  if (projectionNr == p_None || projectionNr == p_Random) {
    fastled_fadeToBlackBy(fixture->ledsP, fixture->nrOfLeds, fadeBy);
  } else {
    for (size_t indexV=0; indexV<mappingTable.size(); indexV++) {
      if (mappingTable[indexV][0] != UINT16_MAX) {
        for (size_t p=0; p<mappingTable[indexV].size(); p++) {
          uint16_t indexP = mappingTable[indexV][p];
          CRGB oldValue = fixture->ledsP[indexP];
          fixture->ledsP[indexP].nscale8(255-fadeBy); //this overrides the old value
          fixture->ledsP[indexP] = blend(fixture->ledsP[indexP], oldValue, fixture->globalBlend); // we want to blend in the old value
        }
      }
    }
  }
}

void Leds::fill_solid(const struct CRGB& color, bool noBlend) {
  if (projectionNr == p_None || projectionNr == p_Random) {
    fastled_fill_solid(fixture->ledsP, fixture->nrOfLeds, color);
  } else {
    for (size_t indexV=0; indexV<mappingTable.size(); indexV++) {
      if (mappingTable[indexV][0] != UINT16_MAX) {
        for (size_t p=0; p<mappingTable[indexV].size(); p++) {
          uint16_t indexP = mappingTable[indexV][p];
          fixture->ledsP[indexP] = noBlend?color:blend(color, fixture->ledsP[indexP], fixture->globalBlend);
        }
      }
    }
  }
}

void Leds::fill_rainbow(unsigned8 initialhue, unsigned8 deltahue) {
  if (projectionNr == p_None || projectionNr == p_Random) {
    fastled_fill_rainbow(fixture->ledsP, fixture->nrOfLeds, initialhue, deltahue);
  } else {
    CHSV hsv;
    hsv.hue = initialhue;
    hsv.val = 255;
    hsv.sat = 240;

    for (size_t indexV=0; indexV<mappingTable.size(); indexV++) {
      if (mappingTable[indexV][0] != UINT16_MAX) {
        for (size_t p=0; p<mappingTable[indexV].size(); p++) {
          uint16_t indexP = mappingTable[indexV][p];
          fixture->ledsP[indexP] = blend(hsv, fixture->ledsP[indexP], fixture->globalBlend);
        }
      }
      hsv.hue += deltahue;
    }
  }
}
