/*
   @title     StarBase
   @file      SysModules.cpp
   @date      20240411
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModules.h"
#include "Sys/SysModPrint.h"
#include "Sys/SysModUI.h"
#include "Sys/SysModWeb.h"
#include "Sys/SysModModel.h"

SysModules::SysModules() {
};

void SysModules::setup() {
  for (SysModule *module:modules) {
    module->setup();
  }

  //delete mdlTbl values if nr of modules has changed (new values created using module defaults)
  for (JsonObject childVar: mdl->varChildren("mdlTbl")) {
    if (!childVar["value"].isNull() && mdl->varValArray(childVar).size() != modules.size()) {
      ppf("mdlTbl clear (%s %s) %d %d\n", childVar["id"].as<String>().c_str(), childVar["value"].as<String>().c_str(), modules.size(), mdl->varValArray(childVar).size());
      childVar.remove("value");
    }
  }

  //do its own setup: will be shown as last module
  JsonObject parentVar = ui->initSysMod(parentVar, "Modules", 4203);

  JsonObject tableVar = ui->initTable(parentVar, "mdlTbl", nullptr, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onUI:
      ui->setLabel(var, "Modules");
      ui->setComment(var, "List of modules");
      return true;
    default: return false;
  }});

  ui->initText(tableVar, "mdlName", nullptr, 32, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onSetValue:
      for (forUnsigned8 rowNr = 0; rowNr < modules.size(); rowNr++)
        mdl->setValue(var, JsonString(modules[rowNr]->name, JsonString::Copied), rowNr);
      return true;
    case onUI:
      ui->setLabel(var, "Name");
      return true;
    default: return false;
  }});

  //UINT16_MAX: no value set
  ui->initCheckBox(tableVar, "mdlSuccess", UINT16_MAX, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case onSetValue:
      for (forUnsigned8 rowNr = 0; rowNr < modules.size(); rowNr++)
        mdl->setValue(var, modules[rowNr]->success, rowNr);
      return true;
    case onUI:
      ui->setLabel(var, "Success");
      return true;
    default: return false;
  }});

  ui->initCheckBox(tableVar, "mdlEnabled", UINT16_MAX, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun not readonly! (tbd)
    case onSetValue:
      //never a rowNr as parameter, set all
      //execute only if var has not been set
      for (forUnsigned8 rowNr = 0; rowNr < modules.size(); rowNr++)
        mdl->setValue(var, modules[rowNr]->isEnabled, rowNr);
      return true;
    case onUI:
      //initially set to true, but as enabled are table cells, will be updated to an array
      ui->setLabel(var, "Enabled");
      return true;
    case onChange:
      if (rowNr != UINT8_MAX && rowNr < modules.size()) {
        modules[rowNr]->isEnabled = mdl->getValue(var, rowNr);
        modules[rowNr]->enabledChanged();
      }
      else {
        ppf(" no rowNr or %d > modules.size %d!!\n", rowNr, modules.size());
      }
      // print->printJson(" ", var);
      return true;
    default: return false;
  }});
}

void SysModules::loop() {
  // bool oneSec = false;
  // bool tenSec = false;
  // if (millis() - oneSecondMillis >= 1000) {
  //   oneSecondMillis = millis();
  //   oneSec = true;
  // }
  // if (millis() - tenSecondMillis >= 10000) {
  //   tenSecondMillis = millis();
  //   tenSec = true;
  // }
  for (SysModule *module:modules) {
    if (module->isEnabled && module->success) {
      module->loop();
      if (millis() - module->twentyMsMillis >= 20) {
        module->twentyMsMillis = millis();
        module->loop20ms();
      }
      if (millis() - module->oneSecondMillis >= 1000) {
        module->oneSecondMillis = millis();
        module->loop1s();
      }
      if (millis() - module->tenSecondMillis >= 10000) {
        module->tenSecondMillis = millis();
        module->loop10s();
      }
      // module->testManager();
      // module->performanceManager();
      // module->dataSizeManager();
      // module->codeSizeManager();
    }
  }
  if (newConnection) {
    newConnection = false;
    isConnected = true;
    connectedChanged();
  }

}

void SysModules::reboot() {
  for (SysModule *module:modules) {
    module->reboot();
  }
}

void SysModules::add(SysModule* module) {
  modules.push_back(module);
}

void SysModules::connectedChanged() {
  for (SysModule *module:modules) {
    module->connectedChanged();
  }
}