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
#include "LedModFixture.h"

#include "../Sys/SysModSystem.h"  //for sys->now
#include "../Sys/SysModFiles.h"

#ifdef STARBASE_USERMOD_MPU6050
  #include "../User/UserModMPU6050.h"
#endif

#include "../misc/font/console_font_4x6.h"
#include "../misc/font/console_font_5x8.h"
#include "../misc/font/console_font_5x12.h"
#include "../misc/font/console_font_6x8.h"
#include "../misc/font/console_font_7x9.h"

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

void Effect::setup(LedsLayer &leds, JsonObject parentVar) {
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
      // std::vector<uint16_t> newVector;
      // newVector.push_back(oldIndexP);
      // newVector.push_back(indexP);
      leds.mappingTableIndexesSizeUsed++;
      if (leds.mappingTableIndexes.size() < leds.mappingTableIndexesSizeUsed)
        leds.mappingTableIndexes.push_back({oldIndexP, indexP});
      else
        leds.mappingTableIndexes[leds.mappingTableIndexesSizeUsed-1] = {oldIndexP, indexP};

      indexes = leds.mappingTableIndexesSizeUsed - 1; //array position
      mapType = m_morePixels;
      break; }
    case m_morePixels:
      leds.mappingTableIndexes[indexes].push_back(indexP);
      // ppf(" more %d", mappingTableIndexes.size());
      break;
  }
  // ppf("\n");
}

void LedsLayer::triggerMapping() {
    doMap = true; //specify which leds to remap
    fix->mappingStatus = 1; //start mapping
}

bool LedsLayer::inBounds(int x, int y, int z) {
  return x >= 0 && x < size.x && y >= 0 && y < size.y && z >= 0 && z < size.z;
}
bool LedsLayer::inBounds(Coord3D pos) {
  return pos >= 0 && pos < size;
}

int LedsLayer::XYZUnprojected(Coord3D pixel) {
  return pixel.x + pixel.y * size.x + pixel.z * size.x * size.y;
}

int LedsLayer::XYZ(int x, int y, int z) {
  return XYZ({x, y, z});
}

int LedsLayer::XYZ(Coord3D pixel) {

  //using cached virtual class methods! (so no need for if projectionNr optimizations!)
  if (projection) {
    projectionData.begin();
    (projection->*XYZCached)(*this, pixel);
  }

  return XYZUnprojected(pixel);
}

// maps the virtual led to the physical led(s) and assign a color to it
void LedsLayer::setPixelColor(int indexV, CRGB color) {
  if (indexV < 0)
    return;
  else if (indexV < mappingTableSizeUsed) {
    switch (mappingTable[indexV].mapType) {
      case m_color:{
        mappingTable[indexV].rgb14 = ((min(color.r + 3, 255) >> 3) << 9) + 
                                     ((min(color.g + 3, 255) >> 3) << 4) + 
                                      (min(color.b + 7, 255) >> 4);
        break;
      }
      case m_onePixel: {
        uint16_t indexP = mappingTable[indexV].indexP;
        fix->ledsP[indexP] = fix->pixelsToBlend[indexP]?blend(color, fix->ledsP[indexP], fix->globalBlend):color;
        break; }
      case m_morePixels:
        if (mappingTable[indexV].indexes < mappingTableIndexes.size())
          for (uint16_t indexP: mappingTableIndexes[mappingTable[indexV].indexes]) {
            fix->ledsP[indexP] = fix->pixelsToBlend[indexP]?blend(color, fix->ledsP[indexP], fix->globalBlend): color;
          }
        else
          ppf("dev setPixelColor i:%d m:%d s:%d\n", indexV, mappingTable[indexV].indexes, mappingTableIndexes.size());
        break;
    }
  }
  else if (indexV < NUM_LEDS_Max) //no projection
    fix->ledsP[indexV] = fix->pixelsToBlend[indexV]?blend(color, fix->ledsP[indexV], fix->globalBlend): color;
  // some operations will go out of bounds e.g. VUMeter, uncomment below lines if you wanna test on a specific effect
  // else //if (indexV != UINT16_MAX) //assuming UINT16_MAX is set explicitly (e.g. in XYZ)
  //   ppf(" dev sPC %d >= %d", indexV, NUM_LEDS_Max);
}

void LedsLayer::setPixelColorPal(int indexV, uint8_t palIndex, uint8_t palBri) {
  setPixelColor(indexV, ColorFromPalette(palette, palIndex, palBri));
}

void LedsLayer::blendPixelColor(int indexV, CRGB color, uint8_t blendAmount) {
  setPixelColor(indexV, blend(color, getPixelColor(indexV), blendAmount));
}

CRGB LedsLayer::getPixelColor(int indexV) {
  if (indexV < 0)
    return CRGB::Black;
  else if (indexV < mappingTableSizeUsed) {
    switch (mappingTable[indexV].mapType) {
      case m_onePixel:
        return fix->ledsP[mappingTable[indexV].indexP]; 
        break;
      case m_morePixels:
        return fix->ledsP[mappingTableIndexes[mappingTable[indexV].indexes][0]]; //any would do as they are all the same
        break;
      default: // m_color:
        return CRGB((mappingTable[indexV].rgb14 >> 9) << 3, 
                    (mappingTable[indexV].rgb14 >> 4) << 3, 
                     mappingTable[indexV].rgb14       << 4);
        break;
    }
  }
  else if (indexV < NUM_LEDS_Max) //no mapping
    return fix->ledsP[indexV];
  else {
    // some operations will go out of bounds e.g. VUMeter, uncomment below lines if you wanna test on a specific effect
    // ppf(" dev gPC %d >= %d", indexV, NUM_LEDS_Max);
    return CRGB::Black;
  }
}

void LedsLayer::fadeToBlackBy(uint8_t fadeBy) {
  if (!projection || (fix->layers.size() == 1)) { //faster, else manual 
    fastled_fadeToBlackBy(fix->ledsP, fix->nrOfLeds, fadeBy);
  } else {
    for (uint16_t index = 0; index < mappingTableSizeUsed; index++) {
      CRGB color = getPixelColor(index);
      color.nscale8(255-fadeBy);
      setPixelColor(index, color);
    }
  }
}

void LedsLayer::fill_solid(const struct CRGB& color) {
  if (!projection || (fix->layers.size() == 1)) { //faster, else manual 
    fastled_fill_solid(fix->ledsP, fix->nrOfLeds, color);
  } else {
    for (uint16_t index = 0; index < mappingTableSizeUsed; index++)
      setPixelColor(index, color);
  }
}

void LedsLayer::fill_rainbow(uint8_t initialhue, uint8_t deltahue) {
  if (!projection || (fix->layers.size() == 1)) { //faster, else manual 
    fastled_fill_rainbow(fix->ledsP, fix->nrOfLeds, initialhue, deltahue);
  } else {
    CHSV hsv;
    hsv.hue = initialhue;
    hsv.val = 255;
    hsv.sat = 240;

    for (uint16_t index = 0; index < mappingTableSizeUsed; index++) {
      setPixelColor(index, hsv);
      hsv.hue += deltahue;
    }
  }
}

  void LedsLayer::drawCharacter(unsigned char chr, int x, int y, uint8_t font, CRGB col, uint16_t shiftPixel, uint16_t shiftChr) {
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

  void LedsLayer::addPixelsPre(uint8_t rowNr) {
    if (doMap) {
      fill_solid(CRGB::Black);

      ppf("addPixelsPre clear leds[x] effect:%s pro:%s\n", effect?effect->name():"None", projection?projection->name():"None");
      size = Coord3D{0,0,0};
      //vectors really gone now?
      for (std::vector<uint16_t> mappingTableIndex: mappingTableIndexes) {
        mappingTableIndex.clear();
      }
      mappingTableIndexesSizeUsed = 0; //do not clear mappingTableIndexes, reuse it

      for (size_t i = 0; i < mappingTable.size(); i++) {
        mappingTable[i] = PhysMap();
      }
      mappingTableSizeUsed = 0;

      //new fix size: update leds
      start = mdl->getValue("layers", "start", rowNr).as<Coord3D>().minimum(fix->fixSize - Coord3D{1,1,1});
      middle = mdl->getValue("layers", "middle", rowNr).as<Coord3D>().minimum(fix->fixSize - Coord3D{1,1,1});
      end = mdl->getValue("layers", "end", rowNr).as<Coord3D>().minimum(fix->fixSize - Coord3D{1,1,1});

      if (middle == Coord3D{0,0,0}) 
        middle = fix->fixSize / 2; //so pinWheel shows a midpoint
      if (end == Coord3D{0,0,0}) 
        end = fix->fixSize - Coord3D{1,1,1};

      size = (end - start) + Coord3D{1,1,1};

      // 0 to 3D depending on start and end (e.g. to display ScrollingText on one side of a cube)
      projectionDimension = 0;
      if (size.x > 1) projectionDimension++;
      if (size.y > 1) projectionDimension++;
      if (size.z > 1) projectionDimension++;

      if (projection) {
        mdl->getValueRowNr = rowNr; //run projection functions in the right rowNr context
        projectionData.begin();
        (projection->*addPixelsPreCached)(*this);
        mdl->getValueRowNr = UINT8_MAX; // end of run projection functions in the right rowNr context
      }

      nrOfLeds = size.x * size.y * size.z;
    }
  }

  void LedsLayer::addPixel(Coord3D pixel, uint8_t rowNr) {
    if (projection && doMap) { //only real projections: add pixel in leds mappingTable
      // ppf("addPixel %d %d", pixel, fix->factor);
      if (pixel >= start * fix->factor && pixel <= end * fix->factor ) { //if pixel between start and end pos

        Coord3D pixelAdjusted = pixel / fix->factor - start; //pixel also rounded to a 1x1 grid space

        // addPixelCached = &Projection::addPixel;
        // XYZCached = &Projection::XYZ;

        uint16_t indexV = XYZUnprojected(pixelAdjusted); //default

        // Setup changes leds.size, mapped, indexV
        mdl->getValueRowNr = rowNr; //run projection functions in the right rowNr context
        projectionData.begin();
        (projection->*addPixelCached)(*this, pixelAdjusted, indexV);
        mdl->getValueRowNr = UINT8_MAX; // end of run projection functions in the right rowNr context

        if (indexV != UINT16_MAX) {
          if (indexV >= nrOfLeds || indexV >= NUM_VLEDS_Max) {
            // ppf("dev leds[%d] pre indexV too high %d>=%d or %d (m:%d p:%d) p:%d,%d,%d s:%d,%d,%d\n", rowNr, indexV, nrOfLeds, NUM_VLEDS_Max, mappingTableSizeUsed, fix->indexP, pixel.x, pixel.y, pixel.z, size.x, size.y, size.z);
          }
          else {
            //create new physMaps if needed
            if (indexV >= mappingTable.size()) {
              for (size_t i = mappingTable.size(); i <= indexV; i++) {
                // ppf("mapping %d,%d,%d add physMap before %d %d\n", pixel.y, pixel.y, pixel.z, indexV, mappingTable.size());
                mappingTable.push_back(PhysMap());
                // mappingTableIndexesSizeUsed++;
              }
            }

            if (indexV >= mappingTableSizeUsed) mappingTableSizeUsed = indexV + 1;

            mappingTable[indexV].addIndexP(*this, fix->indexP);
            // ppf("mapping b:%d t:%d V:%d\n", indexV, indexP, mappingTableSizeUsed);
          } //indexV not too high
        } //indexV

      } //if x,y,z between start and end
    } //if projection && doMap
  } //addPixel

  void LedsLayer::addPixelsPost(uint8_t rowNr) {
    if (doMap) {
      ppf("addPixelsPost leds[%d] effect:%s pro:%s\n", rowNr, effect?effect->name():"None", projection?projection->name():"None");

      uint16_t nrOfLogical = 0;
      uint16_t nrOfPhysical = 0;
      uint16_t nrOfPhysicalM = 0;
      uint16_t nrOfColor = 0;

      if (!projection) { //projection is none

        //defaults
        size = fix->fixSize;
        nrOfLeds = fix->nrOfLeds;
        nrOfPhysical = fix->nrOfLeds;

      } else {

        if (mappingTable.size() < size.x * size.y * size.z)
          ppf("addPixelsPost add extra physMap %d to %d size: %d,%d,%d\n", mappingTableSizeUsed, size.x * size.y * size.z, size.x, size.y, size.z);
        for (size_t i = mappingTable.size(); i < size.x * size.y * size.z; i++) {
          mappingTable.push_back(PhysMap());
          mappingTableSizeUsed++;
        }

        nrOfLeds = mappingTableSizeUsed;

        //debug info + summary values
        for (size_t i = 0; i< mappingTableSizeUsed; i++) {
          PhysMap &map = mappingTable[i];
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

      ppf("addPixelsPost leds[%d] V:%d x %d x %d -> %d (v:%d - p:%d pm:%d of %d c:%d)\n", rowNr, size.x, size.y, size.z, nrOfLeds, nrOfLogical, nrOfPhysical, nrOfPhysicalM, mappingTableIndexesSizeUsed, nrOfColor);

      char buf[32];
      print->fFormat(buf, sizeof(buf), "%d x %d x %d -> %d", size.x, size.y, size.z, nrOfLeds);
      mdl->setValue("layers", "size", JsonString(buf, JsonString::Copied), rowNr);

      ppf("addPixelsPost leds[%d].size = so:%d + m:(%d of %d) * %d + d:(%d + %d) B\n", rowNr, sizeof(LedsLayer), mappingTableSizeUsed, mappingTable.size(), sizeof(PhysMap), effectData.bytesAllocated, projectionData.bytesAllocated); //44 -> 164

      doMap = false;
    } //doMap

  }