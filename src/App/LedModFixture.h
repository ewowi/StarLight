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
  #define NUM_LEDS_PER_STRIP 256 //used in I2SClocklessVirtualLedDriver.h, should not be needed here...
  #define CORE_DEBUG_LEVEL 1 //surpress ESP_LOGE compile error
  #define USE_FASTLED //so CRGB is supported e.g. in initled
  #define NB_DMA_BUFFER 10
  #define BRIGHTNESS_BIT 5
  #ifndef NBIS2SERIALPINS
    #define NBIS2SERIALPINS 6 //6 shift registers
  #endif
  #ifndef SVCD_PINS
    #define SVCD_PINS 14,12,13,25,33,32
  #endif
  // #include "esp_heap_caps.h"
  #define I2S_MAPPING_MODE (I2S_MAPPING_MODE_OPTION_NONE) // (I2S_MAPPING_MODE_OPTION_MAPPING_IN_MEMORY)
  #include "I2SClocklessVirtualLedDriver.h"
  #ifndef SCVD_CLOCK_PIN
    #define SCVD_CLOCK_PIN 26
  #endif
  #ifndef SCVD_LATCH_PIN
    #define SCVD_LATCH_PIN 27
  #endif
#endif

#define NUM_LEDS_Max 8192

class LedModFixture: public SysModule {

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
  uint16_t ledSize = 5; //mm
  uint16_t shape = 0; //0 = sphere, 1 = TetrahedronGeometry
  uint16_t factor = 10;

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

  uint16_t fps = 60;
  uint16_t realFps = 60;
  bool showInfo = true;
  char infoText[20] = "";
  bool driverShow = true;

  //temporary here  
  uint16_t indexP = 0;
  uint16_t prevIndexP = 0;
  uint16_t currPin; //lookFor needs u16

  void mapInitAlloc();

  //load fixture json file, parse it and depending on the projection, create a mapping for it
  uint16_t previewBufferIndex = 0;
  AsyncWebSocketMessageBuffer * wsBuf; //buffer for preview create fixture
  void addPixelsPre(Coord3D size, uint16_t nrOfLeds, uint8_t factor = 10, uint8_t ledSize = 5, uint8_t shape = 0);
  void addPixel(Coord3D pixel);
  void addPin(uint16_t pin);
  void addPixelsPost();

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