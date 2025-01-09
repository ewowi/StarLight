/*
   @title     StarLight
   @file      LedModFixture.h
   @date      20241219
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

#ifdef STARLIGHT_PHYSICAL_DRIVER
  #define NUMSTRIPS 16 //can this be changed e.g. when we have 20 pins?
  #define NUM_LEDS_PER_STRIP 256 //could this be removed from driver lib as makes not so much sense
  #if CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2
    #include "I2SClockLessLedDriveresp32s3.h"
    static I2SClocklessLedDriveresp32S3 driver;
  #else
    #include "I2SClocklessLedDriver.h"
    static I2SClocklessLedDriver driver;
  #endif
#elif STARLIGHT_VIRTUAL_DRIVER //see https://github.com/ewowi/I2SClocklessVirtualLedDriver read me
  
  // #define NUM_LEDS_PER_STRIP 256 // for I2S_MAPPING_MODE_OPTION_MAPPING_IN_MEMORY ...

  #define CORE_DEBUG_LEVEL 0 //surpress ESP_LOGE compile error, increase to 6 when debugging
  //catch errors from library, enable when debugging
  // #define ICVD_LOGD(tag, format, ...) ppf(format, ##__VA_ARGS__)
  // #define ICVD_LOGE(tag, format, ...) ppf(format, ##__VA_ARGS__)
  // #define ICVD_LOGV(tag, format, ...) ppf(format, ##__VA_ARGS__)
  // #define ICVD_LOGI(tag, format, ...) ppf(format, ##__VA_ARGS__)

  #define USE_FASTLED //so CRGB is supported e.g. in initLed
  // #define __BRIGHTNESS_BIT 5 //underscore ! default 8, set off for the moment as ui brightness stopped working, will look at it later. 
                                //the max brightness will be 2^5=32 If you remember when I have discussed about the fact that the showPixels is not always occupied with gives time for other processes to run. Well the less time we 'spent' in buffer calcualtion the better.for instance if you do not use gamma calculation and you can cope with a brightness that is a power of 2:
  // #include "esp_heap_caps.h"
  #if STARBASE_USERMOD_LIVE & STARLIGHT_LIVE_MAPPING
    #define I2S_MAPPING_MODE (I2S_MAPPING_MODE_OPTION_MAPPING_SOFTWARE) //works no flickering anymore (due to __NB_DMA_BUFFER)!
    // #define I2S_MAPPING_MODE (I2S_MAPPING_MODE_OPTION_MAPPING_IN_MEMORY) //not working: IllegalInstruction Backtrace: 0x5515d133:0x3ffb1fc0 |<-CORRUPTED
    // #define _DMA_EXTENSTION 64 //not needed (yet)
  #else
    #define I2S_MAPPING_MODE (I2S_MAPPING_MODE_OPTION_NONE) //works but mapping using StarLight mappingTable needed
  #endif

  #define TAG "StarLight" // for S3 (todo also for non s3...)

  #include "I2SClocklessVirtualLedDriver.h"
  static I2SClocklessVirtualLedDriver driver;

#elif STARLIGHT_HUB75_DRIVER
  #include "WhateverHubDriver.h"
  static WhateverHubDriver driver;
#endif

struct SortedPin {
  uint16_t startLed;
  uint16_t nrOfLeds;
  uint8_t pin;
};

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
    ppf("Fixture constructor ptb:%d\n", pixelsToBlend.size());

    #ifdef STARLIGHT_PHYSICAL_DRIVER
      //'hack' to make sure show is not called before init
      #if !(CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2)
        driver.total_leds = 0;
      #endif
    #endif
  };

  void setup() override;

  void loop() override;
  void loop1s() override;

  Coord3D fixSize = {8,8,1};
  uint16_t nrOfLeds = 64; //amount of physical leds

  //Fixture definition
  uint8_t ledFactor = 1;
  uint8_t ledSize = 4; //mm
  uint8_t ledShape = 0; //0 = sphere, 1 = TetrahedronGeometry

  //clockless driver (check FastLED support...)
  uint8_t colorOrder = 3; //GRB is default for WS2812 (not for FastLED yet: see pio.ini)

  //for virtual driver (but keep enabled to avoid compile errors when used in non virtual context
  uint8_t clockPin = 3; //3 for S3, 26 for ESP32 (wrover)
  uint8_t latchPin = 46; //46 for S3, 27 for ESP32 (wrover)
  uint8_t clockFreq = 10; //clockFreq==10?clock_1000KHZ:clockFreq==11?clock_1111KHZ:clockFreq==12?clock_1123KHZ:clock_800KHZ
                // 1.0MHz is default and runs well (0.8MHz is normal non overclocking). higher then 1.0 is causing flickering at least at ewowi big screen
  uint8_t dmaBuffer = 30; //not used yet

  //End Fixture definition
  
  unsigned long lastMappingMillis = 0;
  uint8_t viewRotation = 0;
  uint8_t bri = 10;
  uint8_t bytesPerPixel = 2;

  uint8_t gammaRed = 255;
  uint8_t gammaGreen = 176;
  uint8_t gammaBlue = 240;

  uint8_t fixtureNr = UINT8_MAX;

  std::vector<LedsLayer *> layers; //virtual leds

  Coord3D head = {0,0,0};

  uint8_t mappingStatus = 0; //not mapping
  bool doAllocPins = false;
  bool doSendFixtureDefinition = false;

  uint8_t globalBlend = 128;

  uint16_t fps = 200;
  uint16_t realFps = 200;
  bool3State showTicker = false;
  char tickerTape[20] = "";
  bool3State showDriver = true;
  uint16_t maxPowerWatt = 10; //default 10W, save for usb ports

  //temporary here  
  uint16_t indexP = 0;
  uint16_t prevIndexP = 0;
  uint8_t currPin;

  void mapInitAlloc();

  //load fixture json file, parse it and depending on the projection, create a mapping for it
  uint16_t previewBufferIndex = 0;
  unsigned long start = millis();
  uint8_t pass = 0; //'class global' so addPixel/Pin functions know which pass it is in
  byte* buffer; // AsyncWebSocketMessageBuffer * wsBuf; //buffer for preview create fixture
  void addPixelsPre();
  void addPixel(Coord3D pixel);
  void addPin(uint8_t pin);
  void addPixelsPost();
  void driverInit(const std::vector<SortedPin> &sortedPins);
  void driverShow();

  #ifdef STARBASE_USERMOD_LIVE
    uint8_t liveFixtureID = UINT8_MAX;
  #endif

  #if STARLIGHT_PHYSICAL_DRIVER || STARLIGHT_VIRTUAL_DRIVER
    uint8_t setMaxPowerBrightnessFactor = 90; //tbd: implement driver.setMaxPowerInMilliWatts
  #endif

};

extern LedModFixture *fix;