/*
   @title     StarLight
   @file      LedFixture.cpp
   @date      20241014
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