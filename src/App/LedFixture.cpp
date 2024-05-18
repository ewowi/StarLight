/*
   @title     StarLeds
   @file      LedFixture.cpp
   @date      20240228
   @repo      https://github.com/MoonModules/StarLeds
   @Authors   https://github.com/MoonModules/StarLeds/commits/main
   @Copyright © 2024 Github StarLeds Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "LedFixture.h"

#include "../Sys/SysModFiles.h"
#include "../Sys/SysStarJson.h"
#include "../Sys/SysModPins.h"


//load fixture json file, parse it and depending on the projection, create a mapping for it
void Fixture::projectAndMap() {
  char fileName[32] = "";

  if (files->seqNrToName(fileName, fixtureNr)) { // get the fixture.json
    StarJson starJson(fileName); //open fileName for deserialize

    // reset leds
    stackUnsigned8 rowNr = 0;
    for (Leds *leds: projections) {
      if (leds->doMap) {
        leds->fill_solid(CRGB::Black, true); //no blend

        ppf("projectAndMap clear leds[%d] fx:%d pro:%d\n", rowNr, leds->fx, leds->projectionNr);
        leds->size = Coord3D{0,0,0};
        //vectors really gone now?
        for (PhysMap &map:leds->mappingTable) {
          if (map.indexes) {
            map.indexes->clear();
            delete map.indexes;
          }
        }
        leds->mappingTable.clear();
        leds->sharedData.clear();
      }
      rowNr++;
    }

    //deallocate all led pins
    if (doAllocPins) {
      stackUnsigned8 pinNr = 0;
      for (PinObject &pinObject: pins->pinObjects) {
        if (strcmp(pinObject.owner, "Leds") == 0)
          pins->deallocatePin(pinNr, "Leds");
        pinNr++;
      }
    }

    stackUnsigned16 indexP = 0;
    stackUnsigned16 prevIndexP = 0;
    unsigned16 currPin; //lookFor needs u16

    //what to deserialize
    starJson.lookFor("width", (unsigned16 *)&fixSize.x);
    starJson.lookFor("height", (unsigned16 *)&fixSize.y);
    starJson.lookFor("depth", (unsigned16 *)&fixSize.z);
    starJson.lookFor("nrOfLeds", &nrOfLeds);
    starJson.lookFor("pin", &currPin);

    //lookFor leds array and for each item in array call lambdo to make a projection
    starJson.lookFor("leds", [this, &prevIndexP, &indexP, &currPin](std::vector<unsigned16> uint16CollectList) { //this will be called for each tuple of coordinates!

      if (uint16CollectList.size()>=1) { // process one pixel

        Coord3D pixel; //in mm !
        pixel.x = uint16CollectList[0];
        pixel.y = (uint16CollectList.size()>=2)?uint16CollectList[1]: 0;
        pixel.z = (uint16CollectList.size()>=3)?uint16CollectList[2]: 0;

        // ppf("led %d,%d,%d start %d,%d,%d end %d,%d,%d\n",x,y,z, startPos.x, startPos.y, startPos.z, endPos.x, endPos.y, endPos.z);

        stackUnsigned8 rowNr = 0;
        for (Leds *leds: projections) {

          if (leds->projectionNr != p_Random && leds->projectionNr != p_None)
          if (leds->doMap) { //add pixel in leds mappingtable

            //set start and endPos between bounderies of fixture
            Coord3D startPosAdjusted = (leds->startPos).minimum(fixSize - Coord3D{1,1,1}) * 10;
            Coord3D endPosAdjusted = (leds->endPos).minimum(fixSize - Coord3D{1,1,1}) * 10;

            // mdl->setValue("fxStart", startPosAdjusted/10, rowNr); //rowNr
            // mdl->setValue("fxEnd", endPosAdjusted/10, rowNr); //rowNr

            if (pixel >= startPosAdjusted && pixel <= endPosAdjusted ) { //if pixel between start and end pos

              Coord3D pixelAdjusted = (pixel - startPosAdjusted)/10;

              Coord3D sizeAdjusted = (endPosAdjusted - startPosAdjusted)/10 + Coord3D{1,1,1};

              // 0 to 3D depending on start and endpos (e.g. to display ScrollingText on one side of a cube)
              stackUnsigned8 projectionDimension = 0;
              if (sizeAdjusted.x > 1) projectionDimension++;
              if (sizeAdjusted.y > 1) projectionDimension++;
              if (sizeAdjusted.z > 1) projectionDimension++;

              Coord3D proCenter;
              if (leds->projectionNr == p_DistanceFromPoint || leds->projectionNr == p_Preset1) {
                proCenter = mdl->getValue("proCenter", rowNr);
              }
              else 
                proCenter = Coord3D{0,0,0};

              Coord3D mirrors = Coord3D{0,0,0}; // no mirrors
              if (leds->projectionNr == p_Multiply || leds->projectionNr == p_Preset1) {
                Coord3D proMulti;
                proMulti = mdl->getValue("proMulti", rowNr);
                //promultu can be 0,0,0 but /= protects from /div0
                sizeAdjusted /= proMulti; sizeAdjusted = sizeAdjusted.maximum(Coord3D{1,1,1}); //size min 1,1,1
                proCenter /= proMulti;
                mirrors = pixelAdjusted / sizeAdjusted; //place the pixel in the right quadrant
                pixelAdjusted = pixelAdjusted%sizeAdjusted; // pixel % size
                // ppf("Multiply %d,%d,%d\n", leds->size.x, leds->size.y, leds->size.z);
              }
              bool mirror = mdl->getValue("mirror", rowNr);

              // ppf("projectionNr p:%d f:%d s:%d, %d-%d-%d %d-%d-%d %d-%d-%d\n", projectionNr, effectDimension, projectionDimension, x, y, z, uint16CollectList[0], uint16CollectList[1], uint16CollectList[2], size.x, size.y, size.z);

              if (leds->size == Coord3D{0,0,0}) { // first
                ppf("projectAndMap first leds[%d] size:%d,%d,%d s:%d,%d,%d e:%d,%d,%d\n", rowNr, sizeAdjusted.x, sizeAdjusted.y, sizeAdjusted.z, startPosAdjusted.x, startPosAdjusted.y, startPosAdjusted.z, endPosAdjusted.x, endPosAdjusted.y, endPosAdjusted.z);
              }

              //calculate the indexV to add to current physical led to
              stackUnsigned16 indexV = UINT16_MAX;

              Coord3D mapped;
              switch (leds->effectDimension) {
                case _1D: //effectDimension 1DxD
                  if (leds->size == Coord3D{0,0,0}) { // first
                    leds->size.x = sizeAdjusted.distance(proCenter);
                    leds->size.y = 1;
                    leds->size.z = 1;
                  }

                  mapped = pixelAdjusted;

                  // if mirrored find the indexV of the mirrored pixel
                  if (mirror) {
                    if (mirrors.x %2 != 0) mapped.x = sizeAdjusted.x - 1 - mapped.x;
                    if (mirrors.y %2 != 0) mapped.y = sizeAdjusted.y - 1 - mapped.y;
                    if (mirrors.z %2 != 0) mapped.z = sizeAdjusted.z - 1 - mapped.z;
                  }

                  mapped.x = mapped.distance(proCenter);
                  mapped.y = 0;
                  mapped.z = 0;

                  indexV = leds->XYZNoSpin(mapped);
                  break;
                case _2D: //effectDimension
                  switch(projectionDimension) {
                    case _1D: //2D1D
                      if (leds->size == Coord3D{0,0,0}) { // first
                        leds->size.x = sqrt(sizeAdjusted.x * sizeAdjusted.y * sizeAdjusted.z); //only one is > 1, square root
                        leds->size.y = sizeAdjusted.x * sizeAdjusted.y * sizeAdjusted.z / leds->size.x;
                        leds->size.z = 1;
                      }
                      mapped.x = (pixelAdjusted.x + pixelAdjusted.y + pixelAdjusted.z) % leds->size.x; // only one > 0
                      mapped.y = (pixelAdjusted.x + pixelAdjusted.y + pixelAdjusted.z) / leds->size.x; // all rows next to each other
                      mapped.z = 0;
                      break;
                    case _2D: //2D2D
                      //find the 2 axis 
                      if (leds->size == Coord3D{0,0,0}) { // first
                        if (sizeAdjusted.x > 1) {
                          leds->size.x = sizeAdjusted.x;
                          if (sizeAdjusted.y > 1) leds->size.y = sizeAdjusted.y; else leds->size.y = sizeAdjusted.z;
                        } else {
                          leds->size.x = sizeAdjusted.y;
                          leds->size.y = sizeAdjusted.z;
                        }
                        leds->size.z = 1;
                      }

                      if (sizeAdjusted.x > 1) {
                        mapped.x = pixelAdjusted.x;
                        if (sizeAdjusted.y > 1) mapped.y = pixelAdjusted.y; else mapped.y = pixelAdjusted.z;
                      } else {
                        mapped.x = pixelAdjusted.y;
                        mapped.y = pixelAdjusted.z;
                      }
                      mapped.z = 0;

                      // ppf("2Dto2D %d-%d p:%d,%d,%d m:%d,%d,%d\n", indexV, indexP, pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z, mapped.x, mapped.y, mapped.z
                      break;
                    case _3D: //2D3D
                      if (leds->size == Coord3D{0,0,0}) { // first
                        leds->size.x = sizeAdjusted.x + sizeAdjusted.y / 2;
                        leds->size.y = sizeAdjusted.y / 2 + sizeAdjusted.z;
                        leds->size.z = 1;
                      }
                      mapped.x = pixelAdjusted.x + pixelAdjusted.y / 2;
                      mapped.y = pixelAdjusted.y / 2 + pixelAdjusted.z;
                      mapped.z = 0;

                      // ppf("2D to 3D indexV %d %d\n", indexV, size.x);
                      break;
                  }

                  if (mirror) {
                    if (mirrors.x %2 != 0) mapped.x = sizeAdjusted.x - 1 - mapped.x;
                    if (mirrors.y %2 != 0) mapped.y = sizeAdjusted.y - 1 - mapped.y;
                    if (mirrors.z %2 != 0) mapped.z = sizeAdjusted.z - 1 - mapped.z;
                  }
                  indexV = leds->XYZNoSpin(mapped);
                  break;
                case _3D: //effectDimension
                  mapped = pixelAdjusted;
                  
                  switch(projectionDimension) {
                    case _1D:
                      if (leds->size == Coord3D{0,0,0}) { // first
                        leds->size.x = std::pow(sizeAdjusted.x * sizeAdjusted.y * sizeAdjusted.z, 1/3.); //only one is > 1, cube root
                        leds->size.y = leds->size.x;
                        leds->size.z = leds->size.x;
                      }
                      break;
                    case _2D:
                      if (leds->size == Coord3D{0,0,0}) { // first
                        leds->size.x = sizeAdjusted.x; //2 of the 3 sizes are > 1, so one dimension of the effect is 1
                        leds->size.y = sizeAdjusted.y;
                        leds->size.z = sizeAdjusted.z;
                      }
                      break;
                    case _3D:
                      if (leds->size == Coord3D{0,0,0}) { // first
                        leds->size.x = sizeAdjusted.x;
                        leds->size.y = sizeAdjusted.y;
                        leds->size.z = sizeAdjusted.z;
                      }
                      break;
                  }
                  if (mirror) {
                    if (mirrors.x %2 != 0) mapped.x = sizeAdjusted.x - 1 - mapped.x;
                    if (mirrors.y %2 != 0) mapped.y = sizeAdjusted.y - 1 - mapped.y;
                    if (mirrors.z %2 != 0) mapped.z = sizeAdjusted.z - 1 - mapped.z;
                  }
                  indexV = leds->XYZNoSpin(mapped);
                  
                  break; //effectDimension _3D
              } //effectDimension
              leds->nrOfLeds = leds->size.x * leds->size.y * leds->size.z;

              if (indexV != UINT16_MAX) {
                if (indexV >= leds->nrOfLeds || indexV >= NUM_LEDS_Max) {
                  ppf("dev pre [%d] indexV too high %d>=%d or %d (m:%d p:%d) p:%d,%d,%d s:%d,%d,%d\n", rowNr, indexV, leds->nrOfLeds, NUM_LEDS_Max, leds->mappingTable.size(), indexP, pixel.x, pixel.y, pixel.z, leds->size.x, leds->size.y, leds->size.z);
                }
                else {
                  Trigo trigo(leds->size.x-1); // 8 bits trigo with period leds->size.x-1 (currentl Float trigo as same performance)
                  //post processing: 
                  switch(leds->projectionNr) {
                  case p_DistanceFromPoint:
                    switch (leds->effectDimension) {
                    case _2D: 
                      switch (projectionDimension) {
                      case _2D: //2D2D: inverse mapping
                        float minDistance = 10;
                        // ppf("checking indexV %d\n", indexV);
                        for (forUnsigned16 x=0; x<leds->size.x && minDistance > 0.5f; x++) {
                          // float xFactor = x * TWO_PI / (float)(leds->size.x-1); //between 0 .. 2PI

                          float xNew = trigo.sin(leds->size.x, x);
                          float yNew = trigo.cos(leds->size.y, x);

                          for (forUnsigned16 y=0; y<leds->size.y && minDistance > 0.5f; y++) {

                            // float yFactor = (leds->size.y-1.0f-y) / (leds->size.y-1.0f); // between 1 .. 0
                            float yFactor = 1 - y / (leds->size.y-1.0f); // between 1 .. 0

                            float x2New = round((yFactor * xNew + leds->size.x) / 2.0f); // 0 .. size.x
                            float y2New = round((yFactor * yNew + leds->size.y) / 2.0f); //  0 .. size.y

                            // ppf(" %d,%d->%f,%f->%f,%f", x, y, sinf(x * TWO_PI / (float)(size.x-1)), cosf(x * TWO_PI / (float)(size.x-1)), xNew, yNew);

                            //this should work (better) but needs more testing
                            // float distance = abs(indexV - xNew - yNew * size.x);
                            // if (distance < minDistance) {
                            //   minDistance = distance;
                            //   indexV = x+y*size.x;
                            // }

                            // if the new XY i
                            if (indexV == leds->XY(x2New, y2New)) { //(unsigned8)xNew + (unsigned8)yNew * size.x) {
                              // ppf("  found one %d => %d=%d+%d*%d (%f+%f*%d) [%f]\n", indexV, x+y*size.x, x,y, size.x, xNew, yNew, size.x, distance);
                              indexV = leds->XY(x, y);

                              if (indexV%10 == 0) ppf("."); //show some progress as this projection is slow (Need S007 to optimize ;-)
                                                          
                              minDistance = 0.0f; // stop looking further
                            }
                          }
                        }
                        if (minDistance > 0.5f) indexV = UINT16_MAX;
                        break;
                      }
                      break;
                    }
                    break;
                  }

                  if (indexV != UINT16_MAX) { //can be nulled by inverse mapping 
                    //add physical tables if not present
                    if (indexV >= leds->nrOfLeds || indexV >= NUM_LEDS_Max) {
                      ppf("dev post [%d] indexV too high %d>=%d or %d (p:%d m:%d) p:%d,%d,%d\n", rowNr, indexV, leds->nrOfLeds, NUM_LEDS_Max, leds->mappingTable.size(), indexP, pixel.x, pixel.y, pixel.z);
                    }
                    else if (indexP < NUM_LEDS_Max) {
                      //create new physMaps if needed
                      if (indexV >= leds->mappingTable.size()) {
                        for (size_t i = leds->mappingTable.size(); i <= indexV; i++) {
                          // ppf("mapping %d,%d,%d add physMap before %d %d\n", pixel.y, pixel.y, pixel.z, indexV, leds->mappingTable.size());
                          leds->mappingTable.push_back(PhysMap()); //abort() was called at PC 0x40191473 on core 1 std::allocator<unsigned short> >&&)
                        }
                      }
                      //indexV is within the square
                      if (!leds->isMapped(indexV)) {
                        leds->mappingTable[indexV].indexes = new std::vector<unsigned16>;
                      }
                      leds->mappingTable[indexV].indexes->push_back(indexP); //add the current led to indexes
                    }
                    else 
                      ppf("dev post [%d] indexP too high %d>=%d or %d (p:%d m:%d) p:%d,%d,%d\n", rowNr, indexP, nrOfLeds, NUM_LEDS_Max, leds->mappingTable.size(), indexP, pixel.x, pixel.y, pixel.z);
                  }
                  // ppf("mapping b:%d t:%d V:%d\n", indexV, indexP, leds->mappingTable.size());
                } //indexV not too high
              } //indexV

            } //if x,y,z between start and endpos
          } //leds->doMap
          rowNr++;
        } //projections
        indexP++; //also increase if no buffer created
      } //if 1D-3D pixel

      else { // end of leds array

        if (doAllocPins) {
          //check if pin already allocated, if so, extend range in details
          PinObject pinObject = pins->pinObjects[currPin];
          char details[32] = "";
          if (pins->isOwner(currPin, "Leds")) { //if owner

            char * after = strtok((char *)pinObject.details, "-");
            if (after != NULL ) {
              char * before;
              before = after;
              after = strtok(NULL, " ");
              stackUnsigned16 startLed = atoi(before);
              stackUnsigned16 nrOfLeds = atoi(after) - atoi(before) + 1;
              print->fFormat(details, sizeof(details)-1, "%d-%d", min(prevIndexP, startLed), max((stackUnsigned16)(indexP - 1), nrOfLeds)); //careful: LedModEffects:loop uses this to assign to FastLed
              ppf("pins extend leds %d: %s\n", currPin, details);
              //tbd: more check

              strncpy(pins->pinObjects[currPin].details, details, sizeof(PinObject::details)-1);  
              pins->pinsChanged = true;
            }
          }
          else {//allocate new pin
            //tbd: check if free
            print->fFormat(details, sizeof(details)-1, "%d-%d", prevIndexP, indexP - 1); //careful: LedModEffects:loop uses this to assign to FastLed
            // ppf("allocatePin %d: %s\n", currPin, details);
            pins->allocatePin(currPin, "Leds", details);
          }

          prevIndexP = indexP;
        }
      }
    }); //starJson.lookFor("leds" (create the right type, otherwise crash)

    if (starJson.deserialize()) { //this will call above function parameter for each led

      //after processing each led
      stackUnsigned8 rowNr = 0;

      for (Leds *leds: projections) {
        if (leds->doMap) {
          ppf("projectAndMap post leds[%d] fx:%d pro:%d\n", rowNr, leds->fx, leds->projectionNr);

          stackUnsigned16 nrOfMappings = 0;
          stackUnsigned16 nrOfPixels = 0;

          if (leds->projectionNr == p_Random || leds->projectionNr == p_None) {

            //defaults
            leds->size = fixSize;
            leds->nrOfLeds = nrOfLeds;
            nrOfPixels = nrOfLeds;

          } else {

            // if (leds->mappingTable.size() < leds->size.x * leds->size.y * leds->size.z)
            //   ppf("mapping add extra physMap %d to %d size: %d,%d,%d\n", leds->mappingTable.size(), leds->size.x * leds->size.y * leds->size.z, leds->size.x, leds->size.y, leds->size.z);
            // for (size_t i = leds->mappingTable.size(); i < leds->size.x * leds->size.y * leds->size.z; i++) {
            //   std::vector<unsigned16> physMap;
            //   // physMap.push_back(0);
            //   leds->mappingTable.push_back(physMap);
            // }

            leds->nrOfLeds = leds->mappingTable.size();

            //debug info + summary values
            stackUnsigned16 indexV = 0;
            for (PhysMap &map:leds->mappingTable) {
              if (map.indexes && map.indexes->size()) {
                // if (nrOfMappings < 10 || map.indexes->size() - indexV < 10) //first 10 and last 10
                // if (nrOfMappings%(leds->nrOfLeds/10+1) == 0)
                  // ppf("ledV %d mapping: #ledsP (%d):", indexV, nrOfMappings, map.indexes->size());

                for (forUnsigned16 indexP:*map.indexes) {
                  // if (nrOfPixels < 10 || map.indexes->size() - indexV < 10)
                  // if (nrOfMappings%(leds->nrOfLeds/10+1) == 0)
                    // ppf(" %d", indexP);
                  nrOfPixels++;
                }

                // if (nrOfPixels < 10 || map.indexes->size() - indexV < 10)
                // if (nrOfMappings%(leds->nrOfLeds/10+1) == 0)
                  // ppf("\n");
              }
              nrOfMappings++;
              // else
              //   ppf("ledV %d no mapping\n", x);
              indexV++;
            }
          }

          ppf("projectAndMap leds[%d] V:%d x %d x %d -> %d (v:%d - p:%d)\n", rowNr, leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds, nrOfMappings, nrOfPixels);

          // mdl->setValueV("fxSize", rowNr, "%d x %d x %d = %d", leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds);
          char buf[32];
          print->fFormat(buf, sizeof(buf)-1,"%d x %d x %d -> %d", leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds);
          mdl->setValue("fxSize", JsonString(buf, JsonString::Copied), rowNr);
          // web->sendResponseObject();

          ppf("projectAndMap leds[%d].size = %d + %d\n", rowNr, sizeof(Leds), leds->mappingTable.size()); //44

          leds->doMap = false;
        } //leds->doMap
        rowNr++;
      } // leds

      ppf("projectAndMap fixture P:%dx%dx%d -> %d\n", fixSize.x, fixSize.y, fixSize.z, nrOfLeds);

      mdl->setValue("fixSize", fixSize);
      mdl->setValue("fixCount", nrOfLeds);

    } // if deserialize
  } //if fileName
  else
    ppf("projectAndMap: Filename for fixture %d not found\n", fixtureNr);

  doMap = false;
}