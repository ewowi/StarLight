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

  SysModUI();

  //serve index.htm
  void setup();

  void loop20ms();

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

  Variable initTable(Variable parent, const char * id, const char * value = nullptr, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<const char *>(parent, id, "table", value, 0, 0, readOnly, varEvent);
  }

  Variable initText(Variable parent, const char * id, const char * value = nullptr, uint16_t max = 32, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<const char *>(parent, id, "text", value, 0, max, readOnly, varEvent);
  }

  //vector of text
  Variable initTextVector(Variable parent, const char * id, std::vector<VectorString> *values, uint16_t max = 32, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValueVector(parent, id, "text", values, 0, max, readOnly, varEvent);
  }

  Variable initFileUpload(Variable parent, const char * id, const char * value = nullptr, uint16_t max = 32, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<const char *>(parent, id, "fileUpload", value, 0, max, readOnly, varEvent);
  }
  Variable initPassword(Variable parent, const char * id, const char * value = nullptr, uint8_t max = 32, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<const char *>(parent, id, "password", value, 0, max, readOnly, varEvent);
  }

  //number is uint16_t for the time being (because it is called by uint16_t referenced vars...)
  Variable initNumber(Variable parent, const char * id, uint16_t value = UINT16_MAX, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<uint16_t>(parent, id, "number", value, min, max, readOnly, varEvent);
  }
  //init a number using referenced value
  Variable initNumber(Variable parent, const char * id, uint16_t * value = nullptr, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<uint16_t>(parent, id, "number", value, min, max, readOnly, varEvent);
  }
  //init a number using a vector of integers
  Variable initNumber(Variable parent, const char * id, std::vector<uint16_t> *values, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<uint16_t>(parent, id, "number", values, min, max, readOnly, varEvent);
  }

  Variable initPin(Variable parent, const char * id, uint8_t value = UINT8_MAX, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<uint8_t>(parent, id, "pin", value, 0, NUM_DIGITAL_PINS, readOnly, varEvent);
  }

  //referenced value
  Variable initPin(Variable parent, const char * id, uint8_t *value = nullptr, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<uint8_t>(parent, id, "pin", value, 0, NUM_DIGITAL_PINS, readOnly, varEvent);
  }

  //type int!
  Variable initProgress(Variable parent, const char * id, int value = UINT16_MAX, int min = 0, int max = 255, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<int>(parent, id, "progress", value, min, max, readOnly, varEvent);
  }

  Variable initCoord3D(Variable parent, const char * id, Coord3D value = {UINT16_MAX, UINT16_MAX, UINT16_MAX}, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<Coord3D>(parent, id, "coord3D", value, min, max, readOnly, varEvent);
  }
  //init a Coord3D using referenced value
  Variable initCoord3D(Variable parent, const char * id, Coord3D *value = nullptr, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<Coord3D>(parent, id, "coord3D", value, min, max, readOnly, varEvent);
  }

  //init a range slider, range between 0 and 255!
  Variable initSlider(Variable parent, const char * id, uint8_t value = UINT8_MAX, int min = 0, int max = 255, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<uint8_t>(parent, id, "range", value, min, max, readOnly, varEvent);
  }
  //init a range slider using referenced value
  Variable initSlider(Variable parent, const char * id, uint8_t * value = nullptr, int min = 0, int max = 255, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<uint8_t>(parent, id, "range", value, min, max, readOnly, varEvent);
  }

  Variable initCanvas(Variable parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<int>(parent, id, "canvas", value, 0, 0, readOnly, varEvent);
  }

  //supports 3 state value: if UINT8_MAX it is indeterminated
  Variable initCheckBox(Variable parent, const char * id, bool3State value = UINT8_MAX, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<bool3State>(parent, id, "checkbox", value, 0, 0, readOnly, varEvent);
  }
  //init a checkbox using referenced value
  Variable initCheckBox(Variable parent, const char * id, bool3State *value = nullptr, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<bool3State>(parent, id, "checkbox", value, 0, 0, readOnly, varEvent);
  }

  //a button never gets a value
  Variable initButton(Variable parent, const char * id, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<bool>(parent, id, "button", false, 0, 0, readOnly, varEvent);
  }
  // Variable initButton2(Variable parent, const char * id, bool readOnly = false, VarEvent varEvent = nullptr) {
  //   // return initVarAndValue<bool>(parent, id, "button", false, 0, 0, readOnly, varEvent);
  //   return JsonObject();
  // }

  //int value ?
  Variable initSelect(Variable parent, const char * id, uint8_t value = UINT8_MAX, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<uint8_t>(parent, id, "select", value, 0, 0, readOnly, varEvent);
  }
  //init a select using referenced value
  Variable initSelect(Variable parent, const char * id, uint8_t * value = nullptr, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<uint8_t>(parent, id, "select", value, 0, 0, readOnly, varEvent);
  }

  Variable initIP(Variable parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<int>(parent, id, "ip", value, 0, 255, readOnly, varEvent);
  }

  Variable initTextArea(Variable parent, const char * id, const char * value = nullptr, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<const char *>(parent, id, "textarea", value, 0, 0, readOnly, varEvent);
  }

  Variable initFileEdit(Variable parent, const char * id, const char * value = nullptr, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<const char *>(parent, id, "fileEdit", value, 0, 0, readOnly, varEvent);
  }

  //vector of fileEdit
  Variable initFileEditVector(Variable parent, const char * id, std::vector<VectorString> *values, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValueVector(parent, id, "fileEdit", values, 0, 0, readOnly, varEvent);
  }

  Variable initURL(Variable parent, const char * id, const char * value = nullptr, bool readOnly = false, VarEvent varEvent = nullptr) {
    return initVarAndValue<const char *>(parent, id, "url", value, 0, 0, readOnly, varEvent);
  }

  //initVarAndValue using basic value
  template <typename Type>
  Variable initVarAndValue(Variable parent, const char * id, const char * type, Type value, int min = 0, int max = 255, bool readOnly = true, VarEvent varEvent = nullptr) {
    Variable variable = initVar(parent, id, type, readOnly, varEvent);

    if (initValue(variable, min, max, 0)) { //no pointer
      variable.setValue(value, mdl->setValueRowNr); //does onChange if needed, if var in table, update the table row
    }

    return variable;
  }

  //initVarAndValue using referenced value
  template <typename Type>
  Variable initVarAndValue(Variable parent, const char * id, const char * type, Type * value, int min = 0, int max = 255, bool readOnly = true, VarEvent varEvent = nullptr) {
    Variable variable = initVar(parent, id, type, readOnly, varEvent);

    if (initValue(variable, min, max, int(value))) {
      variable.setValue(*value, mdl->setValueRowNr); //does onChange if needed, if var in table, update the table row
    }

    return variable;
  }

  //initVarAndValue using vector of values
  template <typename Type>
  Variable initVarAndValue(Variable parent, const char * id, const char * type, std::vector<Type> *values, int min = 0, int max = 255, bool readOnly = true, VarEvent varEvent = nullptr) {
    Variable variable = initVar(parent, id, type, readOnly, varEvent);

    if (!variable.var["value"].isNull()) {
      print->printJson("Clearing the vector", variable.var);
      (*values).clear(); // if values already then rebuild the vector with it
    }

    if (initValue(variable.var, min, max, (int)values)) {
      uint8_t rowNrL = 0;
      for (Type value: *values) { //loop over vector
        variable.setValue(value, rowNrL); //does onChange if needed, if var in table, update the table row
        rowNrL++;
      }
    }

    return variable;
  }

  //initVarAndValue using vector of values .  WIP!!!
  Variable initVarAndValueVector(Variable parent, const char * id, const char * type, std::vector<VectorString> *values, int min = 0, int max = 255, bool readOnly = true, VarEvent varEvent = nullptr) {
    Variable variable = initVar(parent, id, type, readOnly, varEvent);

    if (!variable.var["value"].isNull()) {
      print->printJson("Clearing the vector", variable.var);
      (*values).clear(); // if values already then rebuild the vector with it
    }

    if (initValue(variable, min, max, (int)values)) {
      uint8_t rowNrL = 0;
      for (VectorString value: *values) { //loop over vector
        variable.setValue(JsonString(value.s, JsonString::Copied), rowNrL); //does onChange if needed, if var in table, update the table row
        rowNrL++;
      }
    }

    return variable;
  }

  //adds a variable to the model
  Variable initVar(Variable parent, const char * id, const char * type, bool readOnly = true, VarEvent varEvent = nullptr);

  //gives a variable an initital value returns true if setValue must be called 
  bool initValue(Variable variable, int min = 0, int max = 255, int pointer = 0) {

    if (pointer != 0) {
      if (mdl->setValueRowNr == UINT8_MAX)
        variable.var["p"] = pointer; //store pointer!
      else
        variable.var["p"][mdl->setValueRowNr] = pointer; //store pointer in array!
      // ppf("initValue pointer stored %s: %s\n", variable.id(), (void *)(var["p"].as<String>().c_str()));
    }

    if (min) variable.var["min"] = min;
    if (max && max != UINT16_MAX) variable.var["max"] = max;

    //value needs update if varVal not set yet or varVal is array and setValueRowNr is not in it
    bool doSetValue = false;
    if (variable.var["type"] != "button") { //button never gets a value
      if (variable.var["value"].isNull()) {
        doSetValue = true;
        // print->printJson("initValue varEvent value is null", var);
      } else if (variable.var["value"].is<JsonArray>()) {
        JsonArray valueArray = variable.valArray();
        if (mdl->setValueRowNr != UINT8_MAX) { // if var in table
          if (mdl->setValueRowNr >= valueArray.size())
            doSetValue = true;
          else if (valueArray[mdl->setValueRowNr].isNull())
            doSetValue = true;
        }
      }
    }

    //sets the default values, by varEvent if exists, otherwise manually (by returning true)
    if (doSetValue) {
      bool onSetValueExists = false;
      if (!variable.var["fun"].isNull()) {
        onSetValueExists = variable.triggerEvent(onSetValue, mdl->setValueRowNr);
      }
      if (!onSetValueExists) { //setValue provided (if not null)
        return true;
      }
    }
    else { //do onChange on existing value
      //no call of onChange for buttons otherwise all buttons will be fired which is highly undesirable
      if (variable.var["type"] != "button") { // && !var["fun"].isNull(): also if no varEvent to update pointers   !isPointer because 0 is also a value then && (!isPointer || value)
        bool onChangeExists = false;
        if (variable.var["value"].is<JsonArray>()) {
          //refill the vector
          for (uint8_t rowNr = 0; rowNr < variable.valArray().size(); rowNr++) {
            onChangeExists |= variable.triggerEvent(onChange, rowNr, true); //init, also set var["p"]
          }
        }
        else {
          onChangeExists = variable.triggerEvent(onChange, mdl->setValueRowNr, true); //init, also set var["p"] 
        }

        if (onChangeExists)
          ppf("initValue onChange %s.%s <- %s\n", variable.pid(), variable.id(), variable.valueString().c_str());
      }
    }
    return false;
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
  std::vector<VarLoop> loopFunctions;

};

extern SysModUI *ui;