/*
   @title     StarBase
   @file      SysModPrint.h
   @date      20240411
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "SysModule.h"

#define ppf(x...) print->printf(x)
// #define ppf(x...) //to have no print code compiled, difference is only 6308 bytes 
// Flash: [======    ]  62.8% (used 1194250 bytes from 1900544 bytes)
// Flash: [======    ]  63.2% (used 1200558 bytes from 1900544 bytes)

class SysModPrint:public SysModule {

public:

  SysModPrint();
  void setup();
  void loop20ms();

  //generic print function
  void printf(const char * format, ...);

  //not used yet
  void println(const __FlashStringHelper * x);

  //print var as id:value + [childVars recursively]
  void printVar(JsonObject var);

  void printJson(const char * text, JsonVariantConst source);

  size_t fFormat(char * buf, size_t size, const char * format, ...);

  void printJDocInfo(const char * text, JsonDocument source);

private:
  bool setupsDone = false;
};

extern SysModPrint *print;