/*
   @title     StarBase
   @file      SysModUI.h
   @date      20240227
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

enum FunTypes
{
  onSetValue,
  onUI,
  onChange,
  onLoop,
  onAddRow,
  onDeleteRow,
  f_count
};

// https://stackoverflow.com/questions/59111610/how-do-you-declare-a-lambda-function-using-typedef-and-then-use-it-by-passing-to
typedef std::function<unsigned8(JsonObject, unsigned8, unsigned8)> VarFun;

struct VarLoop {
  JsonObject var;
  VarFun loopFun;
  unsigned long lastMillis = 0;
  unsigned long counter = 0;
};

class SysModUI: public SysModule {

public:
  std::vector<VarFun> varFunctions;

  SysModUI();

  //serve index.htm
  void setup();

  void loop20ms();
  void loop1s();

  //order: order%4 determines the column (WIP)
  JsonObject initAppMod(JsonObject parent, const char * id, int order = 0) {
    JsonObject var = initVarAndUpdate<const char *>(parent, id, "appmod", (const char *)nullptr);
    if (order) mdl->varSetDefaultOrder(var, order + 1000);
    return var;
  }
  JsonObject initSysMod(JsonObject parent, const char * id, int order = 0) {
    JsonObject var = initVarAndUpdate<const char *>(parent, id, "sysmod", (const char *)nullptr);
    if (order) mdl->varSetDefaultOrder(var, order + 1000);
    return var;
  }
  JsonObject initUserMod(JsonObject parent, const char * id, int order = 0) {
    JsonObject var = initVarAndUpdate<const char *>(parent, id, "usermod", (const char *)nullptr);
    if (order) mdl->varSetDefaultOrder(var, order + 1000);
    return var;
  }

  JsonObject initTable(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "table", value, 0, 0, readOnly, varFun);
  }

  JsonObject initText(JsonObject parent, const char * id, const char * value = nullptr, unsigned16 max = 32, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "text", value, 0, max, readOnly, varFun);
  }

  JsonObject initFile(JsonObject parent, const char * id, const char * value = nullptr, unsigned16 max = 32, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "file", value, 0, max, readOnly, varFun);
  }
  JsonObject initPassword(JsonObject parent, const char * id, const char * value = nullptr, unsigned8 max = 32, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "password", value, 0, max, readOnly, varFun);
  }

  JsonObject initNumber(JsonObject parent, const char * id, int value = UINT16_MAX, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "number", value, min, max, readOnly, varFun);
  }
  //init a number using referenced value
  JsonObject initNumber(JsonObject parent, const char * id, uint16_t * value = nullptr, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<uint16_t>(parent, id, "number", value, min, max, readOnly, varFun);
  }

  JsonObject initPin(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "pin", value, 0, NUM_DIGITAL_PINS, readOnly, varFun);
  }

  JsonObject initProgress(JsonObject parent, const char * id, int value = UINT16_MAX, int min = 0, int max = 255, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "progress", value, min, max, readOnly, varFun);
  }

  JsonObject initCoord3D(JsonObject parent, const char * id, Coord3D value = {UINT16_MAX, UINT16_MAX, UINT16_MAX}, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<Coord3D>(parent, id, "coord3D", value, min, max, readOnly, varFun);
  }
  //init a Coord3D using referenced value
  JsonObject initCoord3D(JsonObject parent, const char * id, Coord3D *value = nullptr, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<Coord3D>(parent, id, "coord3D", value, min, max, readOnly, varFun);
  }

  //init a range slider, range between 0 and 255!
  JsonObject initSlider(JsonObject parent, const char * id, int value = UINT16_MAX, int min = 0, int max = 255, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "range", value, min, max, readOnly, varFun);
  }
  //init a range slider using referenced value
  JsonObject initSlider(JsonObject parent, const char * id, unsigned8 * value = nullptr, int min = 0, int max = 255, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<unsigned8>(parent, id, "range", value, min, max, readOnly, varFun);
  }

  JsonObject initCanvas(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "canvas", value, 0, 0, readOnly, varFun);
  }

  //supports 3 state value: if UINT16_MAX it is indeterminated
  JsonObject initCheckBox(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "checkbox", value, 0, 0, readOnly, varFun);
  }
  //init a checkbox using referenced value
  JsonObject initCheckBox(JsonObject parent, const char * id, bool * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<bool>(parent, id, "checkbox", value, 0, 0, readOnly, varFun);
  }

  //a button never gets a value
  JsonObject initButton(JsonObject parent, const char * id, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<bool>(parent, id, "button", false, 0, 0, readOnly, varFun);
  }

  //int value ?
  JsonObject initSelect(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "select", value, 0, 0, readOnly, varFun);
  }
  //init a select using referenced value
  JsonObject initSelect(JsonObject parent, const char * id, unsigned8 * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<unsigned8>(parent, id, "select", value, 0, 0, readOnly, varFun);
  }

  JsonObject initIP(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "ip", value, 0, 255, readOnly, varFun);
  }

  JsonObject initTextArea(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "textarea", value, 0, 0, readOnly, varFun);
  }

  JsonObject initURL(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "url", value, 0, 0, readOnly, varFun);
  }

  //initVarAndUpdate using referenced value
  template <typename Type>
  JsonObject initVarAndUpdate(JsonObject parent, const char * id, const char * type, Type * value, int min = 0, int max = 255, bool readOnly = true, VarFun varFun = nullptr) {
    JsonObject var = initVarAndUpdate(parent, id, type, *value, min, max, readOnly, varFun, (int)value); //call with extra parameter: int of the pointer
    return var;
  }

  template <typename Type>
  JsonObject initVarAndUpdate(JsonObject parent, const char * id, const char * type, Type value, int min = 0, int max = 255, bool readOnly = true, VarFun varFun = nullptr, int pointer = 0) {
    JsonObject var = initVar(parent, id, type, readOnly, varFun);
    if (pointer != 0) {
      if (mdl->setValueRowNr == UINT8_MAX)
        var["p"] = pointer; //store pointer!
      else
        var["p"][mdl->setValueRowNr] = pointer; //store pointer in array! 
      ppf("pointer stored %s: %s\n", id, (void *)(var["p"].as<String>().c_str()));
    }
    if (min) var["min"] = min;
    if (max && max != UINT16_MAX) var["max"] = max;

    bool valueNeedsUpdate = false;
    if (strcmp(type, "button") != 0) { //button never gets a value
      if (var["value"].isNull()) {
        valueNeedsUpdate = true;
        // print->printJson("initVarAndUpdate varFun value is null", var);
      } else if (var["value"].is<JsonArray>()) {
        JsonArray valueArray = var["value"].as<JsonArray>();
        if (mdl->setValueRowNr != UINT8_MAX) { // if var in table
          if (mdl->setValueRowNr >= valueArray.size())
            valueNeedsUpdate = true;
          else if (valueArray[mdl->setValueRowNr].isNull())
            valueNeedsUpdate = true;
        }
      }
    }

    if (valueNeedsUpdate) {
      bool onSetValueExists = false;
      if (varFun) {
        onSetValueExists = varFun(var, mdl->setValueRowNr, onSetValue);
      }
      if (!onSetValueExists) { //setValue provided (if not null)
        mdl->setValue(var, value, mdl->setValueRowNr); //does onChange if needed, if var in table, update the table row
      }
    }
    else { //do onChange on existing value
      //no call of onChange for buttons otherwise all buttons will be fired which is highly undesirable
      if (strcmp(type,"button") != 0 && varFun ) { //!isPointer because 0 is also a value then && (!isPointer || value)
        bool onChangeExists = false;
        if (var["value"].is<JsonArray>()) {
          int rowNr = 0;
          for (JsonVariant val:var["value"].as<JsonArray>()) {
            onChangeExists |= mdl->callVarChangeFun(var, rowNr++, true); //init, also set var["p"]
          }
        }
        else {
          onChangeExists = mdl->callVarChangeFun(var, UINT8_MAX, true); //init, also set var["p"]
        }

        if (onChangeExists)
          ppf("initVarAndUpdate onChange init %s[x] <- %s\n", mdl->varID(var), var["value"].as<String>().c_str());
      }
    }

    return var;
  }

  JsonObject initVar(JsonObject parent, const char * id, const char * type, bool readOnly = true, VarFun varFun = nullptr);

  //checks if var has fun of type funType implemented by calling it and checking result (for onUI on RO var, also onSetValue is called)
  bool callVarFun(const char * varID, unsigned8 rowNr = UINT8_MAX, unsigned8 funType = onSetValue) {
    JsonObject var = mdl->findVar(varID);
    return callVarFun(var, rowNr, funType);
  }

  //checks if var has fun of type funType implemented by calling it and checking result (for onUI on RO var, also onSetValue is called)
  bool callVarFun(JsonObject var, unsigned8 rowNr = UINT8_MAX, unsigned8 funType = onSetValue) {
    bool result = false;

    if (!var["fun"].isNull()) {//isNull needed here!
      size_t funNr = var["fun"];
      if (funNr < varFunctions.size()) {
        result = varFunctions[funNr](var, rowNr, funType);
        if (result && !mdl->varRO(var)) { //send rowNr = 0 if no rowNr
          //only print vars with a value and not onSetValue as that changes a lot due to insTbl clTbl etc (tbd)
          // if (!var["value"].isNull() && 
          if (funType != onSetValue) {
            ppf("%sFun %s", funType==onSetValue?"val":funType==onUI?"ui":funType==onChange?"ch":funType==onAddRow?"add":funType==onDeleteRow?"del":"other", mdl->varID(var));
            if (rowNr != UINT8_MAX) {
              ppf("[%d] (", rowNr);
              if (funType == onChange) ppf("%s ->", var["oldValue"][rowNr].as<String>().c_str());
              ppf("%s)\n", var["value"][rowNr].as<String>().c_str());
            }
            else {
              ppf(" (");
              if (funType == onChange) ppf("%s ->", var["oldValue"].as<String>().c_str());
              ppf("%s)\n", var["value"].as<String>().c_str());
            }
          }
        }
      }
      else    
        ppf("dev callVarFun function nr %s outside bounds %d >= %d\n", mdl->varID(var), funNr, varFunctions.size());
    }

    //for ro variables, call onSetValue to add also the value in responseDoc (as it is not stored in the model)
    if (funType == onUI && mdl->varRO(var)) {
      callVarFun(var, rowNr, onSetValue);
    }

    return result;
  }

  // assuming callVarFun(varID, UINT8_MAX, onUI); has been called before
  uint8_t selectOptionToValue(const char *varID, const char *label) {
    JsonArray options = web->getResponseObject()[varID]["options"];
    // ppf("selectOptionToValue fileName %s %s\n", label, options[0].as<String>().c_str());
    uint8_t value = 0;
    for (JsonVariant option: options) {
      // ppf("selectOptionToValue fileName2 %s %s\n", label, option.as<String>().c_str());
      if (strstr(option, label) != nullptr) //if label part of value
        return value;
      value++;
    }
    return UINT8_MAX; //not found
  }

  //interpret json and run commands or set values like deserializeJson / deserializeState / deserializeConfig
  void processJson(JsonVariant json); //must be Variant, not object for jsonhandler

  //called to rebuild selects and tables (tbd: also label and comments is done again, that is not needed)
  // void processOnUI(const char * id);

  void setLabel(JsonObject var, const char * text) {
    web->addResponse(var["id"], "label", text);
  }
  void setComment(JsonObject var, const char * text) {
    web->addResponse(var["id"], "comment", text);
  }
  JsonArray setOptions(JsonObject var) {
    return web->addResponseA(var["id"], "options");
  }
  //return the options from onUI (don't forget to clear responseObject)
  JsonArray getOptions(JsonObject var) {
    callVarFun(var, UINT8_MAX, onUI); //rebuild options
    return web->getResponseObject()[mdl->varID(var)]["options"];
  }
  void clearOptions(JsonObject var) {
    web->getResponseObject()[mdl->varID(var)].remove("options");
  }

  //find options text in a hierarchy of options
  void findOptionsText(JsonObject var, uint8_t value, char * groupName, char * optionName) {
    uint8_t startValue = 0;
    bool optionsExisted = !web->getResponseObject()[mdl->varID(var)]["options"].isNull();
    JsonString groupNameJS;
    JsonString optionNameJS;
    JsonArray options = getOptions(var);
    if (!findOptionsTextRec(options, &startValue, value, &groupNameJS, &optionNameJS))
      ppf("findOptions select option not found %d %s %s\n", value, groupNameJS.isNull()?"X":groupNameJS.c_str(), optionNameJS.isNull()?"X":optionNameJS.c_str());
    strcpy(groupName, groupNameJS.c_str());
    strcpy(optionName, optionNameJS.c_str());
    if (!optionsExisted)
      clearOptions(var); //if created here then also remove 
  }

  // (groupName and optionName as pointers? String is already a pointer?)
  bool findOptionsTextRec(JsonVariant options, uint8_t * startValue, uint8_t value, JsonString *groupName, JsonString *optionName, JsonString parentGroup = JsonString()) {
    if (options.is<JsonArray>()) { //array of options
      for (JsonVariant option : options.as<JsonArray>()) {
        if (findOptionsTextRec(option, startValue, value, groupName, optionName, parentGroup))
          return true;
      }
    }
    else if (options.is<JsonObject>()) { //group
      for (JsonPair pair: options.as<JsonObject>()) {
        if (findOptionsTextRec(pair.value(), startValue, value, groupName, optionName, parentGroup.isNull()?pair.key():parentGroup)) //send the master level group name only
          return true;
      }
    } else { //individual option
      if (*startValue == value) {
        *groupName = parentGroup;
        *optionName = options.as<JsonString>();
        ppf("Found %d=%d ? %s . %s\n", *startValue, value, (*groupName).isNull()?"":(*groupName).c_str(), (*optionName).isNull()?"":(*optionName).c_str());
        return true;
      }
      (*startValue)++;
    }
    return false;
  }

private:
  std::vector<VarLoop> loopFunctions;

};

extern SysModUI *ui;