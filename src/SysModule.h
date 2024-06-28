/*
   @title     StarBase
   @file      SysModule.h
   @date      20240411
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once

//make a string from pio variables (_INIT and STRINGIFY needed to make TOSTRING work)
#define _INIT(x) x
#define STRINGIFY(X) #X
#define TOSTRING(X) STRINGIFY(X)

//conventional (works)
// #define unsigned8 uint8_t
// #define unsigned16 uint16_t
// #define unsigned32 uint32_t

// tuning 1 (works)
#define unsigned8 uint8_t
#define unsigned16 uint16_t
#define unsigned32 unsigned
#define forUnsigned8 unsigned
#define forUnsigned16 unsigned
#define stackUnsigned8 uint8_t //unsigned causes some effects to run faulty. tbd to find out
#define stackUnsigned16 unsigned
#define stackUnsigned32 unsigned

//tuning 2 - current goal as lowest flash size and no speed difference (runs but some effects messed up: tbd find out why)
// #define unsigned8 unsigned
// #define unsigned16 unsigned
// #define unsigned32 unsigned

// //tuning 3 (runs but some effects messed up: tbd find out why)
// #define unsigned8 uint_fast8_t
// #define unsigned16 uint_fast16_t
// #define unsigned32 uint_fast32_t

//include all system wide libraries here (so no need to replicate in multiple modules)

#include <Arduino.h>
#include <HardwareSerial.h>  // ensure we have the correct "Serial" on new MCUs (depends on ARDUINO_USB_MODE and ARDUINO_USB_CDC_ON_BOOT)
#include <WiFi.h>
#include "esp_wifi.h"
#include <LittleFS.h>        // make sure that ArduinoJson knows we have LittleFS

// #define ARDUINOJSON_ENABLE_ARDUINO_STRING 0
#include "ArduinoJson.h"

#include <vector>

class SysModule {

public:
  const char * name;
  bool success;
  bool isEnabled;
  unsigned long twentyMsMillis = millis() - random(1000); //random so not all 1s are fired at once
  unsigned long oneSecondMillis = millis() - random(1000); //random so not all 1s are fired at once
  unsigned long tenSecondMillis = millis() - random(10000); //random within a second

  JsonObject parentVar;

  SysModule(const char * name) {
    this->name = name;
    success = true;
    isEnabled = true;
  }

  virtual void setup() {}

  virtual void loop() {}//24000 fps if no load...
  virtual void loop20ms() {} //50fps
  virtual void loop1s() {} //1fps
  virtual void loop10s() {}

  virtual void reboot() {}

  virtual void connectedChanged() {onOffChanged();}
  virtual void enabledChanged() {onOffChanged();}
  virtual void onOffChanged() {}

  virtual void testManager() {}
  virtual void performanceManager() {}
  virtual void dataSizeManager() {}
  virtual void codeSizeManager() {}
};