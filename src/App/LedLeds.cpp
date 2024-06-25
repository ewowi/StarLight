/*
   @title     StarLight
   @file      LedLeds.cpp
   @date      20240226
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "LedLeds.h"
#include "../Sys/SysModSystem.h"  //for sys->now
#ifdef STARBASE_USERMOD_MPU6050
  #include "../User/UserModMPU6050.h"
#endif

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

unsigned16 Leds::XYZ(Coord3D pixel) {

  //as this is a call to a virtual function it reduces the theoretical (no show) speed by half, even if XYZ is not implemented
  //  the real speed is hardly affected, but room for improvement!
  //  so as a workaround we list them here explicetly
  if ((projectionNr == p_TiltPanRoll || projectionNr == p_Preset1) && projectionNr < fixture->projections.size())
    fixture->projections[projectionNr]->adjustXYZ(*this, pixel);

  return XYZUnprojected(pixel);
}

// maps the virtual led to the physical led(s) and assign a color to it
void Leds::setPixelColor(unsigned16 indexV, CRGB color, unsigned8 blendAmount) {
  if (indexV < mappingTable.size()) {
    if (mappingTable[indexV].isOneIndex()) {
      uint16_t indexP = mappingTable[indexV].indexP[0];
      fixture->ledsP[indexP] = blend(color, fixture->ledsP[indexP], blendAmount==UINT8_MAX?fixture->globalBlend:blendAmount);
    } else if (mappingTable[indexV].isMultipleIndexes()) {
      for (forUnsigned16 indexP:*mappingTable[indexV].indexes) {
        fixture->ledsP[indexP] = blend(color, fixture->ledsP[indexP], blendAmount==UINT8_MAX?fixture->globalBlend:blendAmount);
      }
    }
    else {
      mappingTable[indexV].setColor(color);
    }
  }
  else if (indexV < NUM_LEDS_Max) //no projection
    fixture->ledsP[(projectionNr == p_Random)?random(fixture->nrOfLeds):indexV] = color;
  else if (indexV != UINT16_MAX) //assuming UINT16_MAX is set explicitly (e.g. in XYZ)
    ppf(" dev sPC V:%d >= %d", indexV, NUM_LEDS_Max);
}

CRGB Leds::getPixelColor(unsigned16 indexV) {
  if (indexV < mappingTable.size()) {
    if (mappingTable[indexV].isOneIndex())
      return fixture->ledsP[mappingTable[indexV].indexP[0]]; //any would do as they are all the same
    if (mappingTable[indexV].isMultipleIndexes())
      return fixture->ledsP[*mappingTable[indexV].indexes->begin()]; //any would do as they are all the same
    else 
      return mappingTable[indexV].color;
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
    for (PhysMap &map:mappingTable) {
      if (map.isOneIndex()) {
          uint16_t indexP = map.indexP[0];
          CRGB oldValue = fixture->ledsP[indexP];
          fixture->ledsP[indexP].nscale8(255-fadeBy); //this overrides the old value
          fixture->ledsP[indexP] = blend(fixture->ledsP[indexP], oldValue, fixture->globalBlend); // we want to blend in the old value
      } else if (map.isMultipleIndexes())
        for (forUnsigned16 indexP:*map.indexes) {
          CRGB oldValue = fixture->ledsP[indexP];
          fixture->ledsP[indexP].nscale8(255-fadeBy); //this overrides the old value
          fixture->ledsP[indexP] = blend(fixture->ledsP[indexP], oldValue, fixture->globalBlend); // we want to blend in the old value
        }
    }
  }
}

void Leds::fill_solid(const struct CRGB& color, bool noBlend) {
  if (projectionNr == p_None || projectionNr == p_Random) {
    fastled_fill_solid(fixture->ledsP, fixture->nrOfLeds, color);
  } else {
    for (PhysMap &map:mappingTable) {
      if (map.isOneIndex()) {
          uint16_t indexP = map.indexP[0];
          fixture->ledsP[indexP] = noBlend?color:blend(color, fixture->ledsP[indexP], fixture->globalBlend);
      } else if (map.isMultipleIndexes())
        for (forUnsigned16 indexP:*map.indexes) {
          fixture->ledsP[indexP] = noBlend?color:blend(color, fixture->ledsP[indexP], fixture->globalBlend);
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

    for (PhysMap &map:mappingTable) {
      if (map.isOneIndex()) {
          uint16_t indexP = map.indexP[0];
          fixture->ledsP[indexP] = blend(hsv, fixture->ledsP[indexP], fixture->globalBlend);
      } else if (map.isMultipleIndexes())
        for (forUnsigned16 indexP:*map.indexes) {
          fixture->ledsP[indexP] = blend(hsv, fixture->ledsP[indexP], fixture->globalBlend);
        }
      hsv.hue += deltahue;
    }
  }
}
