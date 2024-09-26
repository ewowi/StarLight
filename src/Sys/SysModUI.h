/*
   @title     StarBase
   @file      SysModUI.h
   @date      20240819
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
  onAdd,
  onDelete,
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
    JsonObject var = initVarAndValue<const char *>(parent, id, "appmod", (const char *)nullptr);
    if (order) Variable(var).defaultOrder(order + 1000);
    return var;
  }
  JsonObject initSysMod(JsonObject parent, const char * id, int order = 0) {
    JsonObject var = initVarAndValue<const char *>(parent, id, "sysmod", (const char *)nullptr);
    if (order) Variable(var).defaultOrder(order + 1000);
    return var;
  }
  JsonObject initUserMod(JsonObject parent, const char * id, int order = 0) {
    JsonObject var = initVarAndValue<const char *>(parent, id, "usermod", (const char *)nullptr);
    if (order) Variable(var).defaultOrder(order + 1000);
    return var;
  }

  JsonObject initTable(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<const char *>(parent, id, "table", value, 0, 0, readOnly, varFun);
  }

  JsonObject initText(JsonObject parent, const char * id, const char * value = nullptr, unsigned16 max = 32, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<const char *>(parent, id, "text", value, 0, max, readOnly, varFun);
  }

  //vector of text
  JsonObject initTextVector(JsonObject parent, const char * id, std::vector<VectorString> *values, unsigned16 max = 32, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValueVector(parent, id, "text", values, 0, max, readOnly, varFun);
  }

  JsonObject initFileUpload(JsonObject parent, const char * id, const char * value = nullptr, unsigned16 max = 32, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<const char *>(parent, id, "fileUpload", value, 0, max, readOnly, varFun);
  }
  JsonObject initPassword(JsonObject parent, const char * id, const char * value = nullptr, unsigned8 max = 32, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<const char *>(parent, id, "password", value, 0, max, readOnly, varFun);
  }

  //number is uint16_t for the time being (because it is called by uint16_t referenced vars...)
  JsonObject initNumber(JsonObject parent, const char * id, uint16_t value = UINT16_MAX, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<uint16_t>(parent, id, "number", value, min, max, readOnly, varFun);
  }
  //init a number using referenced value
  JsonObject initNumber(JsonObject parent, const char * id, uint16_t * value = nullptr, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<uint16_t>(parent, id, "number", value, min, max, readOnly, varFun);
  }
  //init a number using a vector of integers
  JsonObject initNumber(JsonObject parent, const char * id, std::vector<uint16_t> *values, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<uint16_t>(parent, id, "number", values, min, max, readOnly, varFun);
  }

  JsonObject initPin(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<int>(parent, id, "pin", value, 0, NUM_DIGITAL_PINS, readOnly, varFun);
  }

  JsonObject initProgress(JsonObject parent, const char * id, int value = UINT16_MAX, int min = 0, int max = 255, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<int>(parent, id, "progress", value, min, max, readOnly, varFun);
  }

  JsonObject initCoord3D(JsonObject parent, const char * id, Coord3D value = {UINT16_MAX, UINT16_MAX, UINT16_MAX}, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<Coord3D>(parent, id, "coord3D", value, min, max, readOnly, varFun);
  }
  //init a Coord3D using referenced value
  JsonObject initCoord3D(JsonObject parent, const char * id, Coord3D *value = nullptr, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<Coord3D>(parent, id, "coord3D", value, min, max, readOnly, varFun);
  }

  //init a range slider, range between 0 and 255!
  JsonObject initSlider(JsonObject parent, const char * id, int value = UINT16_MAX, int min = 0, int max = 255, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<int>(parent, id, "range", value, min, max, readOnly, varFun);
  }
  //init a range slider using referenced value
  JsonObject initSlider(JsonObject parent, const char * id, unsigned8 * value = nullptr, int min = 0, int max = 255, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<unsigned8>(parent, id, "range", value, min, max, readOnly, varFun);
  }

  JsonObject initCanvas(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<int>(parent, id, "canvas", value, 0, 0, readOnly, varFun);
  }

  //supports 3 state value: if UINT16_MAX it is indeterminated
  JsonObject initCheckBox(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<int>(parent, id, "checkbox", value, 0, 0, readOnly, varFun);
  }
  //init a checkbox using referenced value
  JsonObject initCheckBox(JsonObject parent, const char * id, bool * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<bool>(parent, id, "checkbox", value, 0, 0, readOnly, varFun);
  }

  //a button never gets a value
  JsonObject initButton(JsonObject parent, const char * id, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<bool>(parent, id, "button", false, 0, 0, readOnly, varFun);
  }

  //int value ?
  JsonObject initSelect(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<int>(parent, id, "select", value, 0, 0, readOnly, varFun);
  }
  //init a select using referenced value
  JsonObject initSelect(JsonObject parent, const char * id, unsigned8 * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<unsigned8>(parent, id, "select", value, 0, 0, readOnly, varFun);
  }

  JsonObject initIP(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<int>(parent, id, "ip", value, 0, 255, readOnly, varFun);
  }

  JsonObject initTextArea(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<const char *>(parent, id, "textarea", value, 0, 0, readOnly, varFun);
  }

  JsonObject initFileEdit(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<const char *>(parent, id, "fileEdit", value, 0, 0, readOnly, varFun);
  }

  //vector of fileEdit
  JsonObject initFileEditVector(JsonObject parent, const char * id, std::vector<VectorString> *values, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValueVector(parent, id, "fileEdit", values, 0, 0, readOnly, varFun);
  }

  JsonObject initURL(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndValue<const char *>(parent, id, "url", value, 0, 0, readOnly, varFun);
  }

  //initVarAndValue using basic value
  template <typename Type>
  JsonObject initVarAndValue(JsonObject parent, const char * id, const char * type, Type value, int min = 0, int max = 255, bool readOnly = true, VarFun varFun = nullptr) {
    JsonObject var = initVar(parent, id, type, readOnly, varFun);

    if (initValue(var, min, max, 0)) { //no pointer
      mdl->setValue(var, value, mdl->setValueRowNr); //does onChange if needed, if var in table, update the table row
    }

    return var;
  }

  //initVarAndValue using referenced value
  template <typename Type>
  JsonObject initVarAndValue(JsonObject parent, const char * id, const char * type, Type * value, int min = 0, int max = 255, bool readOnly = true, VarFun varFun = nullptr) {
    JsonObject var = initVar(parent, id, type, readOnly, varFun);

    if (initValue(var, min, max, int(value))) {
      mdl->setValue(var, *value, mdl->setValueRowNr); //does onChange if needed, if var in table, update the table row
    }

    return var;
  }

  //initVarAndValue using vector of values
  template <typename Type>
  JsonObject initVarAndValue(JsonObject parent, const char * id, const char * type, std::vector<Type> *values, int min = 0, int max = 255, bool readOnly = true, VarFun varFun = nullptr) {
    JsonObject var = initVar(parent, id, type, readOnly, varFun);

    if (!var["value"].isNull()) {
      print->printJson("Clearing the vector", var);
      (*values).clear(); // if values already then rebuild the vector with it
    }

    if (initValue(var, min, max, (int)values)) {
      uint8_t rowNrL = 0;
      for (Type value: *values) { //loop over vector
        mdl->setValue(var, value, rowNrL); //does onChange if needed, if var in table, update the table row
        rowNrL++;
      }
    }

    return var;
  }

  //initVarAndValue using vector of values .  WIP!!!
  JsonObject initVarAndValueVector(JsonObject parent, const char * id, const char * type, std::vector<VectorString> *values, int min = 0, int max = 255, bool readOnly = true, VarFun varFun = nullptr) {
    JsonObject var = initVar(parent, id, type, readOnly, varFun);

    if (!var["value"].isNull()) {
      print->printJson("Clearing the vector", var);
      (*values).clear(); // if values already then rebuild the vector with it
    }

    if (initValue(var, min, max, (int)values)) {
      uint8_t rowNrL = 0;
      for (VectorString value: *values) { //loop over vector
        mdl->setValue(var, JsonString(value.s, JsonString::Copied), rowNrL); //does onChange if needed, if var in table, update the table row
        rowNrL++;
      }
    }

    return var;
  }

  //adds a variable to the model
  JsonObject initVar(JsonObject parent, const char * id, const char * type, bool readOnly = true, VarFun varFun = nullptr);

  //gives a variable an initital value returns true if setValue must be called 
  bool initValue(JsonObject var, int min = 0, int max = 255, int pointer = 0) {
    Variable variable = Variable(var);

    if (pointer != 0) {
      if (mdl->setValueRowNr == UINT8_MAX)
        var["p"] = pointer; //store pointer!
      else
        var["p"][mdl->setValueRowNr] = pointer; //store pointer in array!
      ppf("initValue pointer stored %s: %s\n", variable.id(), (void *)(var["p"].as<String>().c_str()));
    }

    if (min) var["min"] = min;
    if (max && max != UINT16_MAX) var["max"] = max;

    //value needs update if varVal not set yet or varVal is array and setValueRowNr is not in it
    bool doSetValue = false;
    if (var["type"] != "button") { //button never gets a value
      if (var["value"].isNull()) {
        doSetValue = true;
        // print->printJson("initValue varFun value is null", var);
      } else if (var["value"].is<JsonArray>()) {
        JsonArray valueArray = variable.valArray();
        if (mdl->setValueRowNr != UINT8_MAX) { // if var in table
          if (mdl->setValueRowNr >= valueArray.size())
            doSetValue = true;
          else if (valueArray[mdl->setValueRowNr].isNull())
            doSetValue = true;
        }
      }
    }

    //sets the default values, by varFun if exists, otherwise manually (by returning true)
    if (doSetValue) {
      bool onSetValueExists = false;
      if (!var["fun"].isNull()) {
        onSetValueExists = callVarFun(var, mdl->setValueRowNr, onSetValue);
      }
      if (!onSetValueExists) { //setValue provided (if not null)
        return true;
      }
    }
    else { //do onChange on existing value
      //no call of onChange for buttons otherwise all buttons will be fired which is highly undesirable
      if (var["type"] != "button" && !var["fun"].isNull() ) { //!isPointer because 0 is also a value then && (!isPointer || value)
        bool onChangeExists = false;
        if (var["value"].is<JsonArray>()) {
          //refill the vector
          for (uint8_t rowNr = 0; rowNr < variable.valArray().size(); rowNr++) {
            onChangeExists |= mdl->callVarOnChange(var, rowNr, true); //init, also set var["p"]
          }
        }
        else {
          onChangeExists = mdl->callVarOnChange(var, mdl->setValueRowNr, true); //init, also set var["p"]
        }

        if (onChangeExists)
          ppf("initValue onChange init %s[x] <- %s\n", variable.id(), variable.valueString().c_str());
      }
    }
    return false;
  }

  //checks if var has fun of type funType implemented by calling it and checking result (for onUI on RO var, also onSetValue is called)
  bool callVarFun(const char * varID, unsigned8 rowNr = UINT8_MAX, unsigned8 funType = onSetValue) {
    JsonObject var = mdl->findVar(varID);
    return callVarFun(var, rowNr, funType);
  }

  //checks if var has fun of type funType implemented by calling it and checking result (for onUI on RO var, also onSetValue is called)
  bool callVarFun(JsonObject var, unsigned8 rowNr = UINT8_MAX, unsigned8 funType = onSetValue) {
    Variable variable = Variable(var);
    bool result = false;

    if (funType == onAdd || funType == onDelete) {
      //do this before calling onDelete
      print->printJson("callVarFun add/del", var);
      //if delete, delete also from vector ...
      //find the columns of the table
      if (funType == onDelete) {
        for (JsonObject childVar: variable.children()) {
          int pointer;
          if (childVar["p"].is<JsonArray>())
            pointer = childVar["p"][rowNr];
          else
            pointer = childVar["p"];

          ppf("  delete vector [%d] %d\n", rowNr, pointer);

          if (pointer != 0) {

            if (childVar["type"] == "select" || childVar["type"] == "checkbox" || childVar["type"] == "range") {
              std::vector<uint16_t> *valuePointer = (std::vector<uint16_t> *)pointer;
              (*valuePointer).erase((*valuePointer).begin() + rowNr);
            } else if (childVar["type"] == "number") {
              std::vector<uint16_t> *valuePointer = (std::vector<uint16_t> *)pointer;
              (*valuePointer).erase((*valuePointer).begin() + rowNr);
            } else if (childVar["type"] == "coord3D") {
              std::vector<Coord3D> *valuePointer = (std::vector<Coord3D> *)pointer;
              (*valuePointer).erase((*valuePointer).begin() + rowNr);
            }
            else
              print->printJson("dev callVarFun onDelete type not supported yet", var);
          }
        }
      }
      web->getResponseObject()[funType==onAdd?"onAdd":"onDelete"]["rowNr"] = rowNr;
    }

    //call varFun if exists
    if (!var["fun"].isNull()) {//isNull needed here!
      size_t funNr = var["fun"];
      if (funNr < varFunctions.size()) {
        result = varFunctions[funNr](var, rowNr, funType);
        if (result && !variable.readOnly()) { //send rowNr = 0 if no rowNr
          //only print vars with a value and not onSetValue as that changes a lot due to insTbl clTbl etc (tbd)
          //don't print if onSetValue or oldValue is null
          if (funType != onSetValue && (!var["oldValue"].isNull() || ((rowNr != UINT8_MAX) && !var["oldValue"][rowNr].isNull()))) {
            ppf("%sFun %s", funType==onSetValue?"val":funType==onUI?"ui":funType==onChange?"ch":funType==onAdd?"add":funType==onDelete?"del":"other", Variable(var).id());
            if (rowNr != UINT8_MAX) {
              ppf("[%d] (", rowNr);
              if (funType == onChange) ppf("%s ->", var["oldValue"][rowNr].as<String>().c_str());
              ppf("%s)\n", var["value"][rowNr].as<String>().c_str());
            }
            else {
              ppf(" (");
              if (funType == onChange) ppf("%s ->", var["oldValue"].as<String>().c_str());
              ppf("%s)\n", variable.valueString().c_str());
            }
          }
        }
      }
      else    
        ppf("dev callVarFun function nr %s outside bounds %d >= %d\n", variable.id(), funNr, varFunctions.size());
    }

    //for ro variables, call onSetValue to add also the value in responseDoc (as it is not stored in the model)
    if (funType == onUI && variable.readOnly()) {
      callVarFun(var, rowNr, onSetValue);
    }

    return result; //varFun exists
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
    return web->getResponseObject()[Variable(var).id()]["options"];
  }
  void clearOptions(JsonObject var) {
    web->getResponseObject()[Variable(var).id()].remove("options");
  }

  //find options text in a hierarchy of options
  void findOptionsText(JsonObject var, uint8_t value, char * groupName, char * optionName) {
    uint8_t startValue = 0;
    bool optionsExisted = !web->getResponseObject()[Variable(var).id()]["options"].isNull();
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