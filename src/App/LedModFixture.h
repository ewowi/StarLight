/*
   @title     StarLight
   @file      LedModFixture.h
   @date      20241014
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModule.h"

class LedModFixture: public SysModule {

public:

  uint8_t viewRotation = 0;
  uint8_t bri = 10;
  bool rgb1B = false;

  LedModFixture() :SysModule("Fixture") {};

  void setup();
};

extern LedModFixture *fix;