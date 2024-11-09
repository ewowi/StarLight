/*
   @title     StarBase
   @file      SysModPins.cpp
   @date      20241105
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
  for (int pin=0; pin<NUM_DIGITAL_PINS; pin++) {
    strlcpy(pinObjects[pin].owner, "", sizeof(PinObject::owner));  
    strlcpy(pinObjects[pin].details, "", sizeof(PinObject::details));  
  }
};

void SysModPins::setup() {
  SysModule::setup();
  Variable parentVar = ui->initSysMod(Variable(), name, 2202);

  //show table of allocated pins
  Variable tableVar = ui->initTable(parentVar, "pins", nullptr, true, [](EventArguments) { switch (eventType) {
    case onUI: {
      variable.setComment("Allocated Pins");
      return true; }
    default: return false;
  }});

  ui->initPin(tableVar, "pin", UINT8_MAX, true, [this](EventArguments) { switch (eventType) {
    case onSetValue:
      variable.var.remove("value");
      ppf("pin onSetValue %s %d\n", variable.valueString().c_str(), getNrOfAllocatedPins());
      for (uint8_t rowNr = 0; rowNr < getNrOfAllocatedPins(); rowNr++)
        variable.setValue(getPinNr(rowNr), rowNr);
      return true;
    default: return false;
  }});

  ui->initText(tableVar, "owner", nullptr, 32, true, [this](EventArguments) { switch (eventType) {
    case onSetValue:
      variable.var.remove("value");
      for (uint8_t rowNr = 0; rowNr < getNrOfAllocatedPins(); rowNr++)
        variable.setValue(JsonString(getNthAllocatedPinObject(rowNr).owner, JsonString::Copied), rowNr);
      return true;
    default: return false;
  }});

  ui->initText(tableVar, "details", nullptr, 256, true, [this](EventArguments) { switch (eventType) {
    case onSetValue:
      variable.var.remove("value");
      for (uint8_t rowNr = 0; rowNr < getNrOfAllocatedPins(); rowNr++) {
        // ppf("details[%d] d:%s\n", rowNr, getNthAllocatedPinObject(rowNr).details);
        variable.setValue(JsonString(getNthAllocatedPinObject(rowNr).details, JsonString::Copied), rowNr);
      }
      return true;
    default: return false;
  }});

  ui->initCanvas(parentVar, "board", UINT16_MAX, true, [](EventArguments) { switch (eventType) {
    case onUI:
      variable.setComment("Pin viewer 🚧");
      return true;
    case onLoop:
      if (!web->isBusy && web->ws.getClients().length()) {
        variable.var["interval"] = 100; //every 100 ms

        size_t len = NUM_DIGITAL_PINS + 5;
        AsyncWebSocketMessageBuffer *wsBuf= web->ws.makeBuffer(len); //global wsBuf causes crash in audio sync module!!!
        if (wsBuf) {
          wsBuf->lock();
          byte* buffer = wsBuf->get();
          // send pins to clients
          for (size_t pin = 0; pin < NUM_DIGITAL_PINS; pin++)
          {
            buffer[pin+5] = random(8);// digitalRead(pin) * 255; // is only 0 or 1
          }
          // ppf("\n");
          //new values
          buffer[0] = 0; //userFun id

          web->sendBuffer(wsBuf, true);

          wsBuf->unlock();
          web->ws._cleanBuffers();
        }
      }
      return true;

    default: return false;
  }});

#if CONFIG_IDF_TARGET_ESP32
  // softhack007: configuring these pins on S3/C3/S2 may cause major problems (crashes included)
  // pinMode(2, OUTPUT);  // softhack007 default LED pin on some boards, so don't play around with gpio2
  allocatePin(19, "Pins", "Relais");
  pinMode(19, OUTPUT); //tbd: part of allocatePin?

  ui->initCheckBox(parentVar, "pin19", true, false, [this](EventArguments) { switch (eventType) {
    case onUI:
      variable.setComment("🚧 used for relais on Serg shields");
      return true;
    case onChange: {
      bool pinValue = variable.value();

      ppf("onChange pin19 %s.%s:=%d\n", variable.pid(), variable.id(), pinValue);

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

    for (JsonObject childVar: Variable(mdl->findVar("Pins", "pins")).children())
      Variable(childVar).triggerEvent(onSetValue); //set the value (WIP)
  }
}

void SysModPins::allocatePin(uint8_t pin, const char * owner, const char * details) {
  ppf("allocatePin %d %s %s\n", pin, owner, details);
  if ((pin < NUM_DIGITAL_PINS) && (digitalPinIsValid(pin))) {
    if (strnlen(pinObjects[pin].owner, sizeof(PinObject::owner)) > 0 && strncmp(pinObjects[pin].owner, owner, sizeof(PinObject::owner)) != 0)
      ppf("allocatePin %d: not owner %s!=%s", pin, owner, pinObjects[pin].owner);
    else {
      strlcpy(pinObjects[pin].owner, owner, sizeof(PinObject::owner));
      strlcpy(pinObjects[pin].details, details, sizeof(PinObject::details));
      pinsChanged = true;
    }
  }
}

void SysModPins::deallocatePin(uint8_t pin, const char * owner) {
  for (int p = 0; p<NUM_DIGITAL_PINS; p++) {
    if (pin == UINT8_MAX || pin == p) {
      if (strncmp(pinObjects[p].owner, owner, sizeof(PinObject::owner)) != 0) {
        if (pin != UINT8_MAX)
          ppf("deallocatePin %d: not owner %s!=%s\n", p, owner, pinObjects[p].owner);
      } else {
        ppf("deallocatePin %d %s %s\n", p, owner, pinObjects[p].details);
        strlcpy(pinObjects[p].owner, "", sizeof(PinObject::owner));
        strlcpy(pinObjects[p].details, "", sizeof(PinObject::owner));
        pinsChanged = true;
      }
    }
  }
}