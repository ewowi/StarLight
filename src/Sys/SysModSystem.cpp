/*
   @title     StarBase
   @file      SysModSystem.cpp
   @date      20241219
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModSystem.h"
#include "SysModPrint.h"
#include "SysModUI.h"
// #include "SysModWeb.h"
#include "SysModModel.h"
#include "User/UserModMDNS.h"

// #include <Esp.h>

// get the right RTC.H for each MCU
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
  #if CONFIG_IDF_TARGET_ESP32S2
    #include <esp32s2/rom/rtc.h>
  #elif CONFIG_IDF_TARGET_ESP32C3
    #include <esp32c3/rom/rtc.h>
  #elif CONFIG_IDF_TARGET_ESP32S3
    #include <esp32s3/rom/rtc.h>
  #elif CONFIG_IDF_TARGET_ESP32 // ESP32/PICO-D4
    #include <esp32/rom/rtc.h>
  #endif
#else // ESP32 Before IDF 4.0
  #include <rom/rtc.h>
#endif

SysModSystem::SysModSystem() :SysModule("System") {};

void SysModSystem::setup() {
  SysModule::setup();

  const Variable parentVar = ui->initSysMod(Variable(), name, 2000);
  parentVar.var["s"] = true; //setup

  ui->initText(parentVar, "name", _INIT(TOSTRING(APP)), 24, false, [this](EventArguments) { switch (eventType) {
    case onUI:
      variable.setComment("Instance name");
      return true;
    case onChange:
      char name[24];
      removeInvalidCharacters(name, variable.value());
      ppf("instance name stripped %s\n", name);
      variable.setValue(JsonString(name)); //update with stripped name
      mdns->resetMDNS(); // set the new name for mdns
      return true;
    default: return false;
  }});

  ui->initNumber(parentVar, "now", UINT16_MAX, 0, (unsigned long)-1, true, [this](EventArguments) { switch (eventType) {
    case onUI:
      variable.setComment("s");
      return true;
    case onLoop1s:
      variable.setValue(now/1000);
      return true;
    default: return false;
  }});

  ui->initNumber(parentVar, "timeBase", UINT16_MAX, 0, (unsigned long)-1, true, [this](EventArguments) { switch (eventType) {
    case onUI:
      variable.setComment("s");
      return true;
    case onLoop1s:
      variable.setValue((now<millis())? - (UINT32_MAX - timebase)/1000:timebase/1000);
      return true;
    default: return false;
  }});

  ui->initText(parentVar, "loops", nullptr, 16, true, [this](EventArguments) { switch (eventType) {
    case onUI:
      variable.setComment("Loops per second");
      return true;
    case onLoop1s:
      variable.setValue(loopCounter);
      loopCounter = 0;
      return true;
    default: return false;
  }});

  ui->initProgress(parentVar, "mainStack", 0, 0, getArduinoLoopTaskStackSize(), true, [this](EventArguments) { switch (eventType) {
    case onChange:
      variable.var["max"] = getArduinoLoopTaskStackSize(); //makes sense?
      web->addResponse(variable.var, "comment", "%d of %d B", sysTools_get_arduino_maxStackUsage(), getArduinoLoopTaskStackSize());
      return true;
    case onLoop1s:
      variable.setValue(sysTools_get_arduino_maxStackUsage());
      return true;
    default: return false;
  }});

  ui->initCheckBox(parentVar, "safeMode", &safeMode);

}

void SysModSystem::loop() {
  loopCounter++;
  now = millis() + timebase;
}

void SysModSystem::loop10s() {
  //heartbeat
  if (sys->now < 60000)
    ppf("❤️ http://%s\n", WiFi.localIP().toString().c_str());
  else
    ppf("❤️");
}

//from esptools.h - public
int SysModSystem::sysTools_get_arduino_maxStackUsage(void) {
  char * loop_taskname = pcTaskGetTaskName(loop_taskHandle);    // ask for name of the known task (to make sure we are still looking at the right one)

  if ((loop_taskHandle == nullptr) || (loop_taskname == nullptr) || (strncmp(loop_taskname, "loopTask", 9) != 0)) {
    loop_taskHandle = xTaskGetHandle("loopTask");              // need to look for the task by name. FreeRTOS docs say this is very slow, so we store the result for next time
  }

  if (loop_taskHandle != nullptr) return uxTaskGetStackHighWaterMark(loop_taskHandle); // got it !!
  else return -1;
}