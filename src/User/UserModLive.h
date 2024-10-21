/*
   @title     StarBase
   @file      UserModLive.h
   @date      20241014
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
  std::string scPreBaseScript = ""; //externals etc generated (would prefer String for esp32...)

  UserModLive() :SysModule("LiveScripts") {
    isEnabled = false; //need to enable after fresh setup
  };

  void setup() override;

  void addExternals();

  void addExternalVal(std::string result, std::string name, void * ptr);

  void addExternalFun(std::string result, std::string name, std::string parameters, void * ptr);

  // void addExternalFun(string name, std::function<void(int)> fun);
  // void addExternalFun(string name, std::function<void(int, int)> fun);

  //testing class functions instead of static
  void showM();

  void loop();

  void loop20ms();

  void loop1s();

  void run(const char *fileName, const char * main = "main", const char * post = nullptr);

  void kill(const char * fileName = nullptr);

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
external uint8_t slider1;
external uint8_t slider2;
external uint8_t slider3;
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
define panel_width 32
*/