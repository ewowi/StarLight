/*
   @title     StarLight
   @file      LedFixture.h
   @date      20241014
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once

#include "../Sys/SysModModel.h" //for Coord3D

#include "LedLayer.h"

#define NUM_LEDS_Max 8192

class LedsLayer; //forward

class Fixture {

public:

  CRGB ledsP[NUM_LEDS_Max];
  std::vector<bool> pixelsToBlend; //this is a 1-bit vector !!! overlapping effects will blend

  // CRGB *leds = nullptr;
    // if (!leds)
  //   leds = (CRGB*)calloc(nrOfLeds, sizeof(CRGB));
  // else
  //   leds = (CRGB*)reallocarray(leds, nrOfLeds, sizeof(CRGB));
  // if (leds) free(leds);
  // leds = (CRGB*)malloc(nrOfLeds * sizeof(CRGB));
  // leds = (CRGB*)reallocarray

  uint16_t nrOfLeds = 64; //amount of physical leds
  uint8_t fixtureNr = -1;
  Coord3D fixSize = {8,8,1};

  std::vector<LedsLayer *> layers; //virtual leds

  Coord3D head = {0,0,0};

  bool doMap = false;
  bool doAllocPins = false;

  uint8_t globalBlend = 128;

  Fixture() {
    //init pixelsToBlend
    for (uint16_t i=0; i<nrOfLeds; i++) {
      if (pixelsToBlend.size() < nrOfLeds)
        pixelsToBlend.push_back(false);
    }
    ppf("Fixture constructor ptb:%d", pixelsToBlend.size());
  }

  //temporary here  
  uint16_t indexP = 0;
  uint16_t prevIndexP = 0;
  uint16_t currPin; //lookFor needs u16

  //load fixture json file, parse it and depending on the projection, create a mapping for it
  void projectAndMap();
  void projectAndMapPre();
  void projectAndMapPixel(Coord3D pixel);
  void projectAndMapPin(uint16_t pin);
  void projectAndMapPost();

  #ifdef STARLIGHT_CLOCKLESS_LED_DRIVER
    uint8_t setMaxPowerBrightness = 30; //tbd: implement driver.setMaxPowerInMilliWatts
  #endif

};
