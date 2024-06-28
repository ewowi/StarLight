/*
   @title     StarBase
   @file      SysModPins.cpp
   @date      20240411
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModPins.h"
#include "SysModUI.h"
#include "SysModWeb.h"

SysModPins::SysModPins() :SysModule("Pins") {
  //start with no pins allocated
  for (int pinNr=0; pinNr<NUM_DIGITAL_PINS; pinNr++) {
    strcpy(pinObjects[pinNr].owner, "");  
    strcpy(pinObjects[pinNr].details, "");  
  }
};

void SysModPins::setup() {
  SysModule::setup();
  parentVar = ui->initSysMod(parentVar, name, 2202);

  //show table of allocated pins
  JsonObject tableVar = ui->initTable(parentVar, "pinTbl", nullptr, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI: {
      ui->setLabel(var, "Allocated Pins");
      return true; }
    default: return false;
  }});

  ui->initPin(tableVar, "pinNr", UINT16_MAX, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onSetValue:
      for (forUnsigned8 rowNr = 0; rowNr < getNrOfAllocatedPins(); rowNr++)
        mdl->setValue(var, getPinNr(rowNr), rowNr);
      return true;
    case onUI:
      ui->setLabel(var, "Pin");
      return true;
    default: return false;
  }});

  ui->initText(tableVar, "pinOwner", nullptr, 32, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onSetValue:
      for (forUnsigned8 rowNr = 0; rowNr < getNrOfAllocatedPins(); rowNr++)
        mdl->setValue(var, JsonString(getNthAllocatedPinObject(rowNr).owner, JsonString::Copied), rowNr);
      return true;
    case onUI:
      ui->setLabel(var, "Owner");
      return true;
    default: return false;
  }});

  ui->initText(tableVar, "pinDetails", nullptr, 256, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onSetValue:
      for (forUnsigned8 rowNr = 0; rowNr < getNrOfAllocatedPins(); rowNr++) {
        // ppf("pinDetails[%d] d:%s\n", rowNr, getNthAllocatedPinObject(rowNr).details);
        mdl->setValue(var, JsonString(getNthAllocatedPinObject(rowNr).details, JsonString::Copied), rowNr);
      }
      return true;
    case onUI:
      ui->setLabel(var, "Details");
      return true;
    default: return false;
  }});

  ui->initCanvas(parentVar, "board", UINT16_MAX, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Pin viewer");
      ui->setComment(var, "🚧");
      return true;
    case onLoop:
      var["interval"] = 100; //every 100 ms

      web->sendDataWs([](AsyncWebSocketMessageBuffer * wsBuf) {
        byte* buffer;

        buffer = wsBuf->get();

        // send pins to clients
        for (size_t pinNr = 0; pinNr < NUM_DIGITAL_PINS; pinNr++)
        {
          buffer[pinNr+5] = random(8);// digitalRead(pinNr) * 255; // is only 0 or 1
        }
        // ppf("\n");
        //new values
        buffer[0] = 0; //userFun id

      }, NUM_DIGITAL_PINS + 5, true);
      return true;

    default: return false;
  }});

#if CONFIG_IDF_TARGET_ESP32
  // softhack007: configuring these pins on S3/C3/S2 may cause major problems (crashes included)
  // pinMode(2, OUTPUT);  // softhack007 default LED pin on some boards, so don't play around with gpio2
  allocatePin(19, "Pins", "Relais");
  pinMode(19, OUTPUT); //tbd: part of allocatePin?

  ui->initCheckBox(parentVar, "pin19", true, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setComment(var, "🚧 used for relais on Serg shields");
      return true;
    case onChange: {
      bool pinValue = var["value"];

      ppf("onChange pin19 %s:=%d\n", mdl->varID(var), pinValue);

      // softhack007: writing these pins on S3/C3/S2 may cause major problems (crashes included)
      digitalWrite(19, pinValue?HIGH:LOW);
      return true; }
    default: return false;
  }});
#endif
}

void SysModPins::loop20ms() {

  if (pinsChanged) {
    pinsChanged = false;

    for (JsonObject childVar: mdl->varChildren("pinTbl"))
      ui->callVarFun(childVar, UINT8_MAX, onSetValue); //set the value (WIP)
  }
}

void SysModPins::allocatePin(unsigned8 pinNr, const char * owner, const char * details) {
  ppf("allocatePin %d %s %s\n", pinNr, owner, details);
  if ((pinNr < NUM_DIGITAL_PINS) && (digitalPinIsValid(pinNr))) {
    if (strcmp(pinObjects[pinNr].owner, "") != 0 && strcmp(pinObjects[pinNr].owner, owner) != 0)
      ppf("allocatePin %d: not owner %s!=%s", pinNr, owner, pinObjects[pinNr].owner);
    else {
      strncpy(pinObjects[pinNr].owner, owner, sizeof(PinObject::owner)-1);  
      strncpy(pinObjects[pinNr].details, details, sizeof(PinObject::details)-1);  
      pinsChanged = true;
    }
  }
}

void SysModPins::deallocatePin(unsigned8 pinNr, const char * owner) {
  ppf("deallocatePin %d %s\n", pinNr, owner);
  if (pinNr < NUM_DIGITAL_PINS) {
    if (strcmp(pinObjects[pinNr].owner, owner) != 0)
      ppf("deallocatePin %d: not owner %s!=%s", pinNr, owner, pinObjects[pinNr].owner);
    else {
      strcpy(pinObjects[pinNr].owner, "");  
      strcpy(pinObjects[pinNr].details, "");  
      pinsChanged = true;
    }
  }
}