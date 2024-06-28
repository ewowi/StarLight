/*
   @title     StarBase
   @file      UserModE131.h
   @date      20240411
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include <ESPAsyncE131.h>

#define maxChannels 513

class UserModE131:public SysModule {

public:

  UserModE131() :SysModule("E131") {
    isEnabled = false; //default not enabled
  };

  void setup() {
    SysModule::setup();

    parentVar = ui->initUserMod(parentVar, name, 6201);

    ui->initNumber(parentVar, "dun", &universe, 0, 7, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "DMX Universe");
        return true;
      default: return false;
    }});

    JsonObject currentVar = ui->initNumber(parentVar, "dch", &channel, 1, 512, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "DMX Channel");
        ui->setComment(var, "First channel");
        return true;
      case onChange:
        for (JsonObject childVar: mdl->varChildren("e131Tbl"))
          ui->callVarFun(childVar, UINT8_MAX, onSetValue); //set the value (WIP)
        return true;
      default: return false;
    }});
    currentVar["dash"] = true;

    JsonObject tableVar = ui->initTable(parentVar, "e131Tbl", nullptr, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Vars to watch");
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "e131Channel", UINT16_MAX, 1, 512, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onSetValue:
        for (forUnsigned8 rowNr = 0; rowNr < varsToWatch.size(); rowNr++)
          mdl->setValue(var, channel + varsToWatch[rowNr].channelOffset, rowNr);
        return true;
      case onUI:
        ui->setLabel(var, "Channel");
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "e131Name", nullptr, 32, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onSetValue:
        for (forUnsigned8 rowNr = 0; rowNr < varsToWatch.size(); rowNr++)
          mdl->setValue(var, varsToWatch[rowNr].id, rowNr);
        return true;
      case onUI:
        ui->setLabel(var, "Name");
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "e131Max", UINT16_MAX, 0, UINT16_MAX, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onSetValue:
        for (forUnsigned8 rowNr = 0; rowNr < varsToWatch.size(); rowNr++)
          mdl->setValue(var, varsToWatch[rowNr].max, rowNr);
        return true;
      case onUI:
        ui->setLabel(var, "Max");
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "e131Value", UINT16_MAX, 0, 255, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onSetValue:
        for (forUnsigned8 rowNr = 0; rowNr < varsToWatch.size(); rowNr++)
          mdl->setValue(var, varsToWatch[rowNr].savedValue, rowNr);
        return true;
      case onUI:
        ui->setLabel(var, "Value");
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
                ppf(" varsToWatch: %s\n", varToWatch.id);
                mdl->setValue(varToWatch.id, varToWatch.savedValue%(varToWatch.max+1)); // TODO: ugly to have magic string 
              }
              else
                ppf("\n");
            }//!= savedValue
          }//if channel
        }//maxChannels
      } //for varToWatch
    } //!e131.isEmpty()
  } //loop

  void patchChannel(unsigned8 channelOffset, const char * id, unsigned8 max = 255) {
    VarToWatch varToWatch;
    varToWatch.channelOffset = channelOffset;
    varToWatch.id = id;
    varToWatch.savedValue = 0; // Always reset when (re)patching so variable gets set to DMX value even if unchanged
    varToWatch.max = max;
    varsToWatch.push_back(varToWatch);
  }

  // unsigned8 getValue(const char * id) {
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
      unsigned16 channelOffset;
      const char * id = nullptr;
      unsigned16 max = -1;
      unsigned8 savedValue = -1;
    };

    std::vector<VarToWatch> varsToWatch;

    ESPAsyncE131 e131;
    boolean e131Created = false;
    unsigned16 channel = 1;
    unsigned16 universe = 1;
    unsigned8 universeCount = 1;

};

extern UserModE131 *e131mod;