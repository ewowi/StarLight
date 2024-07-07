/*
   @title     StarLight
   @file      LedFixture.cpp
   @date      20240228
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "LedFixture.h"

#include "../Sys/SysModFiles.h"
#include "../Sys/SysStarJson.h"
#include "../Sys/SysModPins.h"


//load fixture json file, parse it and depending on the projection, create a mapping for it
void Fixture::projectAndMap() {
  unsigned long start = millis();
  char fileName[32] = "";

  if (files->seqNrToName(fileName, fixtureNr, "F_")) { // get the fixture.json
    StarJson starJson(fileName); //open fileName for deserialize

    // reset leds
    stackUnsigned8 rowNr = 0;
    for (Leds *leds: listOfLeds) {
      if (leds->doMap) {
        leds->fill_solid(CRGB::Black, true); //no blend

        ppf("projectAndMap clear leds[%d] fx:%d pro:%d\n", rowNr, leds->fx, leds->projectionNr);
        leds->size = Coord3D{0,0,0};
        //vectors really gone now?
        for (PhysMap &map:leds->mappingTable) {
          if (map.isMultipleIndexes()) {
            map.indexes->clear();
            delete map.indexes;
          }
        }
        leds->mappingTable.clear();
        // leds->effectData.reset(); //do not reset as want to save settings.
      }
      rowNr++;
    }

    //deallocate all led pins
    if (doAllocPins) {
      stackUnsigned8 pinNr = 0;
      for (PinObject &pinObject: pinsM->pinObjects) {
        if (strcmp(pinObject.owner, "Leds") == 0)
          pinsM->deallocatePin(pinNr, "Leds");
        pinNr++;
      }
    }

    uint16_t indexP = 0;
    uint16_t prevIndexP = 0;
    uint16_t currPin; //lookFor needs u16

    //what to deserialize
    starJson.lookFor("width", (uint16_t *)&fixSize.x);
    starJson.lookFor("height", (uint16_t *)&fixSize.y);
    starJson.lookFor("depth", (uint16_t *)&fixSize.z);
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

        if (indexP < NUM_LEDS_Max) {

          stackUnsigned8 rowNr = 0;
          for (Leds *leds: listOfLeds) {

            if (leds->projectionNr != p_Random && leds->projectionNr != p_None) //only real projections
            if (leds->doMap) { //add pixel in leds mappingtable

              //set start and endPos between bounderies of fixture
              Coord3D startPosAdjusted = (leds->startPos).minimum(fixSize - Coord3D{1,1,1}) * 10;
              Coord3D endPosAdjusted = (leds->endPos).minimum(fixSize - Coord3D{1,1,1}) * 10;
              Coord3D midPosAdjusted = (leds->midPos).minimum(fixSize - Coord3D{1,1,1}); //not * 10

              // mdl->setValue("ledsStart", startPosAdjusted/10, rowNr); //rowNr
              // mdl->setValue("ledsEnd", endPosAdjusted/10, rowNr); //rowNr

              if (pixel >= startPosAdjusted && pixel <= endPosAdjusted ) { //if pixel between start and end pos

                Coord3D pixelAdjusted = (pixel - startPosAdjusted)/10; //pixelRelative to startPos in cm

                Coord3D sizeAdjusted = (endPosAdjusted - startPosAdjusted)/10 + Coord3D{1,1,1}; // in cm

                // 0 to 3D depending on start and endpos (e.g. to display ScrollingText on one side of a cube)
                leds->projectionDimension = 0;
                if (sizeAdjusted.x > 1) leds->projectionDimension++;
                if (sizeAdjusted.y > 1) leds->projectionDimension++;
                if (sizeAdjusted.z > 1) leds->projectionDimension++;

                Projection *projection = nullptr;
                if (leds->projectionNr < projections.size())
                  projection = projections[leds->projectionNr];

                mdl->getValueRowNr = rowNr; //run projection functions in the right rowNr context

                //calculate the indexV to add to current physical led to
                uint16_t indexV = UINT16_MAX;

                Coord3D mapped;
                if (leds->projectionNr == p_Pinwheel) {
                  (projection->*leds->adjustSizeAndPixelCached)(*leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);  // Adjust Size and Pixel
                  if (leds->size == Coord3D{0,0,0}) {                                       
                    leds->size = sizeAdjusted;                                                                        // Adjust leds->size if not done yet
                    ppf("projectAndMap first leds[%d] size:%d,%d,%d s:%d,%d,%d e:%d,%d,%d\n", rowNr, sizeAdjusted.x, sizeAdjusted.y, sizeAdjusted.z, startPosAdjusted.x, startPosAdjusted.y, startPosAdjusted.z, endPosAdjusted.x, endPosAdjusted.y, endPosAdjusted.z);
                  }
                  (projection->*leds->adjustMappedCached)(*leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);// Adjust Mapped
                  indexV = leds->XYZUnprojected(mapped);
                }
                else {
                  //using cached virtual class methods!
                  if (projection) (projection->*leds->adjustSizeAndPixelCached)(*leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
                  if (leds->size == Coord3D{0,0,0}) { // first
                    ppf("projectAndMap first leds[%d] size:%d,%d,%d s:%d,%d,%d e:%d,%d,%d\n", rowNr, sizeAdjusted.x, sizeAdjusted.y, sizeAdjusted.z, startPosAdjusted.x, startPosAdjusted.y, startPosAdjusted.z, endPosAdjusted.x, endPosAdjusted.y, endPosAdjusted.z);
                  }                
                  switch (leds->effectDimension) {
                    case _1D: //effectDimension 1DxD
                      if (leds->size == Coord3D{0,0,0}) { // first
                        leds->size.x = sqrt(sq(max(sizeAdjusted.x - midPosAdjusted.x, midPosAdjusted.x)) + 
                                            sq(max(sizeAdjusted.y - midPosAdjusted.y, midPosAdjusted.y)) + 
                                            sq(max(sizeAdjusted.z - midPosAdjusted.z, midPosAdjusted.z))) + 1;
                        leds->size.y = 1;
                        leds->size.z = 1;
                      }

                      mapped = pixelAdjusted;

                      //using cached virtual class methods!
                      if (projection) (projection->*leds->adjustMappedCached)(*leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted); // Currently does nothing.

                      mapped.x = mapped.distance(midPosAdjusted);
                      mapped.y = 0;
                      mapped.z = 0;                

                      indexV = leds->XYZUnprojected(mapped);
                      break;
                    case _2D: //effectDimension
                      switch(leds->projectionDimension) {
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

                      //using cached virtual class methods!
                      if (projection) (projection->*leds->adjustMappedCached)(*leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted); // Currently does nothing.

                      indexV = leds->XYZUnprojected(mapped);
                      break;
                    case _3D: //effectDimension
                      mapped = pixelAdjusted;
                      
                      switch(leds->projectionDimension) {
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

                      //using cached virtual class methods!
                      if (projection) (projection->*leds->adjustMappedCached)(*leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted); // Currently does nothing.

                      indexV = leds->XYZUnprojected(mapped);
                      
                      break; //effectDimension _3D
                  } //effectDimension
                }
                leds->nrOfLeds = leds->size.x * leds->size.y * leds->size.z;

                //post processing: 
                if (projection) (projection->*leds->postProcessingCached)(*leds, indexV);

                if (indexV != UINT16_MAX) {
                  if (indexV >= leds->nrOfLeds || indexV >= NUM_VLEDS_Max)
                    ppf("dev pre [%d] indexV too high %d>=%d or %d (m:%d p:%d) p:%d,%d,%d s:%d,%d,%d\n", rowNr, indexV, leds->nrOfLeds, NUM_VLEDS_Max, leds->mappingTable.size(), indexP, pixel.x, pixel.y, pixel.z, leds->size.x, leds->size.y, leds->size.z);
                  else {

                    //create new physMaps if needed
                    if (indexV >= leds->mappingTable.size()) {
                      for (size_t i = leds->mappingTable.size(); i <= indexV; i++) {
                        // ppf("mapping %d,%d,%d add physMap before %d %d\n", pixel.y, pixel.y, pixel.z, indexV, leds->mappingTable.size());
                        leds->mappingTable.push_back(PhysMap()); //abort() was called at PC 0x40191473 on core 1 std::allocator<unsigned short> >&&)
                      }
                    }

                    leds->mappingTable[indexV].addIndexP(indexP);
                    // ppf("mapping b:%d t:%d V:%d\n", indexV, indexP, leds->mappingTable.size());
                  } //indexV not too high
                } //indexV

                mdl->getValueRowNr = UINT8_MAX; // end of run projection functions in the right rowNr context

              } //if x,y,z between start and endpos
            } //if leds->doMap
            rowNr++;
          } //for listOfLeds
        } //indexP < max
        else 
          ppf("dev post indexP too high %d>=%d or %d p:%d,%d,%d\n", indexP, nrOfLeds, NUM_LEDS_Max, pixel.x, pixel.y, pixel.z);
        indexP++; //also increase if no buffer created
      } //if 1D-3D pixel

      else { // end of leds array

        if (doAllocPins) {
          //check if pin already allocated, if so, extend range in details
          PinObject pinObject = pinsM->pinObjects[currPin];
          char details[32] = "";
          if (pinsM->isOwner(currPin, "Leds")) { //if owner

            char * after = strtok((char *)pinObject.details, "-");
            if (after != NULL ) {
              char * before;
              before = after;
              after = strtok(NULL, " ");
              uint16_t startLed = atoi(before);
              uint16_t nrOfLeds = atoi(after) - atoi(before) + 1;
              print->fFormat(details, sizeof(details)-1, "%d-%d", min(prevIndexP, startLed), max((uint16_t)(indexP - 1), nrOfLeds)); //careful: LedModEffects:loop uses this to assign to FastLed
              ppf("pins extend leds %d: %s\n", currPin, details);
              //tbd: more check

              strncpy(pinsM->pinObjects[currPin].details, details, sizeof(PinObject::details)-1);  
              pinsM->pinsChanged = true;
            }
          }
          else {//allocate new pin
            //tbd: check if free
            print->fFormat(details, sizeof(details)-1, "%d-%d", prevIndexP, indexP - 1); //careful: LedModEffects:loop uses this to assign to FastLed
            // ppf("allocatePin %d: %s\n", currPin, details);
            pinsM->allocatePin(currPin, "Leds", details);
          }

          prevIndexP = indexP;
        }
      }
    }); //starJson.lookFor("leds" (create the right type, otherwise crash)

    if (starJson.deserialize()) { //this will call above function parameter for each led

      //after processing each led
      stackUnsigned8 rowNr = 0;

      for (Leds *leds: listOfLeds) {
        if (leds->doMap) {
          ppf("projectAndMap post leds[%d] fx:%d pro:%d\n", rowNr, leds->fx, leds->projectionNr);

          uint16_t nrOfMappings = 0;
          uint16_t nrOfPixels = 0;

          if (leds->projectionNr == p_Random || leds->projectionNr == p_None) {

            //defaults
            leds->size = fixSize;
            leds->nrOfLeds = nrOfLeds;
            nrOfPixels = nrOfLeds;

          } else {

            if (leds->mappingTable.size() < leds->size.x * leds->size.y * leds->size.z)
              ppf("mapping add extra physMap %d to %d size: %d,%d,%d\n", leds->mappingTable.size(), leds->size.x * leds->size.y * leds->size.z, leds->size.x, leds->size.y, leds->size.z);
            for (size_t i = leds->mappingTable.size(); i < leds->size.x * leds->size.y * leds->size.z; i++) {
              leds->mappingTable.push_back(PhysMap());
            }

            leds->nrOfLeds = leds->mappingTable.size();

            //debug info + summary values
            uint16_t indexV = 0;
            for (PhysMap &map:leds->mappingTable) {
            // for (auto map=leds->mappingTable.begin(); map!=leds->mappingTable.end(); ++map) {
              if (map.isOneIndex()) {
                  nrOfPixels++;
              }
              else if (map.isMultipleIndexes()) { // && map.indexes->size()
                // if (nrOfMappings < 10 || map.indexes->size() - indexV < 10) //first 10 and last 10
                // if (nrOfMappings%(leds->nrOfLeds/10+1) == 0)
                  // ppf("ledV %d mapping: #ledsP (%d):", indexV, nrOfMappings);

                for (uint16_t indexP:*map.indexes) {
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

          // mdl->setValueV("ledsSize", rowNr, "%d x %d x %d = %d", leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds);
          char buf[32];
          print->fFormat(buf, sizeof(buf)-1,"%d x %d x %d -> %d", leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds);
          mdl->setValue("ledsSize", JsonString(buf, JsonString::Copied), rowNr);
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
  ppf("projectAndMap done %d ms\n", millis()-start);
}