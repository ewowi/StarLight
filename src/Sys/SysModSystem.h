/*
   @title     StarBase
   @file      SysModSystem.h
   @date      20240411
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once

#include "SysModule.h"
#include "dependencies/Toki.h"

class SysModSystem:public SysModule {

public:
  char build[64] = "";
  char chipInfo[64] = "";

  Toki toki = Toki(); //Minimal millisecond accurate timekeeping.
  uint32_t
      now = millis(),
      timebase = 0;

  SysModSystem();
  void setup();
  void loop();
  void loop1s();
  void loop10s();

  //from esp32Tools
  bool sysTools_normal_startup(void);              // FALSE if unusual startup code --> use next function to get more info
  String sysTools_getRestartReason(void);          // long string including restart codes from system, Core#0 and Core#1 (if availeable)
  String sysTools_restart2String(int reasoncode);  // helper for SysModSystem::addRestartReasonsSelect. Returns "(#) ReasonText"
  String sysTools_reset2String(int resetCode);     // helper for SysModSystem::addResetReasonsSelect. Returns "shortResetReasonText (#)"
  int sysTools_get_arduino_maxStackUsage(void);    // to query max used stack of the arduino task. returns "-1" if unknown
  int sysTools_get_webserver_maxStackUsage(void);  // to query max used stack of the webserver task. returns "-1" if unknown

  //tbd: utility function ... (pka prepareHostname)
  void removeInvalidCharacters(char* hostname, const char *in)
  {
    const char *pC = in;
    uint8_t pos = 0;
    while (*pC && pos < 24) { // while !null and not over length
      if (isalnum(*pC)) {     // if the current char is alpha-numeric append it to the hostname
        hostname[pos] = *pC;
        pos++;
      } else if (*pC == ' ' || *pC == '_' || *pC == '-' || *pC == '+' || *pC == '!' || *pC == '?' || *pC == '*') {
        hostname[pos] = '-';
        pos++;
      }
      // else do nothing - no leading hyphens and do not include hyphens for all other characters.
      pC++;
    }
    //last character must not be hyphen
    if (pos > 5) {
      while (pos > 4 && hostname[pos -1] == '-') pos--;
      hostname[pos] = '\0'; // terminate string (leave at least "star")
    }
  }

private:
  unsigned long loopCounter = 0;

  void addResetReasonsSelect(JsonArray select);
  void addRestartReasonsSelect(JsonArray select);

  // from esp32Tools: helper fuctions
  int getCoreResetReason(int core);
  String resetCode2Info(int reason);
  esp_reset_reason_t getRestartReason();
  String restartCode2InfoLong(esp_reset_reason_t reason);
  String restartCode2Info(esp_reset_reason_t reason);

  TaskHandle_t loop_taskHandle = NULL;                   // to store the task handle for later calls
  TaskHandle_t tcp_taskHandle = NULL;                   // to store the task handle for later calls

};

extern SysModSystem *sys;