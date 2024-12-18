/*
   @title     StarBase
   @file      SysModPins.h
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

#include "Wire.h" //for I2S

#define pinTypeIO 0
#define pinTypeReadOnly 1
#define pinTypeReserved 2
#define pinTypeSpi 3
#define pinTypeInvalid UINT8_MAX

//info stored per pin
struct PinObject {
  char owner[32]; //if not "" then allocated (tbd: no use char)
  char details[32]; //info about pin usage
};

class SysModPins:public SysModule {

public:

  PinObject pinObjects[NUM_DIGITAL_PINS]; //all pins

  SysModPins();
  void setup() override;
  void loop20ms() override;

  void allocatePin(uint8_t pin, const char * owner, const char * details);
  void deallocatePin(uint8_t pin = UINT8_MAX, const char * owner = nullptr);
  bool isOwner(uint8_t pin, const char * owner) const {
    return strncmp(pinObjects[pin].owner, owner, sizeof(PinObject::owner)) == 0;
  }

  //temporary functions until we refactored the PinObject
  PinObject getNthAllocatedPinObject(uint8_t rowNr) {
    uint8_t n = 0;
    for (PinObject &pinObject:pinObjects) {
      if (strnlen(pinObject.owner, 32) > 0) {
        if (n == rowNr)
          return pinObject;
        n++;
      }
    }
    return PinObject();
  }
  uint8_t getNrOfAllocatedPins() {
    uint8_t n = 0;
    for (PinObject &pinObject:pinObjects) {
      if (strncmp(pinObject.owner, "", sizeof(PinObject::owner)) != 0) {
        n++;
      }
    }
    return n;
  }
  uint8_t getPinNr(uint8_t rowNr) {
    uint8_t pin = 0;
    uint8_t n = 0;
    for (PinObject &pinObject:pinObjects) {
      if (strncmp(pinObject.owner, "", sizeof(PinObject::owner)) != 0) {
        if (n == rowNr)
          return pin;
        n++;
      }
      pin++;
    }
    return UINT8_MAX;
  }

  bool pinsChanged = false; //update pins table if pins changed

  uint8_t getPinType(uint8_t pin) {
    uint8_t pinType;
    if (digitalPinIsValid(pin)) {

      #if defined(CONFIG_IDF_TARGET_ESP32S2)
        if ((pin > 18 && pin < 21) || (pin > 21 && pin < 33)) pinType = pinTypeReserved; else 
      #elif defined(CONFIG_IDF_TARGET_ESP32S3)
        if ((pin > 18 && pin < 21) || (pin > 21 && pin < 33)) pinType = pinTypeReserved; else 
      #elif defined(CONFIG_IDF_TARGET_ESP32C3)
        if ((pin > 11 && pin < 18) || (pin > 17 && pin < 20)) pinType = pinTypeReserved; else 
      #elif defined(ESP32)
        if (pin > 5 && pin < 12) pinType = pinTypeReserved; else 
      #else //???
        prf("dev unknown board\n");
        pinType = pinTypeInvalid; return pinType; 
      #endif

      if (!digitalPinCanOutput(pin)) 
        pinType = pinTypeReadOnly;
      else
        pinType = pinTypeIO;

      //results in crashes
      // if (digitalPinToRtcPin(pin)) pinType = pinTypeIO; else pinType = pinTypeInvalid; //error: 'RTC_GPIO_IS_VALID_GPIO' was not declared in this scope
      // if (digitalPinToDacChannel(pin)) pinType = pinTypeIO; else pinType = pinTypeInvalid; //error: 'DAC_CHANNEL_1_GPIO_NUM' was not declared in this scope

      //not so relevant
      // if (digitalPinToAnalogChannel(pin)) pinType = pinTypeInvalid;
      // if (digitalPinToTouchChannel(pin)) pinType = pinTypeInvalid;
    }
    else 
      pinType = pinTypeInvalid;

    return pinType;
  }

  bool initI2S () {
    //tbd: set pins in ui!!
    allocatePin(21, "Pins", "I2S SDA");
    allocatePin(22, "Pins", "I2S SCL");
    bool success = Wire.begin(21,22);
    ppf("initI2S Wire begin %s\n", success?"success":"failure");
    return success;
  }
};

extern SysModPins *pinsM;