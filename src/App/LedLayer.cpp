/*
   @title     StarLight
   @file      LedLayer.cpp
   @date      20241014
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "LedLayer.h"
#include "../Sys/SysModSystem.h"  //for sys->now
#ifdef STARBASE_USERMOD_MPU6050
  #include "../User/UserModMPU6050.h"
#endif

//convenience functions to call fastled functions out of the Leds namespace (there naming conflict)
void fastled_fadeToBlackBy(CRGB* leds, uint16_t num_leds, uint8_t fadeBy) {
  fadeToBlackBy(leds, num_leds, fadeBy);
}
void fastled_fill_solid( struct CRGB * targetArray, int numToFill, const struct CRGB& color) {
  fill_solid(targetArray, numToFill, color);
}
void fastled_fill_rainbow(struct CRGB * targetArray, int numToFill, uint8_t initialhue, uint8_t deltahue) {
  fill_rainbow(targetArray, numToFill, initialhue, deltahue);
}

void Effect::controls(LedsLayer &leds, JsonObject parentVar) {
    ui->initSelect(parentVar, "palette", 4, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onUI: {
        JsonArray options = ui->setOptions(var);
        options.add("CloudColors");
        options.add("LavaColors");
        options.add("OceanColors");
        options.add("ForestColors");
        options.add("RainbowColors");
        options.add("RainbowStripeColors");
        options.add("PartyColors");
        options.add("HeatColors");
        options.add("RandomColors");
        return true; }
      case onChange:
        switch (var["value"][rowNr].as<uint8_t>()) {
          case 0: leds.palette = CloudColors_p; break;
          case 1: leds.palette = LavaColors_p; break;
          case 2: leds.palette = OceanColors_p; break;
          case 3: leds.palette = ForestColors_p; break;
          case 4: leds.palette = RainbowColors_p; break;
          case 5: leds.palette = RainbowStripeColors_p; break;
          case 6: leds.palette = PartyColors_p; break;
          case 7: leds.palette = HeatColors_p; break;
          case 8: { //randomColors
            for (int i=0; i < sizeof(leds.palette.entries) / sizeof(CRGB); i++) {
              leds.palette[i] = CHSV(random8(), 255, 255); //take the max saturation, max brightness of the colorwheel
            }
            break;
          }
          default: leds.palette = PartyColors_p; //should never occur
        }
        return true;
      default: return false;
    }});
}


void LedsLayer::triggerMapping() {
    doMap = true; //specify which leds to remap
    fixture->doMap = true; //fixture will also be remapped
  }

uint16_t LedsLayer::XYZ(Coord3D pixel) {

  //using cached virtual class methods! (so no need for if projectionNr optimizations!)
  if (projection)
    (projection->*adjustXYZCached)(*this, pixel);

  return XYZUnprojected(pixel);
}

// maps the virtual led to the physical led(s) and assign a color to it
void LedsLayer::setPixelColor(uint16_t indexV, CRGB color) {
  if (indexV < mappingTable.size()) {
    switch (mappingTable[indexV].mapType) {
      case m_color:{
        mappingTable[indexV].rgb14 = ((min(color.r + 3, 255) >> 3) << 9) + 
                                     ((min(color.g + 3, 255) >> 3) << 4) + 
                                      (min(color.b + 7, 255) >> 4);
        break;
      }
      case m_onePixel: {
        uint16_t indexP = mappingTable[indexV].indexP;
        fixture->ledsP[indexP] = fixture->pixelsToBlend[indexP]?blend(color, fixture->ledsP[indexP], fixture->globalBlend):color;
        break; }
      case m_morePixels:
        if (mappingTable[indexV].indexes < mappingTableIndexes.size())
          for (uint16_t indexP: mappingTableIndexes[mappingTable[indexV].indexes]) {
            fixture->ledsP[indexP] = fixture->pixelsToBlend[indexP]?blend(color, fixture->ledsP[indexP], fixture->globalBlend): color;
          }
        // else
        //   ppf("dev setPixelColor2 i:%d m:%d s:%d\n", indexV, mappingTable[indexV].indexes, mappingTableIndexes.size());
        break;
    }
  }
  else if (indexV < NUM_LEDS_Max) //no projection
    fixture->ledsP[indexV] = fixture->pixelsToBlend[indexV]?blend(color, fixture->ledsP[indexV], fixture->globalBlend): color;
  else if (indexV != UINT16_MAX) //assuming UINT16_MAX is set explicitly (e.g. in XYZ)
    ppf(" dev sPC %d >= %d", indexV, NUM_LEDS_Max);
}

void LedsLayer::setPixelColorPal(uint16_t indexV, uint8_t palIndex, uint8_t palBri) {
  setPixelColor(indexV, ColorFromPalette(palette, palIndex, palBri));
}

void LedsLayer::blendPixelColor(uint16_t indexV, CRGB color, uint8_t blendAmount) {
  setPixelColor(indexV, blend(color, getPixelColor(indexV), blendAmount));
}

CRGB LedsLayer::getPixelColor(uint16_t indexV) {
  if (indexV < mappingTable.size()) {
    switch (mappingTable[indexV].mapType) {
      case m_onePixel:
        return fixture->ledsP[mappingTable[indexV].indexP]; 
        break;
      case m_morePixels:
        return fixture->ledsP[mappingTableIndexes[mappingTable[indexV].indexes][0]]; //any would do as they are all the same
        break;
      default: // m_color:
        return CRGB((mappingTable[indexV].rgb14 >> 9) << 3, 
                    (mappingTable[indexV].rgb14 >> 4) << 3, 
                     mappingTable[indexV].rgb14       << 4);
        break;
    }
  }
  else if (indexV < NUM_LEDS_Max) //no mapping
    return fixture->ledsP[indexV];
  else {
    ppf(" dev gPC %d >= %d", indexV, NUM_LEDS_Max);
    return CRGB::Black;
  }
}

void LedsLayer::fadeToBlackBy(uint8_t fadeBy) {
  if (!projection || (fixture->layers.size() == 1)) { //faster, else manual 
    fastled_fadeToBlackBy(fixture->ledsP, fixture->nrOfLeds, fadeBy);
  } else {
    for (uint16_t index = 0; index < mappingTable.size(); index++) {
      CRGB color = getPixelColor(index);
      color.nscale8(255-fadeBy);
      setPixelColor(index, color);
    }
  }
}

void LedsLayer::fill_solid(const struct CRGB& color) {
  if (!projection || (fixture->layers.size() == 1)) { //faster, else manual 
    fastled_fill_solid(fixture->ledsP, fixture->nrOfLeds, color);
  } else {
    for (uint16_t index = 0; index < mappingTable.size(); index++)
      setPixelColor(index, color);
  }
}

void LedsLayer::fill_rainbow(uint8_t initialhue, uint8_t deltahue) {
  if (!projection || (fixture->layers.size() == 1)) { //faster, else manual 
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

void PhysMap::addIndexP(LedsLayer &leds, uint16_t indexP) {
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

  void LedsLayer::projectAndMapPre() {
    if (doMap) {
      fill_solid(CRGB::Black);

      ppf("projectAndMap clear leds[x] effect:%s pro:%s\n", effect?effect->name():"None", projection?projection->name():"None");
      size = Coord3D{0,0,0};
      //vectors really gone now?
      for (std::vector<uint16_t> mappingTableIndex: mappingTableIndexes) {
        mappingTableIndex.clear();
      }
      mappingTableIndexes.clear();
      mappingTable.clear();
    }
  }

  void LedsLayer::projectAndMapPixel(Coord3D pixel, uint8_t rowNr) {
    if (projection && doMap) { //only real projections: add pixel in leds mappingtable

      //set start and endPos between bounderies of fixture
      Coord3D startPosAdjusted = (startPos).minimum(fixture->fixSize - Coord3D{1,1,1}) * 10;
      Coord3D endPosAdjusted = (endPos).minimum(fixture->fixSize - Coord3D{1,1,1}) * 10;
      Coord3D midPosAdjusted = (midPos).minimum(fixture->fixSize - Coord3D{1,1,1}); //not * 10

      // mdl->setValue("start", startPosAdjusted/10, rowNr); //rowNr
      // mdl->setValue("end", endPosAdjusted/10, rowNr); //rowNr

      if (pixel >= startPosAdjusted && pixel <= endPosAdjusted ) { //if pixel between start and end pos

        Coord3D pixelAdjusted = (pixel - startPosAdjusted)/10; //pixelRelative to startPos in cm

        Coord3D sizeAdjusted = (endPosAdjusted - startPosAdjusted)/10 + Coord3D{1,1,1}; // in cm

        // 0 to 3D depending on start and endpos (e.g. to display ScrollingText on one side of a cube)
        projectionDimension = 0;
        if (sizeAdjusted.x > 1) projectionDimension++;
        if (sizeAdjusted.y > 1) projectionDimension++;
        if (sizeAdjusted.z > 1) projectionDimension++;

        // setupCached = &Projection::setup;
        // adjustXYZCached = &Projection::adjustXYZ;

        mdl->getValueRowNr = rowNr; //run projection functions in the right rowNr context

        uint16_t indexV = XYZUnprojected(pixelAdjusted); //default

        // Setup changes leds.size, mapped, indexV
        if (projection) (projection->*setupCached)(*this, sizeAdjusted, pixelAdjusted, midPosAdjusted, indexV);

        if (size == Coord3D{0,0,0}) size = sizeAdjusted; //first, not assigned in setupCached
        nrOfLeds = size.x * size.y * size.z;

        if (indexV != UINT16_MAX) {
          if (indexV >= nrOfLeds || indexV >= NUM_VLEDS_Max)
            ppf("dev leds[%d] pre indexV too high %d>=%d or %d (m:%d p:%d) p:%d,%d,%d s:%d,%d,%d\n", rowNr, indexV, nrOfLeds, NUM_VLEDS_Max, mappingTable.size(), fixture->indexP, pixel.x, pixel.y, pixel.z, size.x, size.y, size.z);
          else {

            //create new physMaps if needed
            if (indexV >= mappingTable.size()) {
              for (size_t i = mappingTable.size(); i <= indexV; i++) {
                // ppf("mapping %d,%d,%d add physMap before %d %d\n", pixel.y, pixel.y, pixel.z, indexV, mappingTable.size());
                mappingTable.push_back(PhysMap());
              }
            }

            mappingTable[indexV].addIndexP(*this, fixture->indexP);
            // ppf("mapping b:%d t:%d V:%d\n", indexV, indexP, mappingTable.size());
          } //indexV not too high
        } //indexV

        mdl->getValueRowNr = UINT8_MAX; // end of run projection functions in the right rowNr context

      } //if x,y,z between start and endpos
    } //if doMap
  } //projectAndMapPixel

  void LedsLayer::projectAndMapPost(uint8_t rowNr) {
    if (doMap) {
      ppf("projectAndMap post leds[%d] effect:%s pro:%s\n", rowNr, effect?effect->name():"None", projection?projection->name():"None");

      uint16_t nrOfLogical = 0;
      uint16_t nrOfPhysical = 0;
      uint16_t nrOfPhysicalM = 0;
      uint16_t nrOfColor = 0;

      if (!projection) { //projection is none

        //defaults
        size = fixture->fixSize;
        nrOfLeds = nrOfLeds;
        nrOfPhysical = nrOfLeds;

      } else {

        if (mappingTable.size() < size.x * size.y * size.z)
          ppf("mapping add extra physMap %d to %d size: %d,%d,%d\n", mappingTable.size(), size.x * size.y * size.z, size.x, size.y, size.z);
        for (size_t i = mappingTable.size(); i < size.x * size.y * size.z; i++) {
          mappingTable.push_back(PhysMap());
        }

        nrOfLeds = mappingTable.size();

        //debug info + summary values
        for (PhysMap &map:mappingTable) {
          switch (map.mapType) {
            case m_color:
              nrOfColor++;
              break;
            case m_onePixel:
              // ppf("ledV %d mapping =1: #ledsP : %d\n", nrOfLogical, map.indexP);
              nrOfPhysical++;
              break;
            case m_morePixels:
              // ppf("ledV %d mapping >1: #ledsP :", nrOfLogical);
              
              for (uint16_t indexP: mappingTableIndexes[map.indexes]) {
                // ppf(" %d", indexP);
                nrOfPhysicalM++;
              }
              // ppf("\n");
              break;
          }
          nrOfLogical++;
          // else
          //   ppf("ledV %d no mapping\n", x);
        }
      }

      ppf("projectAndMap leds[%d] V:%d x %d x %d -> %d (v:%d - p:%d pm:%d c:%d)\n", rowNr, size.x, size.y, size.z, nrOfLeds, nrOfLogical, nrOfPhysical, nrOfPhysicalM, nrOfColor);

      char buf[32];
      print->fFormat(buf, sizeof(buf), "%d x %d x %d -> %d", size.x, size.y, size.z, nrOfLeds);
      mdl->setValue("layers", "size", JsonString(buf, JsonString::Copied), rowNr);

      ppf("projectAndMap leds[%d].size = %d + m:(%d * %d) + d:(%d + %d) B\n", rowNr, sizeof(LedsLayer), mappingTable.size(), sizeof(PhysMap), effectData.bytesAllocated, projectionData.bytesAllocated); //44 -> 164

      doMap = false;
    } //doMap

  }
