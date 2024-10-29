/*
   @title     StarBase
   @file      SysModWeb.h
   @date      20241014
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "SysModule.h"
#include "SysModPrint.h"

#ifdef STARBASE_USE_Psychic
  #include <PsychicHttp.h>
#else
  #include <ESPAsyncWebServer.h>
#endif


#ifdef STARBASE_USE_Psychic
  #define WebRequest PsychicRequest
  #define WebClient PsychicWebSocketClient
  #define WebServer PsychicHttpServer
  #define WebSocket PsychicWebSocketHandler 
  #define WebResponse PsychicResponse
#else
  #define WebRequest AsyncWebServerRequest
  #define WebClient AsyncWebSocketClient
  #define WebServer AsyncWebServer
  #define WebSocket AsyncWebSocket 
  #define WebResponse AsyncWebServerResponse
#endif

class SysModWeb:public SysModule {

public:
  #ifdef STARBASE_USE_Psychic
    WebSocket ws = WebSocket();
    WebServer server = WebServer();
  #else
    WebSocket ws = WebSocket("/ws");
    WebServer server = WebServer(80);
  #endif

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

  #ifdef STARBASE_USERMOD_LIVE
    char lastFileUpdated[30] = ""; //workaround!
  #endif

  SysModWeb();

  void setup();
  void loop20ms();
  void loop1s();

  void reboot();

  void connectedChanged();

  void wsEvent(WebSocket * ws, WebClient * client, AwsEventType type, void * arg, byte *data, size_t len);
  
  //send json to client or all clients
  void sendDataWs(JsonVariant json = JsonVariant(), WebClient * client = nullptr);
  void sendDataWs(std::function<void(AsyncWebSocketMessageBuffer *)> fill, size_t len, bool isBinary, WebClient * client = nullptr);
  void sendBuffer(AsyncWebSocketMessageBuffer * wsBuf, bool isBinary, WebClient * client = nullptr, bool lossless = true);

  //add an url to the webserver to listen to
  void serveIndex(WebRequest *request);
  void serveNewUI(WebRequest *request);
  //mdl and WLED style state and info
  void serializeState(JsonVariant root);
  void serializeInfo(JsonVariant root);
  void serveJson(WebRequest *request);


  // curl -F 'data=@fixture1.json' 192.168.8.213/upload
  void serveUpload(WebRequest *request, const String& fileName, size_t index, byte *data, size_t len, bool final);
  // curl -s -F "update=@/Users/ewoudwijma/Developer/GitHub/ewowi/StarBase/.pio/build/esp32dev/firmware.bin" 192.168.8.102/update /dev/null &
  void serveUpdate(WebRequest *request, const String& fileName, size_t index, byte *data, size_t len, bool final);
  void serveFiles(WebRequest *request);

  //processJsonUrl handles requests send in javascript using fetch and from a browser or curl
  //try this !!!: curl -X POST "http://192.168.121.196/json" -d '{"pin2":false}' -H "Content-Type: application/json"
  //curl -X POST "http://4.3.2.1/json" -d '{"pin2":false}' -H "Content-Type: application/json"
  //curl -X POST "http://4.3.2.1/json" -d '{"brightness":20, "v":true}' -H "Content-Type: application/json"
  //curl -X POST "http://192.168.8.125/json" -d '{"effect":2}' -H "Content-Type: application/json"
  //curl -X POST "http://192.168.8.152/json" -d '{"nrOfLeds":2000}' -H "Content-Type: application/json"

  //handle "v" and processJson (on /json)
  void jsonHandler(WebRequest *request, JsonVariant json);

  //Is this an IP?
  bool isIp(String str) {
    for (size_t i = 0; i < str.length(); i++) {
      int c = str.charAt(i);
      if (c != '.' && (c < '0' || c > '9')) {
        return false;
      }
    }
    return true;
  }

  bool captivePortal(WebRequest *request);

  template <typename Type>
  void addResponse(JsonObject var, const char * key, Type value, uint8_t rowNr = UINT8_MAX) {
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

  void addResponse(JsonObject var, const char * key, const char * format = nullptr, ...) {
    va_list args;
    va_start(args, format);

    char value[128];
    vsnprintf(value, sizeof(value)-1, format, args);

    va_end(args);

    addResponse(var, key, JsonString(value, JsonString::Copied));
  }

  void clientsToJson(JsonArray array, bool nameOnly = false, const char * filter = nullptr);

  //gets the right responseDoc, depending on which task you are in, alternative for requestJSONBufferLock
  JsonDocument * getResponseDoc();
  JsonObject getResponseObject();
  void sendResponseObject(WebClient * client = nullptr);

  void printClient(const char * text, WebClient * client) {
    ppf("%s client: %d ip:%s q:%d l:%d s:%d (#:%d)\n", text, client?client->id():-1, client?client->remoteIP().toString().c_str():"", client->queueIsFull(), client->queueLen(), client->status(), client->server()->count());
    //status: { WS_DISCONNECTED, WS_CONNECTED, WS_DISCONNECTING }
  }

private:
  bool modelUpdated = false;

  bool clientsChanged = false;

  JsonDocument *responseDocLoopTask = nullptr;
  JsonDocument *responseDocAsyncTCP = nullptr;

};

extern SysModWeb *web;