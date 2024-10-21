/*
   @title     StarLight
   @file      LedModEffects.h
   @date      20241014
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

  uint16_t fps = 60;
  unsigned long lastMappingMillis = 0;

  std::vector<Effect *> effects;
  std::vector<Projection *> projections;

  Fixture fixture = Fixture();

  bool driverShow = true;

  uint8_t doInitEffectRowNr = UINT8_MAX;

  #ifdef STARLIGHT_CLOCKLESS_LED_DRIVER
    #if CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2
      I2SClocklessLedDriveresp32S3 driver;
    #else
      I2SClocklessLedDriver driver;
    #endif
  #endif

  LedModEffects();

  void setup();

  //this loop is run as often as possible so coding should also be as efficient as possible (no findVars etc)
  void loop();

  void mapInitAlloc();

  void initEffect(LedsLayer &leds, uint8_t rowNr);

  // void loop10s();

private:
  unsigned long frameMillis = 0;
  JsonObject varSystem = JsonObject();
  uint8_t viewRot = UINT8_MAX;

};

extern LedModEffects *eff;