/*
   @title     StarBase
   @file      SysModWeb.h
   @date      20241219
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "SysModule.h"
#include "SysModPrint.h"


class SysModWeb:public SysModule {

public:

  SemaphoreHandle_t wsMutex = xSemaphoreCreateMutex();

  uint8_t sendWsCounter = 0;
  uint16_t sendWsTBytes = 0;
  uint16_t sendWsBBytes = 0;
  uint8_t recvWsCounter = 0;
  uint16_t recvWsBytes = 0;
  uint8_t sendUDPCounter = 0;
  uint16_t sendUDPBytes = 0;
  uint8_t recvUDPCounter = 0;
  uint16_t recvUDPBytes = 0;

  bool isBusy = false;

  #ifdef STARBASE_USERMOD_LIVE
    char lastFileUpdated[30] = ""; //workaround!
  #endif

  SysModWeb();

  void setup() override;
  void loop20ms() override;
  void loop1s() override;

  void reboot() override;

  void connectedChanged() override;

  //send json to client or all clients
  
  //add an url to the webserver to listen to
  //mdl and WLED style state and info
  void serializeState(JsonVariant root);
  void serializeInfo(JsonVariant root);

  //Is this an IP?
  bool isIp(const String& str) {
    for (size_t i = 0; i < str.length(); i++) {
      int c = str.charAt(i);
      if (c != '.' && (c < '0' || c > '9')) {
        return false;
      }
    }
    return true;
  }

  template <typename Type>
  void addResponse(const JsonObject var, const char * key, Type value, const uint8_t rowNr = UINT8_MAX) {
    JsonObject responseObject = getResponseObject();
    // if (responseObject[id].isNull()) responseObject[id].to<JsonObject>();;
    char pidid[64];
    print->fFormat(pidid, sizeof(pidid), "%s.%s", var["pid"].as<const char *>(), var["id"].as<const char *>());
    if (rowNr == UINT8_MAX)
      responseObject[pidid][key] = value;
    else {
      if (!responseObject[pidid][key].is<JsonArray>())
        responseObject[pidid][key].to<JsonArray>();
      responseObject[pidid][key][rowNr] = value;
    }
  }

  void addResponse(const JsonObject var, const char * key, const char * format = nullptr, ...) {
    va_list args;
    va_start(args, format);

    char value[128];
    vsnprintf(value, sizeof(value)-1, format, args);

    va_end(args);

    addResponse(var, key, JsonString(value));
  }

  // void clientsToJson(JsonArray array, bool nameOnly = false, const char * filter = nullptr);

  //gets the right responseDoc, depending on which task you are in, alternative for requestJSONBufferLock
  JsonDocument * getResponseDoc();
  JsonObject getResponseObject();
  void sendResponseObject();

private:
  bool modelUpdated = false;

  bool clientsChanged = false;

  JsonDocument *responseDocLoopTask = nullptr;
  JsonDocument *responseDocAsyncTCP = nullptr;

};

extern SysModWeb *web;