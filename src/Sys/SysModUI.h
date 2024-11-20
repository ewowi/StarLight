/*
   @title     StarBase
   @file      SysModUI.h
   @date      20241105
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "SysModule.h"
#include "SysModPrint.h"
#include "SysModModel.h"

struct VarLoop {
  JsonObject var;
  VarEvent loopFun;
  unsigned long lastMillis = 0;
  unsigned long counter = 0;
};

class SysModUI: public SysModule {

public:

  std::vector<VarLoop> loopFunctions;

  SysModUI();

  //serve index.htm
  void setup() override;

  void loop20ms() override;

  //order: order%4 determines the column (WIP)
  Variable initAppMod(Variable parent, const char * id, int order = 0) {
    Variable variable = initVarAndValue<const char *>(parent, id, "appmod", (const char *)nullptr);
    if (order) variable.defaultOrder(order + 1000);
    return variable;
  }
  Variable initSysMod(Variable parent, const char * id, int order = 0) {
    Variable variable = initVarAndValue<const char *>(parent, id, "sysmod", (const char *)nullptr);
    if (order) variable.defaultOrder(order + 1000);
    return variable;
  }
  Variable initUserMod(Variable parent, const char * id, int order = 0) {
    Variable variable = initVarAndValue<const char *>(parent, id, "usermod", (const char *)nullptr);
    if (order) variable.defaultOrder(order + 1000);
    return variable;
  }

  Variable initTable(Variable parent, const char * id, const char * value = nullptr, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<const char *>(parent, id, "table", value, 0, 0, readOnly, varEvent);
  }

  Variable initText(Variable parent, const char * id, const char * value = nullptr, uint16_t max = 32, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<const char *>(parent, id, "text", value, 0, max, readOnly, varEvent);
  }

  //vector of text
  Variable initTextVector(Variable parent, const char * id, std::vector<VectorString> *values, uint16_t max = 32, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValueVector(parent, id, "text", values, 0, max, readOnly, varEvent);
  }

  Variable initFileUpload(Variable parent, const char * id, const char * value = nullptr, uint16_t max = 32, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<const char *>(parent, id, "fileUpload", value, 0, max, readOnly, varEvent);
  }
  Variable initPassword(Variable parent, const char * id, const char * value = nullptr, uint8_t max = 32, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<const char *>(parent, id, "password", value, 0, max, readOnly, varEvent);
  }

  //number is uint16_t for the time being (because it is called by uint16_t referenced vars...)
  Variable initNumber(Variable parent, const char * id, uint16_t value = UINT16_MAX, int min = 0, int max = UINT16_MAX, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<uint16_t>(parent, id, "number", value, min, max, readOnly, varEvent);
  }
  //init a number using referenced value
  Variable initNumber(Variable parent, const char * id, uint16_t * value = nullptr, int min = 0, int max = UINT16_MAX, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<uint16_t>(parent, id, "number", value, min, max, readOnly, varEvent);
  }
  //init a number using a vector of integers
  Variable initNumber(Variable parent, const char * id, std::vector<uint16_t> *values, int min = 0, int max = UINT16_MAX, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<uint16_t>(parent, id, "number", values, min, max, readOnly, varEvent);
  }

  Variable initPin(Variable parent, const char * id, uint8_t value = UINT8_MAX, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<uint8_t>(parent, id, "pin", value, 0, NUM_DIGITAL_PINS, readOnly, varEvent);
  }

  //referenced value
  Variable initPin(Variable parent, const char * id, uint8_t *value = nullptr, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<uint8_t>(parent, id, "pin", value, 0, NUM_DIGITAL_PINS, readOnly, varEvent);
  }

  //type int!
  Variable initProgress(Variable parent, const char * id, int value = UINT16_MAX, int min = 0, int max = 255, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<int>(parent, id, "progress", value, min, max, readOnly, varEvent);
  }

  Variable initCoord3D(Variable parent, const char * id, Coord3D value = {UINT16_MAX, UINT16_MAX, UINT16_MAX}, int min = 0, int max = UINT16_MAX, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<Coord3D>(parent, id, "coord3D", value, min, max, readOnly, varEvent);
  }
  //init a Coord3D using referenced value
  Variable initCoord3D(Variable parent, const char * id, Coord3D *value = nullptr, int min = 0, int max = UINT16_MAX, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<Coord3D>(parent, id, "coord3D", value, min, max, readOnly, varEvent);
  }

  //init a range slider, range between 0 and 255!
  Variable initSlider(Variable parent, const char * id, uint8_t value = UINT8_MAX, int min = 0, int max = 255, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<uint8_t>(parent, id, "range", value, min, max, readOnly, varEvent);
  }
  //init a range slider using referenced value
  Variable initSlider(Variable parent, const char * id, uint8_t * value = nullptr, int min = 0, int max = 255, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<uint8_t>(parent, id, "range", value, min, max, readOnly, varEvent);
  }

  Variable initCanvas(Variable parent, const char * id, int value = UINT16_MAX, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<int>(parent, id, "canvas", value, 0, 0, readOnly, varEvent);
  }

  //supports 3 state value: if UINT8_MAX it is indeterminated
  Variable initCheckBox(Variable parent, const char * id, bool3State value = UINT8_MAX, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<bool3State>(parent, id, "checkbox", value, 0, 0, readOnly, varEvent);
  }
  //init a checkbox using referenced value
  Variable initCheckBox(Variable parent, const char * id, bool3State *value = nullptr, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<bool3State>(parent, id, "checkbox", value, 0, 0, readOnly, varEvent);
  }

  //a button never gets a value
  Variable initButton(Variable parent, const char * id, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<bool>(parent, id, "button", false, 0, 0, readOnly, varEvent);
  }
  // Variable initButton2(Variable parent, const char * id, bool readOnly = false, const VarEvent &varEvent = nullptr) {
  //   // return initVarAndValue<bool>(parent, id, "button", false, 0, 0, readOnly, varEvent);
  //   return JsonObject();
  // }

  //int value ?
  Variable initSelect(Variable parent, const char * id, uint8_t value = UINT8_MAX, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<uint8_t>(parent, id, "select", value, 0, 0, readOnly, varEvent);
  }
  //init a select using referenced value
  Variable initSelect(Variable parent, const char * id, uint8_t * value = nullptr, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<uint8_t>(parent, id, "select", value, 0, 0, readOnly, varEvent);
  }

  Variable initIP(Variable parent, const char * id, int value = UINT16_MAX, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<int>(parent, id, "ip", value, 0, 255, readOnly, varEvent);
  }

  Variable initTextArea(Variable parent, const char * id, const char * value = nullptr, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<const char *>(parent, id, "textarea", value, 0, 0, readOnly, varEvent);
  }

  Variable initFileEdit(Variable parent, const char * id, const char * value = nullptr, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<const char *>(parent, id, "fileEdit", value, 0, 0, readOnly, varEvent);
  }

  //vector of fileEdit
  Variable initFileEditVector(Variable parent, const char * id, std::vector<VectorString> *values, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValueVector(parent, id, "fileEdit", values, 0, 0, readOnly, varEvent);
  }

  Variable initURL(Variable parent, const char * id, const char * value = nullptr, bool readOnly = false, const VarEvent &varEvent = nullptr) {
    return initVarAndValue<const char *>(parent, id, "url", value, 0, 0, readOnly, varEvent);
  }

  //initVarAndValue using basic value
  template <typename Type>
  Variable initVarAndValue(Variable parent, const char * id, const char * type, Type value, int min = 0, int max = 255, bool readOnly = true, const VarEvent &varEvent = nullptr) {
    Variable variable = mdl->initVar(parent, id, type, readOnly, varEvent);

    if (variable.initValue(min, max, 0)) { //no pointer
      variable.setValue(value, mdl->setValueRowNr); //does onChange if needed, if var in table, update the table row
    }

    return variable;
  }

  //initVarAndValue using referenced value
  template <typename Type>
  Variable initVarAndValue(Variable parent, const char * id, const char * type, Type * value, int min = 0, int max = 255, bool readOnly = true, const VarEvent &varEvent = nullptr) {
    Variable variable = mdl->initVar(parent, id, type, readOnly, varEvent);

    if (variable.initValue(min, max, int(value))) {
      variable.setValue(*value, mdl->setValueRowNr); //does onChange if needed, if var in table, update the table row
    }

    return variable;
  }

  //initVarAndValue using vector of values
  template <typename Type>
  Variable initVarAndValue(Variable parent, const char * id, const char * type, std::vector<Type> *values, int min = 0, int max = 255, bool readOnly = true, const VarEvent &varEvent = nullptr) {
    Variable variable = mdl->initVar(parent, id, type, readOnly, varEvent);

    if (!variable.var["value"].isNull()) {
      print->printJson("Clearing the vector", variable.var);
      (*values).clear(); // if values already then rebuild the vector with it
    }

    if (variable.initValue(min, max, (int)values)) {
      uint8_t rowNrL = 0;
      for (Type value: *values) { //loop over vector
        variable.setValue(value, rowNrL); //does onChange if needed, if var in table, update the table row
        rowNrL++;
      }
    }

    return variable;
  }

  //initVarAndValue using vector of values .  WIP!!!
  Variable initVarAndValueVector(Variable parent, const char * id, const char * type, std::vector<VectorString> *values, int min = 0, int max = 255, bool readOnly = true, const VarEvent &varEvent = nullptr) {
    Variable variable = mdl->initVar(parent, id, type, readOnly, varEvent);

    if (!variable.var["value"].isNull()) {
      print->printJson("Clearing the vector", variable.var);
      (*values).clear(); // if values already then rebuild the vector with it
    }

    if (variable.initValue(min, max, (int)values)) {
      uint8_t rowNrL = 0;
      for (VectorString value: *values) { //loop over vector
        variable.setValue(JsonString(value.s, JsonString::Copied), rowNrL); //does onChange if needed, if var in table, update the table row
        rowNrL++;
      }
    }

    return variable;
  }

  // assuming Variable(varId).triggerEvent(onUI); has been called before
  uint8_t selectOptionToValue(const char *pidid, const char *label) {
    JsonArray options = web->getResponseObject()[pidid]["options"];
    // ppf("selectOptionToValue fileName %s %s\n", label, options[0].as<String>().c_str());
    uint8_t value = 0;
    for (JsonVariant option: options) {
      // ppf("selectOptionToValue fileName2 %s %s\n", label, option.as<String>().c_str());
      if (strnstr(option, label, 32) != nullptr) //if label part of value
        return value;
      value++;
    }
    return UINT8_MAX; //not found
  }

  //interpret json and run commands or set values like deserializeJson / deserializeState / deserializeConfig
  void processJson(JsonVariant json); //must be Variant, not object for jsonhandler

  //called to rebuild selects and tables (tbd: also label and comments is done again, that is not needed)
  // void processOnUI(const char * id);


private:

};

extern SysModUI *ui;