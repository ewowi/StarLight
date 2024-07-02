/*
   @title     StarBase
   @file      SysModSystem.cpp
   @date      20240411
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModSystem.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModWeb.h"
#include "SysModModel.h"
#include "SysModNetwork.h"
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
  parentVar = ui->initSysMod(parentVar, name, 2000);
  parentVar["s"] = true; //setup

  ui->initText(parentVar, "name", _INIT(TOSTRING(APP)), 24, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      // ui->setLabel(var, "Name");
      ui->setComment(var, "Instance name");
      return true;
    case onChange:
      char name[24];
      removeInvalidCharacters(name, var["value"]);
      ppf("instance name stripped %s\n", name);
      mdl->setValue(mdl->varID(var), JsonString(name, JsonString::Copied)); //update with stripped name
      mdns->resetMDNS(); // set the new name for mdns
      return true;
    default: return false;
  }});

  ui->initText(parentVar, "upTime", nullptr, 16, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setComment(var, "Uptime of board");
      return true;
    default: return false;
  }});

  ui->initNumber(parentVar, "now", UINT16_MAX, 0, (unsigned long)-1, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "now");
      return true;
    default: return false;
  }});

  ui->initNumber(parentVar, "timeBase", UINT16_MAX, 0, (unsigned long)-1, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "TimeBase");
      return true;
    default: return false;
  }});

  ui->initButton(parentVar, "reboot", false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onChange:
      web->ws.closeAll(1012);

      // mdls->reboot(); //not working yet
      // long dly = millis();
      // while (millis() - dly < 450) {
      //   yield();        // enough time to send response to client
      // }
      // FASTLED.clear();
      ESP.restart();
      return true;
    default: return false;
  }});

  ui->initText(parentVar, "loops", nullptr, 16, true);

  print->fFormat(chipInfo, sizeof(chipInfo)-1, "%s %s (%d.%d.%d) c#:%d %d mHz f:%d KB %d mHz %d", ESP.getChipModel(), ESP.getSdkVersion(), ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH, ESP.getChipCores(), ESP.getCpuFreqMHz(), ESP.getFlashChipSize()/1024, ESP.getFlashChipSpeed()/1000000, ESP.getFlashChipMode());
  ui->initText(parentVar, "chip", chipInfo, 16, true);

  ui->initProgress(parentVar, "heap", (ESP.getHeapSize()-ESP.getFreeHeap()) / 1000, 0, ESP.getHeapSize()/1000, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onChange:
      var["max"] = ESP.getHeapSize()/1000; //makes sense?
      web->addResponseV(var["id"], "comment", "f:%d / t:%d (l:%d) B", ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap());
      return true;
    default: return false;
  }});

  if (psramFound()) {
    ui->initProgress(parentVar, "psram", (ESP.getPsramSize()-ESP.getFreePsram()) / 1000, 0, ESP.getPsramSize()/1000, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        var["max"] = ESP.getPsramSize()/1000; //makes sense?
        web->addResponseV(var["id"], "comment", "%d / %d (%d) B", ESP.getFreePsram(), ESP.getPsramSize(), ESP.getMinFreePsram());
        return true;
      default: return false;
    }});
  }

  ui->initProgress(parentVar, "mainStack", sysTools_get_arduino_maxStackUsage(), 0, getArduinoLoopTaskStackSize(), true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Main stack");
      return true;
    case onChange:
      var["max"] = getArduinoLoopTaskStackSize(); //makes sense?
      web->addResponseV(var["id"], "comment", "%d of %d B", sysTools_get_arduino_maxStackUsage(), getArduinoLoopTaskStackSize());
      return true;
    default: return false;
  }});

  ui->initProgress(parentVar, "tcpStack", sysTools_get_webserver_maxStackUsage(), 0, CONFIG_ASYNC_TCP_STACK_SIZE, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "TCP stack");
      return true;
    case onChange:
      web->addResponseV(var["id"], "comment", "%d of %d B", sysTools_get_webserver_maxStackUsage(), CONFIG_ASYNC_TCP_STACK_SIZE);
      return true;
    default: return false;
  }});

  ui->initSelect(parentVar, "reset0", (int)rtc_get_reset_reason(0), true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Reset 0");
      ui->setComment(var, "Reason Core 0");
      addResetReasonsSelect(ui->setOptions(var));
      return true;
    default: return false;
  }});

  if (ESP.getChipCores() > 1)
    ui->initSelect(parentVar, "reset1", (int)rtc_get_reset_reason(1), true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Reset 1");
        ui->setComment(var, "Reason Core 1");
        addResetReasonsSelect(ui->setOptions(var));
        return true;
      default: return false;
    }});

  ui->initSelect(parentVar, "restartReason", (int)esp_reset_reason(), true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Restart");
      ui->setComment(var, "Reason restart");
      addRestartReasonsSelect(ui->setOptions(var));
      return true;
    default: return false;
  }});

  //calculate version in format YYMMDDHH
  //https://forum.arduino.cc/t/can-you-format-__date__/200818/10
  // int month, day, year, hour, minute, second;
  // const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  // sscanf(__DATE__, "%s %d %d", version, &day, &year); // Mon dd yyyy
  // month = (strstr(month_names, version)-month_names)/3+1;
  // sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second); //hh:mm:ss
  // print->fFormat(version, sizeof(version)-1, "%02d%02d%02d%02d", year-2000, month, day, hour);

  // ppf("version %s %s %s %d:%d:%d\n", version, __DATE__, __TIME__, hour, minute, second);

  strcat(build, _INIT(TOSTRING(APP)));
  strcat(build, "_");
  strcat(build, _INIT(TOSTRING(VERSION)));
  strcat(build, "_");
  strcat(build, _INIT(TOSTRING(PIOENV)));

  ui->initText(parentVar, "build", build, 32, true);
  // ui->initText(parentVar, "date", __DATE__, 16, true);
  // ui->initText(parentVar, "time", __TIME__, 16, true);

  ui->initFile(parentVar, "update", nullptr, UINT16_MAX, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "OTA Update");
      return true;
    default: return false;
  }});

  // char msgbuf[32];
  // snprintf(msgbuf, sizeof(msgbuf)-1, "%s rev.%d", ESP.getChipModel(), ESP.getChipRevision());
  // ui->initText(parentVar, "e32model")] = msgbuf;
  // ui->initText(parentVar, "e32cores")] = ESP.getChipCores();
  // ui->initText(parentVar, "e32speed")] = ESP.getCpuFreqMHz();
  // ui->initText(parentVar, "e32flash")] = int((ESP.getFlashChipSize()/1024)/1024);
  // ui->initText(parentVar, "e32flashspeed")] = int(ESP.getFlashChipSpeed()/1000000);
  // ui->initText(parentVar, "e32flashmode")] = int(ESP.getFlashChipMode());
  // switch (ESP.getFlashChipMode()) {
  //   // missing: Octal modes
  //   case FM_QIO:  ui->initText(parentVar, "e32flashtext")] = F(" (QIO)"); break;
  //   case FM_QOUT: ui->initText(parentVar, "e32flashtext")] = F(" (QOUT)");break;
  //   case FM_DIO:  ui->initText(parentVar, "e32flashtext")] = F(" (DIO)"); break;
  //   case FM_DOUT: ui->initText(parentVar, "e32flashtext")] = F(" (DOUT or other)");break;
  //   default: ui->initText(parentVar, "e32flashtext")] = F(" (other)"); break;
  // }
}

void SysModSystem::loop() {
  loopCounter++;
  now = millis() + timebase;
}

void SysModSystem::loop1s() {
  mdl->setUIValueV("upTime", "%lu s", millis()/1000);
  mdl->setUIValueV("now", "%lu s", now/1000);
  mdl->setUIValueV("timeBase", "%lu s", (now<millis())? - (UINT32_MAX - timebase)/1000:timebase/1000);
  mdl->setUIValueV("loops", "%lu /s", loopCounter);

  loopCounter = 0;
}
void SysModSystem::loop10s() {
  mdl->setValue("heap", (ESP.getHeapSize()-ESP.getFreeHeap()) / 1000);
  mdl->setValue("mainStack", sysTools_get_arduino_maxStackUsage());
  mdl->setValue("tcpStack", sysTools_get_webserver_maxStackUsage());

  if (psramFound()) {
    mdl->setValue("psram", (ESP.getPsramSize()-ESP.getFreePsram()) / 1000);
  }

  //heartbeat
  if (millis() < 60000)
    ppf("❤️ %s\n", WiFi.localIP().toString().c_str()); // show IP the first minute
  else
    ppf("❤️");
}

//replace code by sentence as soon it occurs, so we know what will happen and what not
void SysModSystem::addResetReasonsSelect(JsonArray options) {
  options.add(String("NO_MEAN (0)")); // 0,
  options.add(sysTools_reset2String( 1)); //POWERON_RESET"); // 1,    /**<1, */
  options.add(sysTools_reset2String( 2)); // 2,    /**<3, Software reset digital core*/
  options.add(sysTools_reset2String( 3)); // 3,    /**<3, Software reset digital core*/
  options.add(sysTools_reset2String( 4)); // 4,    /**<4, Legacy watch dog reset digital core*/
  options.add(sysTools_reset2String( 5)); // 5,    /**<3, Deep Sleep reset digital core*/
  options.add(sysTools_reset2String( 6)); // 6,    /**<6, Reset by SLC module, reset digital core*/
  options.add(sysTools_reset2String( 7)); // 7,    /**<7, Timer Group0 Watch dog reset digital core*/
  options.add(sysTools_reset2String( 8)); // 8,    /**<8, Timer Group1 Watch dog reset digital core*/
  options.add(sysTools_reset2String( 9)); // 9,    /**<9, RTC Watch dog Reset digital core*/
  options.add(sysTools_reset2String(10)); //10,    /**<10, Instrusion tested to reset CPU*/
  options.add(sysTools_reset2String(11)); //11,    /**<11, Time Group reset CPU*/
  options.add(sysTools_reset2String(12)); //SW_CPU_RESET"); //12,    /**<12, */
  options.add(sysTools_reset2String(13)); //13,    /**<13, RTC Watch dog Reset CPU*/
  options.add(sysTools_reset2String(14)); //EXT_CPU_RESET"); //14,    /**<14, */
  options.add(sysTools_reset2String(15)); //15,    /**<15, Reset when the vdd voltage is not stable*/
  options.add(sysTools_reset2String(16)); //16     /**<16, RTC Watch dog reset digital core and rtc module*/
  // codes below are only used on -S3/-S2/-C3
  options.add(sysTools_reset2String(17)); //17     /* Time Group1 reset CPU */
  options.add(sysTools_reset2String(18)); //18     /* super watchdog reset digital core and rtc module */
  options.add(sysTools_reset2String(19)); //19     /* glitch reset digital core and rtc module */
  options.add(sysTools_reset2String(20)); //20     /* efuse reset digital core */
  options.add(sysTools_reset2String(21)); //21     /* usb uart reset digital core */
  options.add(sysTools_reset2String(22)); //22     /* usb jtag reset digital core */
  options.add(sysTools_reset2String(23)); //23     /* power glitch reset digital core and rtc module */
}

//replace code by sentence as soon it occurs, so we know what will happen and what not
void SysModSystem::addRestartReasonsSelect(JsonArray options) {
  options.add(String("(0) ESP_RST_UNKNOWN"));//           //!< Reset reason can not be determined
  options.add(sysTools_restart2String( 1)); // ESP_RST_POWERON");//  //!< 
  options.add(sysTools_restart2String( 2)); // !< Reset by external pin (not applicable for ESP32)
  options.add(sysTools_restart2String( 3)); // ESP_RST_SW");//       //!< Software reset via esp_restart
  options.add(sysTools_restart2String( 4)); //ESP_RST_PANIC");//    //!< 
  options.add(sysTools_restart2String( 5)); //  //!< Reset (software or hardware) due to interrupt watchdog
  options.add(sysTools_restart2String( 6)); // //!< Reset due to task watchdog
  options.add(sysTools_restart2String( 7)); //      //!< Reset due to other watchdogs
  options.add(sysTools_restart2String( 8)); ////!< Reset after exiting deep sleep mode
  options.add(sysTools_restart2String( 9)); // //!< Brownout reset (software or hardware)
  options.add(sysTools_restart2String(10)); //     //!< Reset over SDIO
}

//from esptools.h - public

// check if estart was "normal"
bool SysModSystem::sysTools_normal_startup() {
  esp_reset_reason_t restartCode = getRestartReason();
  if ((restartCode == ESP_RST_POWERON) || (restartCode == ESP_RST_SW)) return true;  // poweron or esp_restart()
  return false;
}

// RESTART reason as long string
String SysModSystem::sysTools_getRestartReason() {
  esp_reset_reason_t restartCode = getRestartReason();
  String reasonText = restartCode2InfoLong(restartCode);
  String longText = String("(code ") + String((int)restartCode) + String( ") ") + reasonText;

  int core0code = getCoreResetReason(0);
  int core1code = getCoreResetReason(1);
  longText = longText + ". Core#0 (code " + String(core0code) + ") " + resetCode2Info(core0code);
  if (core1code > 0) 
    longText = longText + "; Core#1 (code " + String(core1code) + ") " + resetCode2Info(core1code);

  longText = longText + ".";
  return longText;
}

// helper for SysModSystem::addRestartReasonsSelect. Returns "(#) ReasonText"
String SysModSystem::sysTools_restart2String(int reasoncode) {
  esp_reset_reason_t restartCode = esp_reset_reason_t(reasoncode); // its a trick, not a sony ;-)
  String longText = String("(") + String(reasoncode) + String( ") ") + restartCode2Info(restartCode);
  return longText;
}

// helper for SysModSystem::addResetReasonsSelect. Returns "CoreResetReasonText (#)"
String SysModSystem::sysTools_reset2String(int resetCode) {
  String longText = resetCode2Info(resetCode) + String(" (") + String(resetCode) + String(")");
  return longText;
}

int SysModSystem::sysTools_get_arduino_maxStackUsage(void) {
  char * loop_taskname = pcTaskGetTaskName(loop_taskHandle);    // ask for name of the known task (to make sure we are still looking at the right one)

  if ((loop_taskHandle == NULL) || (loop_taskname == NULL) || (strncmp(loop_taskname, "loopTask", 8) != 0)) {
    loop_taskHandle = xTaskGetHandle("loopTask");              // need to look for the task by name. FreeRTOS docs say this is very slow, so we store the result for next time
  }

  if (loop_taskHandle != NULL) return uxTaskGetStackHighWaterMark(loop_taskHandle); // got it !!
  else return -1;
}

int SysModSystem::sysTools_get_webserver_maxStackUsage(void) {
  char * tcp_taskname = pcTaskGetTaskName(tcp_taskHandle);     // ask for name of the known task (to make sure we are still looking at the right one)

  if ((tcp_taskHandle == NULL) || (tcp_taskname == NULL) || (strncmp(tcp_taskname, "async_tcp", 9) != 0)) {
    tcp_taskHandle = xTaskGetHandle("async_tcp");              // need to look for the task by name. FreeRTOS docs say this is very slow, so we store the result for next time
  }

  if (tcp_taskHandle != NULL) return uxTaskGetStackHighWaterMark(tcp_taskHandle); // got it !!
  else return -1;
}


//from esptools.h - private

// helper fuctions
int SysModSystem::getCoreResetReason(int core) {
  if (core >= ESP.getChipCores()) return 0;
  return((int)rtc_get_reset_reason(core));
}

String SysModSystem::resetCode2Info(int reason) {
  switch(reason) {

    case 1 : //  1 =  Vbat power on reset
      return "power-on"; break;
    case 2 : // 2 = this code is not defined on ESP32
      return "exception"; break;
    case 3 : // 3 = Software reset digital core
      return "SW reset"; break;
    case 12: //12 = Software reset CPU
      return "SW restart"; break;
    case 5 : // 5 = Deep Sleep wakeup reset digital core
      return "wakeup"; break;
    case 14:  //14 = for APP CPU, reset by PRO CPU
      return "restart"; break;
    case 15: //15 = Reset when the vdd voltage is not stable (brownout)
      return "brown-out"; break;

    // watchdog resets
    case 4 : // 4 = Legacy watch dog reset digital core
    case 6 : // 6 = Reset by SLC module, reset digital core
    case 7 : // 7 = Timer Group0 Watch dog reset digital core
    case 8 : // 8 = Timer Group1 Watch dog reset digital core
    case 9 : // 9 = RTC Watch dog Reset digital core
    case 11: //11 = Time Group watchdog reset CPU
    case 13: //13 = RTC Watch dog Reset CPU
    case 16: //16 = RTC Watch dog reset digital core and rtc module
    case 17: //17 = Time Group1 reset CPU
      return "watchdog"; break;
    case 18: //18 = super watchdog reset digital core and rtc module
      return "super watchdog"; break;

    // misc
    case 10: // 10 = Instrusion tested to reset CPU
      return "intrusion"; break;
    case 19: //19 = glitch reset digital core and rtc module
      return "glitch"; break;
    case 20: //20 = efuse reset digital core
      return "EFUSE reset"; break;
    case 21: //21 = usb uart reset digital core
      return "USB UART reset"; break;
    case 22: //22 = usb jtag reset digital core
    return "JTAG reset"; break;
    case 23: //23 = power glitch reset digital core and rtc module
      return "power glitch"; break;

    // unknown reason code
    case 0:
      return "none"; break;
    default: 
      return "unknown"; break;
  }
}

esp_reset_reason_t SysModSystem::getRestartReason() {
  return(esp_reset_reason());
}
String SysModSystem::restartCode2InfoLong(esp_reset_reason_t reason) {
    switch (reason) {
      case ESP_RST_UNKNOWN:  return("Reset reason can not be determined"); break;
      case ESP_RST_POWERON:  return("Restart due to power-on event"); break;
      case ESP_RST_EXT:      return("Reset by external pin (not applicable for ESP32)"); break;
      case ESP_RST_SW:       return("Software restart via esp_restart()"); break;
      case ESP_RST_PANIC:    return("Software reset due to panic or unhandled exception (SW error)"); break;
      case ESP_RST_INT_WDT:  return("Reset (software or hardware) due to interrupt watchdog"); break;
      case ESP_RST_TASK_WDT: return("Reset due to task watchdog"); break;
      case ESP_RST_WDT:      return("Reset due to other watchdogs"); break;
      case ESP_RST_DEEPSLEEP:return("Restart after exiting deep sleep mode"); break;
      case ESP_RST_BROWNOUT: return("Brownout Reset (software or hardware)"); break;
      case ESP_RST_SDIO:     return("Reset over SDIO"); break;
    }
  return("unknown");
}

String SysModSystem::restartCode2Info(esp_reset_reason_t reason) {
    switch (reason) {
      case ESP_RST_UNKNOWN:  return("unknown reason"); break;
      case ESP_RST_POWERON:  return("power-on event"); break;
      case ESP_RST_EXT:      return("external pin reset"); break;
      case ESP_RST_SW:       return("SW restart by esp_restart()"); break;
      case ESP_RST_PANIC:    return("SW error - panic or exception"); break;
      case ESP_RST_INT_WDT:  return("interrupt watchdog"); break;
      case ESP_RST_TASK_WDT: return("task watchdog"); break;
      case ESP_RST_WDT:      return("other watchdog"); break;
      case ESP_RST_DEEPSLEEP:return("exit from deep sleep"); break;
      case ESP_RST_BROWNOUT: return("Brownout Reset"); break;
      case ESP_RST_SDIO:     return("Reset over SDIO"); break;
    }
  return("unknown");
}

