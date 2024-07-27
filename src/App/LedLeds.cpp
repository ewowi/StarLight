/*
   @title     StarLight
   @file      LedLeds.cpp
   @date      20240720
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

void Leds::triggerMapping() {
    doMap = true; //specify which leds to remap
    fixture->doMap = true; //fixture will also be remapped
  }

unsigned16 Leds::XYZ(Coord3D pixel) {

  //as this is a call to a virtual function it reduces the theoretical (no show) speed by half, even if XYZ is not implemented
  //  the real speed is hardly affected, but room for improvement!
  //  so as a workaround we list them here explicetly
  // if ((projectionNr == p_TiltPanRoll || projectionNr == p_Preset1) && projectionNr < fixture->projections.size())
    // fixture->projections[projectionNr]->adjustXYZ(*this, pixel);


  //using cached virtual class methods! (so no need for if projectionNr optimizations!)
  if (projectionNr < fixture->projections.size())
    (fixture->projections[projectionNr]->*adjustXYZCached)(*this, pixel);

  return XYZUnprojected(pixel);
}

// maps the virtual led to the physical led(s) and assign a color to it
void Leds::setPixelColor(unsigned16 indexV, CRGB color) {
  if (indexV < mappingTable.size()) {
    if (mappingTable[indexV].mapType == m_colorPal) mappingTable[indexV].mapType = m_color;
    switch (mappingTable[indexV].mapType) {
      case m_color:{
        mappingTable[indexV].r = color.r >> 2; // 8 to 6 bits, 64 values
        mappingTable[indexV].g = color.g >> 3; // 8 to 5 bits, 32 values
        mappingTable[indexV].b = min(color.b + 15, 255) >> 5; // 8 to 3 bits, 8 values
        break;
      }
      case m_onePixel: {
        uint16_t indexP = mappingTable[indexV].indexP;
        fixture->ledsP[indexP] = fixture->pixelsToBlend[indexP]?blend(color, fixture->ledsP[indexP], fixture->globalBlend):color;
        break; }
      case m_morePixels:
        if (mappingTable[indexV].indexes < mappingTableIndexes.size())
          for (forUnsigned16 indexP: mappingTableIndexes[mappingTable[indexV].indexes]) {
            fixture->ledsP[indexP] = fixture->pixelsToBlend[indexP]?blend(color, fixture->ledsP[indexP], fixture->globalBlend): color;
          }
        // else
        //   ppf("dev setPixelColor2 i:%d m:%d s:%d\n", indexV, mappingTable[indexV].indexes, mappingTableIndexes.size());
        break;
    }
  }
  else if (indexV < NUM_LEDS_Max) { //no projection
    uint16_t indexP = (projectionNr == p_Random)?random(fixture->nrOfLeds):indexV;
    fixture->ledsP[indexP] = fixture->pixelsToBlend[indexP]?blend(color, fixture->ledsP[indexP], fixture->globalBlend): color;
  }
  else if (indexV != UINT16_MAX) //assuming UINT16_MAX is set explicitly (e.g. in XYZ)
    ppf(" dev sPC V:%d >= %d", indexV, NUM_LEDS_Max);
}

void Leds::setPixelColorPal(unsigned16 indexV, uint8_t palIndex, uint8_t palBri) {
  if (indexV < mappingTable.size()) {
    if (mappingTable[indexV].mapType == m_color) mappingTable[indexV].mapType = m_colorPal;
    switch (mappingTable[indexV].mapType) {
      case m_colorPal:
        mappingTable[indexV].palIndex = palIndex;
        mappingTable[indexV].palBri = palBri >> 2; // 8 bits to 6 bits
        break;
      default:
        setPixelColor(indexV, ColorFromPalette(palette, palIndex, palBri));
        break;
    }
  }
  else if (indexV < NUM_LEDS_Max) {//no projection
    uint16_t indexP = (projectionNr == p_Random)?random(fixture->nrOfLeds):indexV;
    fixture->ledsP[indexP] = fixture->pixelsToBlend[indexP]?blend(ColorFromPalette(palette, palIndex, palBri), fixture->ledsP[indexP], fixture->globalBlend): ColorFromPalette(palette, palIndex, palBri);
  }
  else if (indexV != UINT16_MAX) //assuming UINT16_MAX is set explicitly (e.g. in XYZ)
    ppf(" dev sPC V:%d >= %d", indexV, NUM_LEDS_Max);
}

CRGB Leds::getPixelColor(unsigned16 indexV) {
  if (indexV < mappingTable.size()) {
    switch (mappingTable[indexV].mapType) {
      case m_onePixel:
        return fixture->ledsP[mappingTable[indexV].indexP]; //any would do as they are all the same
        break;
      case m_morePixels:
        return fixture->ledsP[mappingTableIndexes[mappingTable[indexV].indexes][0]];
        break;
      case m_color:
        return CRGB(mappingTable[indexV].r << 2, mappingTable[indexV].g << 3, mappingTable[indexV].b << 5);
        break;
      default: // case m_colorPal:
        return ColorFromPalette(palette, mappingTable[indexV].palIndex, mappingTable[indexV].palBri << 2);
        break;
    }
  }
  else if (indexV < NUM_LEDS_Max) //no mapping
    return fixture->ledsP[indexV];
  else {
    ppf(" dev gPC N: %d >= %d", indexV, NUM_LEDS_Max);
    return CRGB::Black;
  }
}

void Leds::fadeToBlackBy(unsigned8 fadeBy) {
  if (projectionNr == p_None || projectionNr == p_Random || (fixture->listOfLeds.size() == 1)) {
    fastled_fadeToBlackBy(fixture->ledsP, fixture->nrOfLeds, fadeBy);
  } else {
    for (uint16_t index = 0; index < mappingTable.size(); index++) {
      CRGB color = getPixelColor(index);
      color.nscale8(255-fadeBy);
      setPixelColor(index, color);
    }
  }
}

void Leds::fill_solid(const struct CRGB& color) {
  if (projectionNr == p_None || projectionNr == p_Random || (fixture->listOfLeds.size() == 1)) {
    fastled_fill_solid(fixture->ledsP, fixture->nrOfLeds, color);
  } else {
    for (uint16_t index = 0; index < mappingTable.size(); index++)
      setPixelColor(index, color);
  }
}

void Leds::fill_rainbow(unsigned8 initialhue, unsigned8 deltahue) {
  if (projectionNr == p_None || projectionNr == p_Random || (fixture->listOfLeds.size() == 1)) {
    fastled_fill_rainbow(fixture->ledsP, fixture->nrOfLeds, initialhue, deltahue);
  } else {
    CHSV hsv;
    hsv.hue = initialhue;
    hsv.val = 255;
    hsv.sat = 240;

    for (uint16_t index = 0; index < mappingTable.size(); index++) {
      setPixelColor(index, hsv);
      hsv.hue += deltahue;
    }
  }
}

void PhysMap::addIndexP(Leds &leds, uint16_t indexP) {
  // ppf("addIndexP i:%d t:%d", indexP, mapType);
  switch (mapType) {
    case m_color:
    // case m_rgbColor:
      this->indexP = indexP;
      mapType = m_onePixel;
      break;
    case m_onePixel: {
      uint16_t oldIndexP = this->indexP;
      std::vector<uint16_t> newVector;
      newVector.push_back(oldIndexP);
      newVector.push_back(indexP);
      leds.mappingTableIndexes.push_back(newVector);
      indexes = leds.mappingTableIndexes.size() - 1;
      mapType = m_morePixels;
      break; }
    case m_morePixels:
      leds.mappingTableIndexes[indexes].push_back(indexP);
      // ppf(" more %d", mappingTableIndexes.size());
      break;
  }
  // ppf("\n");
}
