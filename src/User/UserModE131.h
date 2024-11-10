/*
   @title     StarBase
   @file      UserModE131.h
   @date      20241105
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include <ESPAsyncE131.h>

#include "SysModules.h"

#define maxChannels 513

class UserModE131:public SysModule {

public:

  UserModE131() :SysModule("E131") {
    isEnabled = false; //default not enabled
  };

  void setup() {
    SysModule::setup();

    Variable parentVar = ui->initUserMod(Variable(), name, 6201);

    ui->initNumber(parentVar, "universe", &universe, 0, 7);

    Variable currentVar = ui->initNumber(parentVar, "channel", &channel, 1, 512, false, [this](EventArguments) { switch (eventType) {
      case onUI:
        variable.setComment("First channel");
        return true;
      case onChange:
        for (JsonObject childVar: Variable(mdl->findVar("E131", "watches")).children())
          Variable(childVar).triggerEvent(onSetValue); //set the value (WIP)
        return true;
      default: return false;
    }});
    currentVar.var["dash"] = true;

    Variable tableVar = ui->initTable(parentVar, "watches", nullptr, true, [](EventArguments) { switch (eventType) {
      case onUI:
        variable.setComment("Variables to watch");
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "channel", UINT16_MAX, 1, 512, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        for (size_t rowNr = 0; rowNr < varsToWatch.size(); rowNr++)
          variable.setValue(channel + varsToWatch[rowNr].channelOffset, rowNr);
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "variable", nullptr, 32, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        for (size_t rowNr = 0; rowNr < varsToWatch.size(); rowNr++)
          variable.setValue(varsToWatch[rowNr].id, rowNr);
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "max", UINT16_MAX, 0, UINT16_MAX, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        for (size_t rowNr = 0; rowNr < varsToWatch.size(); rowNr++)
          variable.setValue(varsToWatch[rowNr].max, rowNr);
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "value", UINT16_MAX, 0, 255, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        for (size_t rowNr = 0; rowNr < varsToWatch.size(); rowNr++)
          variable.setValue(varsToWatch[rowNr].savedValue, rowNr);
        return true;
      default: return false;
    }});

  }

  // void connectedChanged() {
  //   setOnOff();
  // }

  // void enabledChanged() {
  //   setOnOff();
  // }

  void onOffChanged() {

    if (mdls->isConnected && isEnabled) {
      ppf("UserModE131::connected && enabled\n");

      if (e131Created) { // TODO: crashes here - no idea why!
        ppf("UserModE131 - ESPAsyncE131 created already\n");
        return;
      }
      ppf("UserModE131 - Create ESPAsyncE131\n");

      e131 = ESPAsyncE131(universeCount);
      if (this->e131.begin(E131_MULTICAST, universe, universeCount)) { // TODO: multicast igmp failing, so only works with unicast currently
        ppf("Network exists, begin e131.begin ok\n");
        success = true;
      }
      else {
        ppf("Network exists, begin e131.begin FAILED\n");
      }
      e131Created = true;
    }
    else {
      // e131.end();//???
      e131Created = false;
    }
  }

  void loop20ms() {
    if(!e131Created) {
      return;
    }
    if (!e131.isEmpty()) {
      e131_packet_t packet;
      e131.pull(&packet);     // Pull packet from ring buffer

      for (VarToWatch &varToWatch : varsToWatch) {
        for (int i=0; i < maxChannels; i++) {
          if (i == channel + varToWatch.channelOffset) {
            if (packet.property_values[i] != varToWatch.savedValue) {

              ppf("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH%d: %u -> %u",
                      htons(packet.universe),                 // The Universe for this packet
                      htons(packet.property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
                      e131.stats.num_packets,                 // Packet counter
                      e131.stats.packet_errors,               // Packet error counter
                      i,
                      varToWatch.savedValue,
                      packet.property_values[i]);             // Dimmer data for Channel i

              varToWatch.savedValue = packet.property_values[i];

              if (varToWatch.id != nullptr && varToWatch.max != 0) {
                ppf(" varsToWatch: %s.%s\n", varToWatch.pid, varToWatch.id);
                mdl->setValue(varToWatch.pid, varToWatch.id, varToWatch.savedValue%(varToWatch.max+1)); // TODO: ugly to have magic string 
              }
              else
                ppf("\n");
            }//!= savedValue
          }//if channel
        }//maxChannels
      } //for varToWatch
    } //!e131.isEmpty()
  } //loop

  void patchChannel(uint8_t channelOffset, const char * pid, const char * id, uint8_t max = 255) {
    VarToWatch varToWatch;
    varToWatch.channelOffset = channelOffset;
    varToWatch.pid = pid;
    varToWatch.id = id;
    varToWatch.savedValue = 0; // Always reset when (re)patching so variable gets set to DMX value even if unchanged
    varToWatch.max = max;
    varsToWatch.push_back(varToWatch);
  }

  // uint8_t getValue(const char * id) {
  //   for (int i=0; i < maxChannels; i++) {
  //       if(varsToWatch[i].id == id) {
  //         return varsToWatch[i].savedValue;
  //       }
  //   }
  //   ppf("ERROR: failed to find param %s\n", id);
  //   return 0;
  // }

  private:
    struct VarToWatch {
      uint16_t channelOffset;
      const char * pid = nullptr;
      const char * id = nullptr;
      uint16_t max = -1;
      uint8_t savedValue = -1;
    };

    std::vector<VarToWatch> varsToWatch;

    ESPAsyncE131 e131;
    boolean e131Created = false;
    uint16_t channel = 1;
    uint16_t universe = 1;
    uint8_t universeCount = 1;

};

extern UserModE131 *e131mod;