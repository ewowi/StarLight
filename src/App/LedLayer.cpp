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

#include "../Sys/SysModFiles.h"
#include "../Sys/SysStarJson.h"

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

  void LedsLayer::drawCharacter(unsigned char chr, int x, int16_t y, uint8_t font, CRGB col, uint16_t shiftPixel, uint16_t shiftChr) {
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

//load fixture json file, parse it and depending on the projection, create a mapping for it
void Fixture::projectAndMap() {
  unsigned long start = millis();
  char fileName[32] = "";

  if (files->seqNrToName(fileName, fixtureNr, "F_")) { // get the fixture.json

    if (strnstr(fileName, ".sc", sizeof(fileName)) != nullptr) {
      ppf("Live Fixture %s\n", fileName);

      strlcpy(web->lastFileUpdated, fileName, sizeof(web->lastFileUpdated));
      // ppf("script.onChange f:%d s:%s\n", fileNr, web->lastFileUpdated);
    }
    else {

      StarJson starJson(fileName); //open fileName for deserialize

      projectAndMapPre();

      //what to deserialize
      starJson.lookFor("width", (uint16_t *)&fixSize.x);
      starJson.lookFor("height", (uint16_t *)&fixSize.y);
      starJson.lookFor("depth", (uint16_t *)&fixSize.z);
      starJson.lookFor("nrOfLeds", &nrOfLeds);
      starJson.lookFor("pin", &currPin);

      //lookFor leds array and for each item in array call lambda to make a projection
      starJson.lookFor("leds", [this](std::vector<uint16_t> uint16CollectList) { //this will be called for each tuple of coordinates!

        if (uint16CollectList.size()>=1) { // process one pixel

          Coord3D pixel; //in mm !
          pixel.x = uint16CollectList[0];
          pixel.y = (uint16CollectList.size()>=2)?uint16CollectList[1]: 0;
          pixel.z = (uint16CollectList.size()>=3)?uint16CollectList[2]: 0;

          projectAndMapPixel(pixel);
        } //if 1D-3D pixel

        else { // end of leds array
          projectAndMapPin(currPin);
        }
      }); //starJson.lookFor("leds" (create the right type, otherwise crash)

      if (starJson.deserialize()) { //this will call above function parameter for each led
        projectAndMapPost();
      } // if deserialize
    }//Live Fixture
  } //if fileName
  else
    ppf("projectAndMap: Filename for fixture %d not found\n", fixtureNr);

  ppf("projectAndMap done %d ms\n", millis()-start);
}

void Fixture::projectAndMapPre() {
  ppf("projectAndMapPre\n");
  // reset leds
  uint8_t rowNr = 0;
  for (LedsLayer *leds: layers) {
    if (leds->doMap) {
      leds->projectAndMapPre();
    }
    rowNr++;
  }

  //deallocate all led pins
  if (doAllocPins) {
    // uint8_t pinNr = 0;
    // for (PinObject &pinObject: pinsM->pinObjects) {
    //   if (strncmp(pinObject.owner, "Leds", 5) == 0)
        pinsM->deallocatePin(UINT8_MAX, "Leds"); //deallocate all led pins
    //   pinNr++;
    // }
  }

  indexP = 0;
  prevIndexP = 0; //for allocPins
}

void Fixture::projectAndMapPixel(Coord3D pixel) {
  // ppf("led %d,%d,%d start %d,%d,%d end %d,%d,%d\n",x,y,z, startPos.x, startPos.y, startPos.z, endPos.x, endPos.y, endPos.z);

  if (indexP < NUM_LEDS_Max) {

    // //send pixel to ui ...
    // web->sendDataWs([this](AsyncWebSocketMessageBuffer * wsBuf) {
    //   byte* buffer;

    //   buffer = wsBuf->get();
    //   buffer[0] = 0; //,,,

    // }, 100, true);
    
    uint8_t rowNr = 0;
    for (LedsLayer *leds: layers) {
      leds->projectAndMapPixel(pixel, rowNr);
      rowNr++;
    } //for layers
  } //indexP < max
  else 
    ppf("dev post indexP too high %d>=%d or %d p:%d,%d,%d\n", indexP, nrOfLeds, NUM_LEDS_Max, pixel.x, pixel.y, pixel.z);

  indexP++; //also increase if no buffer created
}

void Fixture::projectAndMapPin(uint16_t pin) {
  if (doAllocPins) {
    //check if pin already allocated, if so, extend range in details
    PinObject pinObject = pinsM->pinObjects[pin];
    char details[32] = "";
    if (pinsM->isOwner(pin, "Leds")) { //if owner

      //merge already assigned leds with new assignleds in %d-%d
      char * after = strtok((char *)pinObject.details, "-");
      if (after != NULL ) {
        char * before;
        before = after;
        after = strtok(NULL, "-");
        uint16_t startLed = atoi(before);
        uint16_t nrOfLeds = atoi(after) - atoi(before) + 1;
        print->fFormat(details, sizeof(details), "%d-%d", min(prevIndexP, startLed), max((uint16_t)(indexP - 1), nrOfLeds)); //careful: LedModEffects:loop uses this to assign to FastLed
        ppf("pins extend leds %d: %s\n", pin, details);
        //tbd: more check

        strlcpy(pinsM->pinObjects[pin].details, details, sizeof(PinObject::details));  
        pinsM->pinsChanged = true;
      }
    }
    else {//allocate new pin
      //tbd: check if free
      print->fFormat(details, sizeof(details), "%d-%d", prevIndexP, indexP - 1); //careful: LedModEffects:loop uses this to assign to FastLed
      // ppf("allocatePin %d: %s\n", pin, details);
      pinsM->allocatePin(pin, "Leds", details);
    }

    prevIndexP = indexP;
  }
}

void Fixture::projectAndMapPost() {
  //after processing each led
  uint8_t rowNr = 0;

  for (LedsLayer *leds: layers) {
    if (leds->doMap) {
      leds->projectAndMapPost(rowNr);
    }
    rowNr++;
  } // leds

  ppf("projectAndMap fixture P:%dx%dx%d -> %d\n", fixSize.x, fixSize.y, fixSize.z, nrOfLeds);
  ppf("projectAndMap fixture.size = %d + l:(%d * %d) B\n", sizeof(Fixture) - NUM_LEDS_Max * sizeof(CRGB), NUM_LEDS_Max, sizeof(CRGB)); //56

  mdl->setValue("fixture", "size", fixSize);
  mdl->setValue("fixture", "count", nrOfLeds);

  //init pixelsToBlend
  for (uint16_t i=0; i<nrOfLeds; i++) {
    if (pixelsToBlend.size() < nrOfLeds)
      pixelsToBlend.push_back(false);
  }
  
  doMap = false;
}