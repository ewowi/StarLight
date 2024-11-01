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

#pragma once
#include "SysModule.h"
#include "../Sys/SysModModel.h"

#include "LedLayer.h"

#include "FastLED.h"

#ifdef STARLIGHT_CLOCKLESS_LED_DRIVER
  #if CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2
    #include "I2SClockLessLedDriveresp32s3.h"
  #else
    #include "I2SClocklessLedDriver.h"
  #endif
#endif
#ifdef STARLIGHT_CLOCKLESS_VIRTUAL_LED_DRIVER
  //used in I2SClocklessVirtualLedDriver.h,
  //see https://github.com/ewowi/I2SClocklessVirtualLedDriver read me
  #define NUM_LEDS_PER_STRIP 256 // for I2S_MAPPING_MODE_OPTION_MAPPING_IN_MEMORY ...
  #define CORE_DEBUG_LEVEL 1 //surpress ESP_LOGE compile error
  #define USE_FASTLED //so CRGB is supported e.g. in initLed
  // #define __BRIGHTNESS_BIT 5 //underscore ! default 8, set off for the moment as ui brightness stopped working, will look at it later. 
                                //the max brightness will be 2^5=32 If you remember when I have discussed about the fact that the showPixels is not always occupied with gives time for other processes to run. Well the less time we 'spent' in buffer calcualtion the better.for instance if you do not use gamma calculation and you can cope with a brightness that is a power of 2:
  #ifndef NBIS2SERIALPINS //no underscore !, defined in pio.ini
    #define NBIS2SERIALPINS 6 //6 shift registers
  #endif
  // #include "esp_heap_caps.h"
  #if STARBASE_USERMOD_LIVE & STARLIGHT_LIVE_MAPPING
    #define I2S_MAPPING_MODE (I2S_MAPPING_MODE_OPTION_MAPPING_SOFTWARE) //works no flickering anymore (due to __NB_DMA_BUFFER)!
    // #define I2S_MAPPING_MODE (I2S_MAPPING_MODE_OPTION_MAPPING_IN_MEMORY) //not working: IllegalInstruction Backtrace: 0x5515d133:0x3ffb1fc0 |<-CORRUPTED
    #define __NB_DMA_BUFFER 10 //or 10 ... underscore ! default 2 (2 causes flickering in case of mapping). 
  #else
    #define I2S_MAPPING_MODE (I2S_MAPPING_MODE_OPTION_NONE) //works but mapping using StarLight mappingTable needed
  #endif

  #include "I2SClocklessVirtualLedDriver.h"

  //StarLight specific
  #ifndef STARLIGHT_ICVLD_PINS
    #define STARLIGHT_ICVLD_PINS 14,12,13,25,33,32 // must be 6, see initLed
  #endif

  #ifndef STARLIGHT_ICVLD_CLOCK_PIN
    #define STARLIGHT_ICVLD_CLOCK_PIN 26
  #endif
  #ifndef STARLIGHT_ICVLD_LATCH_PIN
    #define STARLIGHT_ICVLD_LATCH_PIN 27
  #endif
#endif

class LedModFixture: public SysModule {

public:

  CRGB ledsP[STARLIGHT_MAXLEDS];

  // CRGB *leds = nullptr;
    // if (!leds)
  //   leds = (CRGB*)calloc(nrOfLeds, sizeof(CRGB));
  // else
  //   leds = (CRGB*)reallocarray(leds, nrOfLeds, sizeof(CRGB));
  // if (leds) free(leds);
  // leds = (CRGB*)malloc(nrOfLeds * sizeof(CRGB));
  // leds = (CRGB*)reallocarray

  std::vector<bool> pixelsToBlend; //this is a 1-bit vector !!! overlapping effects will blend
  LedModFixture() :SysModule("Fixture") {
    //init pixelsToBlend
    for (uint16_t i=0; i<nrOfLeds; i++) {
      if (pixelsToBlend.size() < nrOfLeds)
        pixelsToBlend.push_back(false);
    }
    ppf("Fixture constructor ptb:%d", pixelsToBlend.size());

    #ifdef STARLIGHT_CLOCKLESS_LED_DRIVER
      #if !(CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2)
        driver.total_leds = 0;
      #endif
    #endif
  };

  void setup();

  void loop();
  void loop1s();

  Coord3D fixSize = {8,8,1};
  uint16_t nrOfLeds = 64; //amount of physical leds
  uint8_t factor = 1;
  uint8_t ledSize = 4; //mm
  uint8_t shape = 0; //0 = sphere, 1 = TetrahedronGeometry

  unsigned long lastMappingMillis = 0;
  uint8_t viewRotation = 0;
  uint8_t bri = 10;
  uint8_t bytesPerPixel = 2;

  uint8_t fixtureNr = UINT8_MAX;

  std::vector<LedsLayer *> layers; //virtual leds

  Coord3D head = {0,0,0};

  uint8_t mappingStatus = 0; //not mapping
  bool doAllocPins = false;
  bool doSendFixtureDefinition = false;

  uint8_t globalBlend = 128;

  uint16_t fps = 200;
  uint16_t realFps = 200;
  bool3State showTicker = true;
  char tickerTape[20] = "";
  bool3State driverShow = true;

  //temporary here  
  uint16_t indexP = 0;
  uint16_t prevIndexP = 0;
  uint8_t currPin;

  void mapInitAlloc();

  //load fixture json file, parse it and depending on the projection, create a mapping for it
  uint16_t previewBufferIndex = 0;
  unsigned long start = millis();
  uint8_t pass = 0; //'class global' so addPixel/Pin functions know which pass it is in
  AsyncWebSocketMessageBuffer * wsBuf; //buffer for preview create fixture
  void addPixelsPre();
  void addPixel(Coord3D pixel);
  void addPin(uint8_t pin);
  void addPixelsPost();

  #ifdef STARBASE_USERMOD_LIVE
    uint8_t liveFixtureID = UINT8_MAX;
  #endif

  #ifdef STARLIGHT_CLOCKLESS_LED_DRIVER
    #if CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2
      I2SClocklessLedDriveresp32S3 driver;
    #else
      I2SClocklessLedDriver driver;
    #endif
    uint8_t setMaxPowerBrightnessFactor = 90; //tbd: implement driver.setMaxPowerInMilliWatts
  #endif
  #ifdef STARLIGHT_CLOCKLESS_VIRTUAL_LED_DRIVER
    I2SClocklessVirtualLedDriver driver;
    uint8_t setMaxPowerBrightnessFactor = 90; //tbd: implement driver.setMaxPowerInMilliWatts
  #endif
};

extern LedModFixture *fix;