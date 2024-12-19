/*
   @title     StarBase
   @file      SysModule.h
   @date      20241219
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

//include all system wide libraries here (so no need to replicate in multiple modules)

#define bool3State uint8_t //0/false, 1/true, UINT8_MAX/unknown

#include <Arduino.h>
#include <HardwareSerial.h>  // ensure we have the correct "Serial" on new MCUs (depends on ARDUINO_USB_MODE and ARDUINO_USB_CDC_ON_BOOT)
#include <WiFi.h>
#include "esp_wifi.h"
#include <LittleFS.h>        // make sure that ArduinoJson knows we have LittleFS

// #define ARDUINOJSON_ENABLE_ARDUINO_STRING 0
#include "ArduinoJson.h"

#include <vector>

//used for init vars with vectors (vector<char[32]> not allowed)
struct VectorString {
  char s[32];
};

class StarString {
  char s[32] = "";
  char sep[3] = "";

  public: 

  //assign
  StarString& operator=(const char *rhs) {
    strlcpy(s, rhs, sizeof(s));
    return *this;
  }

  //concat
  StarString& operator+(const char *rhs) {
    strlcat(s, rhs, sizeof(s));
    return *this;
  }

  //concat
  StarString& operator+=(const char *rhs) {
    strlcat(s, sep, sizeof(s));
    strlcat(s, rhs, sizeof(s));
    // strlcpy(sep, ", ", sizeof(s));
    return *this;
  }

  void catSep(const char* s) {
    strlcpy(sep, s, sizeof(sep));
  }

  size_t length() {
    return strnlen(s, sizeof(s));
  }

  char *format(const char * format, ...) {
    va_list args;
    va_start(args, format);

    size_t len = vsnprintf(s, sizeof(s), format, args);

    va_end(args);
    return s;
  }

  char *getString() {
    return s;
  }

};

class SysModule {

public:
  const char * name;
  bool success;
  bool isEnabled;
  unsigned long twentyMsMillis = millis() - random(1000); //random so not all 1s are fired at once
  unsigned long oneSecondMillis = millis() - random(1000); //random so not all 1s are fired at once
  unsigned long tenSecondMillis = millis() - random(10000); //random within a second
  // void (SysModule::*loopCached)() = &SysModule::loop; //use virtual cached function for speed??? tested, no difference ...

  unsigned long cpuTime = 0;

  explicit SysModule(const char * name) {
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

  void addPresets(JsonObject parentVar);
};