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

#include "SysModule.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModModel.h"
#include "SysModWeb.h"
#include "SysModSystem.h"

SysModPrint::SysModPrint() :SysModule("Print") {

#if ARDUINO_USB_CDC_ON_BOOT || !defined(CONFIG_IDF_TARGET_ESP32S2)
  Serial.begin(115200);
#else
  Serial.begin(115200, SERIAL_8N1, RX, TX);   // workaround for Lolin S2 mini - this board uses non-standard pins for RX and TX
#endif
  delay(500);
  // un-comment the next lines for redirecting kernel error messages to Serial
  #if CORE_DEBUG_LEVEL
  #if ARDUINO_USB_CDC_ON_BOOT
  Serial0.setDebugOutput(false);
  #endif
  Serial.setDebugOutput(true);
  #endif

  // softhack007: USB CDC needs a bit more time to initialize
#if ARDUINO_USB_CDC_ON_BOOT || ARDUINO_USB_MODE
  unsigned waitCounter = 0;
  do {
    delay(1000);
    waitCounter ++;
  } while ((!Serial) && (waitCounter < 8));  // wait until Serial is ready / connected
  delay(3000); // this extra delay avoids repeating disconnects on -s2 "Disconnected (ClearCommError failed"
  Serial.println("   **** COMMODORE BASIC V2 ****   ");
#endif
  if (!sys->sysTools_normal_startup() && Serial) { // only print if Serial is connected, and startup was not normal
    Serial.print("\nWARNING - possible crash: ");
    Serial.println(sys->sysTools_getRestartReason());
    Serial.println("");
  }
  Serial.println("Ready.\n");
  if (Serial) Serial.flush(); // drain output buffer
};

void SysModPrint::setup() {
  SysModule::setup();

  parentVar = ui->initSysMod(parentVar, name, 2302);

  //default to Serial
  ui->initSelect(parentVar, "pOut", 1, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
    {
      ui->setLabel(var, "Output");

      JsonArray options = ui->setOptions(var);
      options.add("No");
      options.add("Serial");
      options.add("UI");

      web->clientsToJson(options, true); //ip only
      return true;
    }
    default: return false;
  }});

  ui->initTextArea(parentVar, "log");
}

void SysModPrint::loop20ms() {
  if (!setupsDone) setupsDone = true;
}

void SysModPrint::printf(const char * format, ...) {
  va_list args;

  va_start(args, format);

  unsigned8 pOut = 1; //default serial
  char buffer[512]; //this is a lot for the stack - move to heap?
  vsnprintf(buffer, sizeof(buffer)-1, format, args);
  bool toSerial = false;
  
  if (mdls->isConnected) {
    if (mdl->model)
      pOut = mdl->getValue("pOut");

    if (pOut == 1) {
      toSerial = true;
    }
    else if (pOut == 2) {
      JsonObject responseObject = web->getResponseObject();
      if (responseObject["log"]["value"].isNull())
        responseObject["log"]["value"] = buffer;
      else
        responseObject["log"]["value"] = responseObject["log"]["value"].as<String>() + String(buffer);
      // web->addResponse("log", "value", JsonString(buffer, JsonString::Copied)); //setValue not necessary
      // mdl->setUIValueV("log", "%s", buffer);
    }
    else if (pOut == 3) {
      //tbd
    }
  }
  else
    toSerial = true;

  if (toSerial) {
    Serial.print(strncmp(pcTaskGetTaskName(NULL), "loopTask", 8) == 0?"":"α"); //looptask λ/ asyncTCP task α
    Serial.print(buffer);
  }

  va_end(args);
}

void SysModPrint::println(const __FlashStringHelper * x) {
  printf("%s\n", x);
}

void SysModPrint::printVar(JsonObject var) {
  char sep[3] = " ";
  for (JsonPair pair: var) {
    if (pair.key() == "id") {
      printf("%s%s", sep, pair.value().as<String>().c_str());
      strcpy(sep, ", ");
    }
    else if (pair.key() == "value") {
      printf(":%s", pair.value().as<String>().c_str());
    }
    else if (pair.key() == "n") {
      printf("[");
      for (JsonObject childVar: mdl->varChildren(var)) {
        printVar(childVar);
      }
      printf("]");
    }
  }
}

void SysModPrint::printJson(const char * text, JsonVariantConst source) {
  char resStr[1024];
  serializeJson(source, resStr, sizeof(resStr));

  printf("%s %s\n", text, resStr);
}

size_t SysModPrint::fFormat(char * buf, size_t size, const char * format, ...) {
  va_list args;
  va_start(args, format);

  size_t len = vsnprintf(buf, size, format, args);

  va_end(args);

  // Serial.printf("fFormat %s (%d)\n", buf, size);

  return len;
}

void SysModPrint::printJDocInfo(const char * text, JsonDocument source) {
  printf("%s (s:%u o:%u n:%u)\n", text, source.size(), source.overflowed(), source.nesting());
}