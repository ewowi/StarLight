/*
   @title     StarBase
   @file      SysModWeb.cpp
   @date      20241219
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModWeb.h"
#include "SysModModel.h"
#include "SysModUI.h"
#include "SysModFiles.h"
#include "SysModules.h"
#include "SysModPins.h"
#include "SysModNetwork.h" //for localIP

#include "User/UserModMDNS.h"
// got multiple definition error here ??? see workaround below
// #ifdef STARBASE_USERMOD_LIVE
//   #include "../User/UserModLive.h"
// #endif

#include "html_ui.h"
#include "html_newui.h"
// #include "WWWData.h"

// #include <ArduinoOTA.h>

//https://techtutorialsx.com/2018/08/24/esp32-web-server-serving-html-from-file-system/
//https://randomnerdtutorials.com/esp32-async-web-server-espasyncwebserver-library/

SysModWeb::SysModWeb() :SysModule("Web") {
  // //CORS compatiblity
  // DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), "*");
  // DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Methods"), "*");
  // DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Headers"), "*");

  responseDocLoopTask = new JsonDocument; responseDocLoopTask->to<JsonObject>();
  responseDocAsyncTCP = new JsonDocument; responseDocAsyncTCP->to<JsonObject>();
};

void SysModWeb::setup() {
  SysModule::setup();
  const Variable parentVar = ui->initSysMod(Variable(), name, 3101);

  // Variable tableVar = ui->initTable(parentVar, "clients", nullptr, true, [](EventArguments) { switch (eventType) {
  //   case onLoop1s:
  //     for (JsonObject childVar: variable.children())
  //       Variable(childVar).triggerEvent(onSetValue); //set the value (WIP)
  //   default: return false;
  // }});

  // ui->initNumber(tableVar, "nr", UINT16_MAX, 0, 999, true, [this](EventArguments) { switch (eventType) {
  //   case onSetValue: {
  //     uint8_t rowNr = 0; for (auto &client:ws.getClients())
  //       variable.setValue(client->id(), rowNr++);
  //     return true; }
  //   default: return false;
  // }});

  // ui->initText(tableVar, "ip", nullptr, 16, true, [this](EventArguments) { switch (eventType) {
  //   case onSetValue: {
  //     uint8_t rowNr = 0; for (auto &client:ws.getClients())
  //       variable.setValue(JsonString(client->remoteIP().toString().c_str()), rowNr++);
  //     return true; }
  //   default: return false;
  // }});

  // //UINT8_MAX: tri state boolean: not true not false
  // ui->initCheckBox(tableVar, "full", UINT8_MAX, true, [this](EventArguments) { switch (eventType) {
  //   case onSetValue: {
  //     uint8_t rowNr = 0; for (auto &client:ws.getClients())
  //       variable.setValue(client->queueIsFull(), rowNr++);
  //     return true; }
  //   default: return false;
  // }});

  // ui->initSelect(tableVar, "status", UINT8_MAX, true, [this](EventArguments) { switch (eventType) {
  //   case onSetValue: {
  //     uint8_t rowNr = 0; for (auto &client:ws.getClients())
  //       variable.setValue(client->status(), rowNr++);
  //     return true; }
  //   case onUI:
  //   {
  //     //tbd: not working yet in ui
  //     JsonArray options = variable.setOptions();
  //     options.add("Disconnected"); //0
  //     options.add("Connected"); //1
  //     options.add("Disconnecting"); //2
  //     return true;
  //   }
  //   default: return false;
  // }});

  // ui->initNumber(tableVar, "length", UINT16_MAX, 0, WS_MAX_QUEUED_MESSAGES, true, [this](EventArguments) { switch (eventType) {
  //   case onSetValue: {
  //     uint8_t rowNr = 0; for (auto &client:ws.getClients())
  //       variable.setValue(client->queueLen(), rowNr++);
  //     return true; }
  //   default: return false;
  // }});

  // ui->initNumber(parentVar, "maxQueue", WS_MAX_QUEUED_MESSAGES, 0, WS_MAX_QUEUED_MESSAGES, true);

  ui->initText(parentVar, "WSSend", nullptr, 16, true, [this](EventArguments) { switch (eventType) {
    case onLoop1s:
      variable.setValueF("#: %d /s T: %d B/s B:%d B/s", sendWsCounter, sendWsTBytes, sendWsBBytes);
      sendWsCounter = 0;
      sendWsTBytes = 0;
      sendWsBBytes = 0;
    default: return false;
  }});

  ui->initText(parentVar, "WSRecv", nullptr, 16, true, [this](EventArguments) { switch (eventType) {
    case onLoop1s:
      variable.setValueF("#: %d /s %d B/s", recvWsCounter, recvWsBytes);
      recvWsCounter = 0;
      recvWsBytes = 0;
      return true;
    default: return false;
  }});

  ui->initText(parentVar, "UDPSend", nullptr, 16, true, [this](EventArguments) { switch (eventType) {
    case onLoop1s:
      variable.setValueF("#: %d /s %d B/s", sendUDPCounter, sendUDPBytes);
      sendUDPCounter = 0;
      sendUDPBytes = 0;
      return true;
    default: return false;
  }});

  ui->initText(parentVar, "UDPRecv", nullptr, 16, true, [this](EventArguments) { switch (eventType) {
    case onLoop1s:
      variable.setValueF("#: %d /s %d B/s", recvUDPCounter, recvUDPBytes);
      recvUDPCounter = 0;
      recvUDPBytes = 0;
    default: return false;
  }});

}

void SysModWeb::loop20ms() {

  //currently not used as each variable is send individually
  if (this->modelUpdated) {
    // sendDataWs(*mdl->model); //send new data, all clients, no def

    this->modelUpdated = false;
  }

  // if something changed in clients
  if (clientsChanged) {
    clientsChanged = false;

    // ppf("SysModWeb clientsChanged\n");
    for (JsonObject childVar: Variable("Web", "clients").children())
      Variable(childVar).triggerEvent(onSetValue); //set the value (WIP)
  }

}

void SysModWeb::loop1s() {
  // sendResponseObject(); //this sends all the loopTask responses once per second !!!
}

void SysModWeb::reboot() {
  ppf("SysModWeb reboot\n");
  // ws.closeAll(1012);
}

void SysModWeb::connectedChanged() {
  if (mdls->isConnected) {
    // ppf("%s server (re)started\n", name); //causes crash for some reason...
    ppf("connectedChanged: web server (re)started %s \n", name);
  }
  //else remove handlers...
}



// void SysModWeb::clientsToJson(JsonArray array, bool nameOnly, const char * filter) {
//   for (auto &client:ws.getClients()) {
//     if (nameOnly) {
//       array.add(JsonString(client->remoteIP().toString().c_str()));
//     } else {
//       // ppf("Client %d %d ...%d\n", client->id(), client->queueIsFull(), client->remoteIP()[3]);
//       JsonArray row = array.add<JsonArray>();
//       row.add(client->id());
//       array.add(JsonString(client->remoteIP().toString().c_str()));
//       row.add(client->queueIsFull());
//       row.add(client->status());
//       row.add(client->queueLen());
//     }
//   }
// }

JsonDocument * SysModWeb::getResponseDoc() {
  // ppf("response wsevent core %d %s\n", xPortGetCoreID(), pcTaskGetTaskName(nullptr));

  // return responseDocLoopTask;
  return strncmp(pcTaskGetTaskName(nullptr), "loopTask", 8) == 0?responseDocLoopTask:responseDocAsyncTCP;
}

JsonObject SysModWeb::getResponseObject() {
  return getResponseDoc()->as<JsonObject>();
}

void SysModWeb::serializeState(JsonVariant root) {
    const char* jsonState;// = "{\"transition\":7,\"ps\":9,\"pl\":-1,\"nl\":{\"on\":false,\"dur\":60,\"mode\":1,\"tbri\":0,\"rem\":-1},\"udpn\":{\"send\":false,\"recv\":true},\"lor\":0,\"mainseg\":0,\"seg\":[{\"id\":0,\"start\":0,\"stop\":144,\"len\":144,\"grp\":1,\"spc\":0,\"of\":0,\"on\":true,\"frz\":false,\"bri\":255,\"cct\":127,\"col\":[[182,15,98,0],[0,0,0,0],[255,224,160,0]],\"fx\":0,\"sx\":128,\"ix\":128,\"pal\":11,\"c1\":8,\"c2\":20,\"c3\":31,\"sel\":true,\"rev\":false,\"mi\":false,\"o1\":false,\"o2\":false,\"o3\":false,\"ssim\":0,\"mp12\":1}]}";
    jsonState = "{\"on\":true,\"bri\":60,\"transition\":7,\"ps\":1,\"pl\":-1,\"AudioReactive\":{\"on\":true},\"nl\":{\"on\":false,\"dur\":60,\"mode\":1,\"tbri\":0,\"rem\":-1},\"udpn\":{\"send\":false,\"recv\":true,\"sgrp\":1,\"rgrp\":1},\"lor\":0,\"mainseg\":0,\"seg\":[{\"id\":0,\"start\":0,\"stop\":16,\"startY\":0,\"stopY\":16,\"len\":16,\"grp\":1,\"spc\":0,\"of\":0,\"on\":true,\"frz\":false,\"bri\":255,\"cct\":127,\"set\":0,\"col\":[[255,160,0],[0,0,0],[0,255,200]],\"fx\":139,\"sx\":240,\"ix\":236,\"pal\":11,\"c1\":255,\"c2\":64,\"c3\":16,\"sel\":true,\"rev\":false,\"mi\":false,\"rY\":false,\"mY\":false,\"tp\":false,\"o1\":false,\"o2\":true,\"o3\":false,\"si\":0,\"m12\":0}],\"ledmap\":0}";
    JsonDocument docState;
    deserializeJson(root, jsonState);

    //tbd:  //StarBase has no idea about leds so this should be led independent
    root["bri"] = mdl->getValue("Fixture", "brightness");
    root["on"] = mdl->getValue("Fixture", "on").as<bool>();

}
void SysModWeb::serializeInfo(JsonVariant root) {
    const char * jsonInfo = "{\"ver\":\"0.14.1-b30.36\",\"rel\":\"abc_wled_controller_v43_M\",\"vid\":2402252,\"leds\":{\"count\":1024,\"countP\":1024,\"pwr\":1124,\"fps\":32,\"maxpwr\":9500,\"maxseg\":32,\"matrix\":{\"w\":32,\"h\":32},\"seglc\":[1],\"lc\":1,\"rgbw\":false,\"wv\":0,\"cct\":0},\"str\":false,\"name\":\"WLED-Wladi\",\"udpport\":21324,\"live\":false,\"liveseg\":-1,\"lm\":\"\",\"lip\":\"\",\"ws\":2,\"fxcount\":195,\"palcount\":75,\"cpalcount\":0,\"maps\":[{\"id\":0}],\"outputs\":[1024],\"wifi\":{\"bssid\":\"\",\"rssi\":0,\"signal\":100,\"channel\":1},\"fs\":{\"u\":20,\"t\":983,\"pmt\":0},\"ndc\":16,\"arch\":\"esp32\",\"core\":\"v3.3.6-16-gcc5440f6a2\",\"lwip\":0,\"totalheap\":294784,\"getflash\":4194304,\"freeheap\":115988,\"freestack\":6668,\"minfreeheap\":99404,\"e32core0code\":12,\"e32core0text\":\"SW restart\",\"e32core1code\":12,\"e32core1text\":\"SW restart\",\"e32code\":4,\"e32text\":\"SW error (panic or exception)\",\"e32model\":\"ESP32-D0WDQ5 rev.3\",\"e32cores\":2,\"e32speed\":240,\"e32flash\":4,\"e32flashspeed\":80,\"e32flashmode\":2,\"e32flashtext\":\" (DIO)\",\"uptime\":167796,\"opt\":79,\"brand\":\"WLED\",\"product\":\"MoonModules\"}";
    JsonDocument docInfo;

    deserializeJson(root, jsonInfo);

    root["name"] = mdl->getValue("System", "name");
    // docInfo["arch"] = "esp32"; //platformName

    // docInfo["rel"] = _INIT(TOSTRING(APP));
    // docInfo["ver"] = "0.0.1";
    // docInfo["vid"] = 2025121212; //WLED-native needs int otherwise status offline!!!
    // docInfo["leds"]["count"] = 999; //StarBase has no idea about leds
    // docInfo["leds"]["countP"] = 998;  //StarBase has no idea about leds
    // docInfo["leds"]["fps"] = mdl->getValue("fps"); //tbd: should be realFps but is ro var
    // docInfo["wifi"]["rssi"] = WiFi.RSSI();// mdl->getValue("rssi"); (ro)

    root["mac"] = JsonString(mdns->escapedMac.c_str());
    root["ip"] = JsonString(net->localIP().toString().c_str());
    // print->printJson("serveJson", root);
}

