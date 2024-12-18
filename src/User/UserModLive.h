/*
   @title     StarBase
   @file      UserModLive.h
   @date      20241219
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors, asmParser © https://github.com/hpwit/ASMParser
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "../SysModule.h"

class UserModLive: public SysModule {

public:

  char fileName[32] = ""; //running sc file
  std::string scScript; //externals etc generated (would prefer String for esp32...)

  UserModLive() :SysModule("LiveScripts") {};

  void setup() override;

  void addDefaultExternals();

  void addExternalVal(std::string result, std::string name, void * ptr);

  void addExternalFun(std::string result, std::string name, std::string parameters, void * ptr);

  // void addExternalFun(string name, std::function<void(int)> fun);
  // void addExternalFun(string name, std::function<void(int, int)> fun);

  //static because called by Parser
  static void sync();
  static void preKill();
  static void postKill();

  void syncWithSync();

  void loop1s() override;

  //return the id of the executable
  uint8_t compile(const char *fileName, const char *post = nullptr);

  uint8_t findExecutable(const char *fileName);
  
  void executeTask(uint8_t exeID, const char *function = "main", int val = UINT16_MAX);
  void executeBackgroundTask(uint8_t exeID, const char *function = "main");

  void killAndDelete(const char *fileName = nullptr);
  void killAndDelete(uint8_t exeID);
};

extern UserModLive *liveM;


/*
Pre script:

external void show();
external void resetStat();
external void display(int a1);
external void dp(float a1);
external void error(int a1, int a2, int a3);
external void print(char * a1);
external float atan2(float a1, float a2);
external float hypot(float a1, float a2);
external float sin(float a1);
external float time(float a1);
external float triangle(float a1);
external uint32_t millis();
external void pinMode(int a1, int a2);
external void digitalWrite(int a1, int a2);
external void delay(int a1);
external uint8_t custom1Control;
external uint8_t custom2Control;
external uint8_t custom3Control;
external CRGB hsv(int a1, int a2, int a3);
external CRGB rgb(int a1, int a2, int a3);
external uint8_t beatSin8(uint8_t a1, uint8_t a2, uint8_t a3);
external uint8_t inoise8(uint16_t a1, uint16_t a2, uint16_t a3);
external uint8_t random8();
external uint8_t sin8(uint8_t a1);
external uint8_t cos8(uint8_t a1);
external void sPC(uint16_t a1, CRGB a2);
external void sCFP(uint16_t a1, uint8_t a2, uint8_t a3);
external void fadeToBlackBy(uint8_t a1);
define width 32
define height 32
define NUM_LEDS 1024
*/