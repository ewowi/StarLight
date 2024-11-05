/*
   @title     StarBase
   @file      SysModUI.cpp
   @date      20241105
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModUI.h"
#include "SysModWeb.h"
#include "SysModModel.h"

SysModUI::SysModUI() :SysModule("UI") {
};

//serve index.htm
void SysModUI::setup() {
  SysModule::setup();

  parentVar = initSysMod(parentVar, name, 4101);

  JsonObject tableVar = initTable(parentVar, "loops", nullptr, true, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case onUI:
      ui->setComment(var, "Loops initiated by a variable");
      return true;
    default: return false;
  }});

  initText(tableVar, "variable", nullptr, 32, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case onSetValue:
      for (size_t rowNr = 0; rowNr < loopFunctions.size(); rowNr++)
        mdl->setValue(var, JsonString(loopFunctions[rowNr].var["id"], JsonString::Copied), rowNr);
      return true;
    default: return false;
  }});

  initNumber(tableVar, "#loops", UINT16_MAX, 0, 999, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case onSetValue:
      for (size_t rowNr = 0; rowNr < loopFunctions.size(); rowNr++)
        mdl->setValue(var, loopFunctions[rowNr].counter, rowNr);
      return true;
    case onLoop1s:
      callVarFun(var, UINT8_MAX, onSetValue); //set the value (WIP)
      for (VarLoop &varLoop : loopFunctions)
        varLoop.counter = 0;
      return true;
    default: return false;
  }});
}

void SysModUI::loop20ms() { //never more then 50 times a second!

  for (VarLoop &varLoop : loopFunctions) {
    if (millis() - varLoop.lastMillis >= varLoop.var["interval"].as<int>()) {
      varLoop.lastMillis = millis();

      varLoop.loopFun(varLoop.var, 1, onLoop); //rowNr..

      varLoop.counter++;
      // ppf("%s %u %u %d %d\n", varLoop->Variable(var).id(), varLoop->lastMillis, millis(), varLoop->interval, varLoop->counter);
    }
  }
}

JsonObject SysModUI::initVar(JsonObject parent, const char * id, const char * type, bool readOnly, VarFun varFun) {
  const char * parentId = parent["id"];
  if (!parentId) parentId = "m"; //m=module
  JsonObject var = mdl->findVar(parentId, id);

  //create new var
  if (var.isNull()) {
    // ppf("initVar new %s: %s.%s\n", type, parentId, id); //parentId not null otherwise crash
    if (parent.isNull()) {
      JsonArray vars = mdl->model->as<JsonArray>();
      var = vars.add<JsonObject>();
    } else {
      if (parent["n"].isNull()) parent["n"].to<JsonArray>(); //TO!!! if parent exist and no "n" array, create it
      var = parent["n"].add<JsonObject>();
      // serializeJson(model, Serial);Serial.println();
    }
    var["id"] = JsonString(id, JsonString::Copied);
  }
  // else {
  //   ppf("initVar Var %s->%s already defined\n", modelParentId, id);
  // }

  if (!var.isNull()) {
    if (var["type"].isNull() || var["type"] != type) {
      var["type"] = JsonString(type, JsonString::Copied);
      // print->printJson("initVar set type", var);
    }

    Variable variable = Variable(var);

    var["pid"] = parentId;

    if (var["ro"].isNull() || variable.readOnly() != readOnly) variable.readOnly(readOnly);

    //set order. make order negative to check if not obsolete, see cleanUpModel
    if (variable.order() >= 1000) //predefined! (modules) - positive as saved in model.json
      variable.order( -variable.order()); //leave the order as is
    else {
      if (!parent.isNull() && Variable(parent).order() >= 0) // if checks on the parent already done so vars added later, e.g. controls, will be autochecked
        variable.order( mdl->varCounter++); //redefine order
      else
        variable.order( -mdl->varCounter++); //redefine order
    }

    //if varFun, add it to the list
    if (varFun) {
      //if fun already in ucFunctions then reuse, otherwise add new fun in ucFunctions
      //lambda update: when replacing typedef void(*UCFun)(JsonObject); with typedef std::function<void(JsonObject)> UCFun; this gives error:
      //  mismatched types 'T*' and 'std::function<void(ArduinoJson::V6213PB2::JsonObject)>' { return *__it == _M_value; }
      //  it also looks like functions are not added more then once anyway
      // std::vector<UCFun>::iterator itr = find(ucFunctions.begin(), ucFunctions.end(), varFun);
      // if (itr!=ucFunctions.end()) //found
      //   var["varFun"] = distance(ucFunctions.begin(), itr); //assign found function
      // else { //not found
        varFunctions.push_back(varFun); //add new function
        var["fun"] = varFunctions.size()-1;
      // }
      
      if (varFun(var, UINT8_MAX, onLoop)) { //test run if it supports loop
        //no need to check if already in...
        VarLoop loop;
        loop.loopFun = varFun;
        loop.var = var;

        loopFunctions.push_back(loop);
        var["loopFun"] = loopFunctions.size()-1;
        // ppf("iObject loopFun %s %u %u %d %d\n", Variable(var).id());
      }
    }
  }
  else
    ppf("initVar could not find or create var %s with %s\n", id, type);

  return var;
}

void SysModUI::processJson(JsonVariant json) {
  if (json.is<JsonObject>()) //should be
  {
     //varFun adds object elements to json which would be processed in the for loop. So we freeze the original pairs in a vector and loop on this
    std::vector<JsonPair> pairs;
    for (JsonPair pair : json.as<JsonObject>()) { //iterate json elements
      pairs.push_back(pair);
    }

    for (JsonPair pair : pairs) { //iterate json elements
      const char * key = pair.key().c_str();
      JsonVariant value = pair.value();

      // commands
      if (pair.key() == "v") { //when called from jsonHandler
        // do nothing as it is no real var but the verbose command of WLED
        ppf("processJson v type %s\n", pair.value().as<String>().c_str());
      }
      else if (pair.key() == "view" || pair.key() == "canvasData" || pair.key() == "theme") { //save the chosen view in System (see index.js)
        JsonObject var = mdl->findVar("m", "System");
        ppf("processJson %s v:%s n: %d s:%s\n", pair.key().c_str(), pair.value().as<String>().c_str(), var.isNull(), Variable(var).id());
        var[JsonString(key, JsonString::Copied)] = JsonString(value, JsonString::Copied); //this is needed as key can become a dangling pointer
        // json.remove(key); //key should stay as all clients use this to perform the changeHTML action
      }
      else if (pair.key() == "onAdd" || pair.key() == "onDelete") {
        if (value.is<JsonObject>()) {
          JsonObject command = value;
          JsonObject var = mdl->findVar(command["pid"], command["id"]);
          uint8_t rowNr = command["rowNr"].isNull()?UINT8_MAX:command["rowNr"];
          ppf("processJson %s - %s[%d]\n", key, Variable(var).id(), rowNr);

          callVarFun(var, rowNr, pair.key() == "onAdd"?onAdd:onDelete);

          //first remove the deleted row both on server and on client(s)
          if (pair.key() == "onDelete") {
            ppf("onDelete remove removeValuesForRow\n");
            Variable(var).removeValuesForRow(rowNr);
          }
        }
        // we need to send back the key so UI can add or delete the value
        // json.remove(key); //key processed we don't need the key in the response
      }
      else if (pair.key() == "onUI") { //JsonString can do ==
        //find the select var and collect it's options...
        if (value.is<JsonArray>()) { //should be
          for (JsonVariant varInArray: value.as<JsonArray>()) {
            char pid[32];
            strlcpy(pid, varInArray, sizeof(pid));
            char * id = strtok(pid, ".");
            if (id != NULL ) {
              strlcpy(pid, id, sizeof(pid)); //copy the id part
              id = strtok(NULL, "."); //the rest after .
            }

            JsonObject var = mdl->findVar(pid, id); //value is the id
            if (!var.isNull()) {
              callVarFun(var, UINT8_MAX, onUI);
              //sendDataWs done in caller of processJson
            }
            else
              ppf("dev processJson Command %s var %s not found\n", key, varInArray.as<String>().c_str());
          }
        } else
          ppf("dev processJson value not array? %s %s\n", key, value.as<String>().c_str());
        json.remove(key); //key processed we don't need the key in the response
      } 

     else if (!value.isNull()) { // {"varid": {"value":value}} or {"varid": value}
        JsonVariant newValue;
        if (value["value"].isNull()) // if no explicit value field (e.g. jsonHandler)
          newValue = value;
        else
          newValue = value["value"]; //use the value field

        char pidid[64];
        strlcpy(pidid, key, sizeof(pidid));
        //check if we deal with multiple rows (from table type)
        char * rowNrC = strtok(pidid, "#");
        if (rowNrC != NULL ) {
          strlcpy(pidid, rowNrC, sizeof(pidid)); //copy the pidid part
          rowNrC = strtok(NULL, "#"); //the rest after #
        }
        uint8_t rowNr = rowNrC?atoi(rowNrC):UINT8_MAX;

        char pid[64];
        strlcpy(pid, pidid, sizeof(pid));
        char * id = strtok(pid, ".");
        if (id != NULL ) {
          strlcpy(pid, id, sizeof(pid)); //copy the id part
          id = strtok(NULL, "."); //the rest after .
        }

        if (pid && id) {
          JsonObject var = mdl->findVar(pid, id);

          ppf("processJson var %s.%s", pid, id);
          if (rowNr != UINT8_MAX) ppf("[%d]", rowNr);
          ppf(" %s -> %s\n", var["value"].as<String>().c_str(), newValue.as<String>().c_str());

          if (!var.isNull())
          {
            //a button never sets the value
            if (var["type"] == "button") { //button always
              mdl->callVarOnChange(var, rowNr);
              if (rowNr != UINT8_MAX) web->getResponseObject()[pidid]["rowNr"] = rowNr;
            }
            else {
              mdl->setValueJV(var, newValue, rowNr);
              //we do need the response! to update multiple clients and also things within a client (e.g. systemName)
              // json.remove(key); //key / var["id"] processed we don't need the key in the response
              // print->printJson("setValueJV", web->getResponseObject());
            }
          }
          else
            ppf("dev Object %s[%d] not found\n", pidid, rowNr);
        }
        else
          ppf("dev processJson no pid.id found k:%s v:%s\n", key, value.as<String>().c_str());
      } 
      else {
        ppf("dev processJson command not recognized k:%s v:%s\n", key, value.as<String>().c_str());
      }
    } //for json pairs
  }
}