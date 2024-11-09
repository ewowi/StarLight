/*
   @title     StarLight
   @file      LedModEffects.h
   @date      20241105
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "LedLayer.h"
#include <vector>

class LedModEffects:public SysModule {

public:
  bool newFrame = false; //for other modules (DDP)
  unsigned long frameCounter = 0;

  std::vector<Effect *> effects;
  std::vector<Projection *> projections;

  uint8_t doInitEffectRowNr = UINT8_MAX;

  LedModEffects();

  void setup();

  //this loop is run as often as possible so coding should also be as efficient as possible (no findVar etc)
  void loop();

  void initEffect(LedsLayer &leds, uint8_t rowNr);

  // void loop10s();

private:
  unsigned long frameMillis = 0;
  JsonObject varSystem = JsonObject();

};

extern LedModEffects *eff;