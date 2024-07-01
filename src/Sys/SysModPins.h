/*
   @title     StarBase
   @file      SysModPins.h
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
  void setup();
  void loop20ms();

  void allocatePin(unsigned8 pinNr, const char * owner, const char * details);
  void deallocatePin(unsigned8 pinNr, const char * owner);
  bool isOwner(unsigned8 pinNr, const char * owner) {
    return strcmp(pinObjects[pinNr].owner, owner) == 0;
  }

  //temporary functions until we refactored the PinObject
  PinObject getNthAllocatedPinObject(unsigned8 rowNr) {
    stackUnsigned8 n = 0;
    for (PinObject &pinObject:pinObjects) {
      if (strcmp(pinObject.owner, "") != 0) {
        if (n == rowNr)
          return pinObject;
        n++;
      }
    }
    return PinObject();
  }
  unsigned8 getNrOfAllocatedPins() {
    stackUnsigned8 n = 0;
    for (PinObject &pinObject:pinObjects) {
      if (strcmp(pinObject.owner, "") != 0) {
        n++;
      }
    }
    return n;
  }
  unsigned8 getPinNr(unsigned8 rowNr) {
    stackUnsigned8 pinNr = 0;
    stackUnsigned8 n = 0;
    for (PinObject &pinObject:pinObjects) {
      if (strcmp(pinObject.owner, "") != 0) {
        if (n == rowNr)
          return pinNr;
        n++;
      }
      pinNr++;
    }
    return UINT8_MAX;
  }

  bool pinsChanged = false; //update pins table if pins changed

  uint8_t getPinType(uint8_t pinNr) {
    uint8_t pinType;
    if (digitalPinIsValid(pinNr)) {

      #if defined(CONFIG_IDF_TARGET_ESP32S2)
        if ((pinNr > 18 && pinNr < 21) || (pinNr > 21 && pinNr < 33)) pinType = pinTypeReserved; else 
      #elif defined(CONFIG_IDF_TARGET_ESP32S3)
        if ((pinNr > 18 && pinNr < 21) || (pinNr > 21 && pinNr < 33)) pinType = pinTypeReserved; else 
      #elif defined(CONFIG_IDF_TARGET_ESP32C3)
        if ((pinNr > 11 && pinNr < 18) || (pinNr > 17 && pinNr < 20)) pinType = pinTypeReserved; else 
      #elif defined(ESP32)
        if (pinNr > 5 && pinNr < 12) pinType = pinTypeReserved; else 
      #else //???
        prf("dev unknown board\n");
        pinType = pinTypeInvalid; return pinType; 
      #endif

      if (!digitalPinCanOutput(pinNr)) 
        pinType = pinTypeReadOnly;
      else
        pinType = pinTypeIO;

      //results in crashes
      // if (digitalPinToRtcPin(pinNr)) pinType = pinTypeIO; else pinType = pinTypeInvalid; //error: 'RTC_GPIO_IS_VALID_GPIO' was not declared in this scope
      // if (digitalPinToDacChannel(pinNr)) pinType = pinTypeIO; else pinType = pinTypeInvalid; //error: 'DAC_CHANNEL_1_GPIO_NUM' was not declared in this scope

      //not so relevant
      // if (digitalPinToAnalogChannel(pinNr)) pinType = pinTypeInvalid;
      // if (digitalPinToTouchChannel(pinNr)) pinType = pinTypeInvalid;
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