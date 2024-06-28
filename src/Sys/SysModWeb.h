/*
   @title     StarBase
   @file      SysModWeb.h
   @date      20240411
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

  unsigned8 sendWsCounter = 0;
  unsigned16 sendWsTBytes = 0;
  unsigned16 sendWsBBytes = 0;
  unsigned8 recvWsCounter = 0;
  unsigned16 recvWsBytes = 0;
  unsigned8 sendUDPCounter = 0;
  unsigned16 sendUDPBytes = 0;
  unsigned8 recvUDPCounter = 0;
  unsigned16 recvUDPBytes = 0;

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

  //add an url to the webserver to listen to
  void serveIndex(WebRequest *request);
  //mdl and WLED style state and info
  void serializeState(JsonObject root);
  void serializeInfo(JsonObject root);
  void serveJson(WebRequest *request);


  // curl -F 'data=@fixture1.json' 192.168.8.213/upload
  void serveUpload(WebRequest *request, const String& filename, size_t index, byte *data, size_t len, bool final);
  // curl -s -F "update=@/Users/ewoudwijma/Developer/GitHub/ewowi/StarBase/.pio/build/esp32dev/firmware.bin" 192.168.8.102/update /dev/null &
  void serveUpdate(WebRequest *request, const String& filename, size_t index, byte *data, size_t len, bool final);
  void serveFiles(WebRequest *request);

  //processJsonUrl handles requests send in javascript using fetch and from a browser or curl
  //try this !!!: curl -X POST "http://192.168.121.196/json" -d '{"pin2":false}' -H "Content-Type: application/json"
  //curl -X POST "http://4.3.2.1/json" -d '{"pin2":false}' -H "Content-Type: application/json"
  //curl -X POST "http://4.3.2.1/json" -d '{"bri":20, "v":true}' -H "Content-Type: application/json"
  //curl -X POST "http://192.168.8.125/json" -d '{"fx":2}' -H "Content-Type: application/json"
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
  void addResponse(const char * id, const char * key, Type value, unsigned8 rowNr = UINT8_MAX) {
    JsonObject responseObject = getResponseObject();
    // if (responseObject[id].isNull()) responseObject[id].to<JsonObject>();;
    if (rowNr == UINT8_MAX)
      responseObject[id][key] = value;
    else {
      if (!responseObject[id][key].is<JsonArray>())
        responseObject[id][key].to<JsonArray>();
      responseObject[id][key][rowNr] = value;
    }
  }

  JsonArray addResponseA(const char * id, const char * key) {
    JsonObject responseObject = getResponseObject();
    // if (responseObject[id].isNull()) responseObject[id].to<JsonObject>();;
    return responseObject[id][key].to<JsonArray>();
  }

  void addResponseV(const char * id, const char * key, const char * format, ...) {
    JsonObject responseObject = getResponseObject();
    // if (responseObject[id].isNull()) responseObject[id].to<JsonObject>();;

    va_list args;
    va_start(args, format);

    // size_t len = vprintf(format, args);
    char value[128];
    vsnprintf(value, sizeof(value)-1, format, args);

    va_end(args);

    responseObject[id][key] = JsonString(value, JsonString::Copied);
  }

  void clientsToJson(JsonArray array, bool nameOnly = false, const char * filter = nullptr);

  //gets the right responseDoc, depending on which task you are in, alternative for requestJSONBufferLock
  JsonDocument * getResponseDoc();
  JsonObject getResponseObject();
  void sendResponseObject(WebClient * client = nullptr);

  void printClient(const char * text, WebClient * client) {
    ppf("%s client: %d ...%d q:%d l:%d s:%d (#:%d)\n", text, client?client->id():-1, client?client->remoteIP()[3]:-1, client->queueIsFull(), client->queueLength(), client->status(), client->server()->count());
    //status: { WS_DISCONNECTED, WS_CONNECTED, WS_DISCONNECTING }
  }

private:
  bool modelUpdated = false;

  bool clientsChanged = false;

  JsonDocument *responseDocLoopTask = nullptr;
  JsonDocument *responseDocAsyncTCP = nullptr;

};

extern SysModWeb *web;