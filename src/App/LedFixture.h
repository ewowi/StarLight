/*
   @title     StarLight
   @file      LedFixture.h
   @date      20240228
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once

#include "../Sys/SysModModel.h" //for Coord3D

#include "LedLeds.h"

#define NUM_LEDS_Max 8192

#define _1D 1
#define _2D 2
#define _3D 3


class Leds; //forward

class Projection {
public:
  virtual const char * name() {return "noname";}
  virtual const char * tags() {return "";}

  virtual void setup(Leds &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted, Coord3D &mapped, uint16_t &indexV) {}
  
  virtual void adjustXYZ(Leds &leds, Coord3D &pixel) {}
  
  virtual void controls(Leds &leds, JsonObject parentVar) {}

};

class Fixture {

public:

  CRGB ledsP[NUM_LEDS_Max];

  // CRGB *leds = nullptr;
    // if (!leds)
  //   leds = (CRGB*)calloc(nrOfLeds, sizeof(CRGB));
  // else
  //   leds = (CRGB*)reallocarray(leds, nrOfLeds, sizeof(CRGB));
  // if (leds) free(leds);
  // leds = (CRGB*)malloc(nrOfLeds * sizeof(CRGB));
  // leds = (CRGB*)reallocarray

  std::vector<Projection *> projections;

  unsigned16 nrOfLeds = 64; //amount of physical leds
  unsigned8 fixtureNr = -1;
  Coord3D fixSize = {8,8,1};

  std::vector<Leds *> listOfLeds; //virtual leds

  Coord3D head = {0,0,0};

  bool doMap = false;
  bool doAllocPins = false;

  unsigned8 globalBlend = 128;
  
  //load fixture json file, parse it and depending on the projection, create a mapping for it
  void projectAndMap();

  #ifdef STARLIGHT_CLOCKLESS_LED_DRIVER
    uint8_t setMaxPowerBrightness = 30; //tbd: implement driver.setMaxPowerInMilliWatts
  #endif

};
