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

  const Variable parentVar = initSysMod(Variable(), name, 4101);

  Variable tableVar = initTable(parentVar, "loops", nullptr, true, [](EventArguments) { switch (eventType) {
    case onUI:
      variable.setComment("Loops initiated by a variable");
      return true;
    default: return false;
  }});

  initText(tableVar, "variable", nullptr, 32, true, [this](EventArguments) { switch (eventType) {
    case onSetValue:
      for (size_t rowNr = 0; rowNr < loopFunctions.size(); rowNr++)
        variable.setValue(JsonString(loopFunctions[rowNr].variable.id()), rowNr);
      return true;
    default: return false;
  }});

  initNumber(tableVar, "#loops", UINT16_MAX, 0, 999, true, [this](EventArguments) { switch (eventType) {
    case onSetValue:
      for (size_t rowNr = 0; rowNr < loopFunctions.size(); rowNr++)
        variable.setValue(loopFunctions[rowNr].counter, rowNr);
      return true;
    case onLoop1s:
      variable.triggerEvent(onSetValue); //set the value (WIP)
      for (VarLoop &varLoop : loopFunctions)
        varLoop.counter = 0;
      return true;
    default: return false;
  }});
}

void SysModUI::loop20ms() { //never more then 50 times a second!

  for (VarLoop &varLoop : loopFunctions) {
    if (millis() - varLoop.lastMillis >= varLoop.variable.var["interval"].as<int>()) {
      varLoop.lastMillis = millis();

      varLoop.loopFun(varLoop.variable, 1, onLoop); //rowNr..

      varLoop.counter++;
      // ppf("%s %u %u %d %d\n", varLoop->Variable(var).id(), varLoop->lastMillis, millis(), varLoop->interval, varLoop->counter);
    }
  }
}

void SysModUI::processJson(JsonVariant json) {
  if (json.is<JsonObject>()) //should be
  {
     //varEvent adds object elements to json which would be processed in the for loop. So we freeze the original pairs in a vector and loop on this
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
        var[JsonString(key)] = pair.value(); //this is needed as key can become a dangling pointer
        // json.remove(key); //key should stay as all clients use this to perform the changeHTML action
      }
      else if (pair.key() == "onAdd" || pair.key() == "onDelete") {
        if (value.is<JsonObject>()) {
          JsonObject command = value;
          JsonObject var = mdl->findVar(command["pid"], command["id"]);
          uint8_t rowNr = command["rowNr"].isNull()?UINT8_MAX:command["rowNr"];
          ppf("processJson %s - %s[%d]\n", key, Variable(var).id(), rowNr);

          Variable(var).triggerEvent(pair.key() == "onAdd"?onAdd:onDelete, rowNr);

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
            if (id != nullptr ) {
              strlcpy(pid, id, sizeof(pid)); //copy the id part
              id = strtok(nullptr, "."); //the rest after .
            }

            JsonObject var = mdl->findVar(pid, id); //value is the id
            if (!var.isNull()) {
              Variable(var).triggerEvent(onUI);
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
        if (rowNrC != nullptr ) {
          strlcpy(pidid, rowNrC, sizeof(pidid)); //copy the pidid part
          rowNrC = strtok(nullptr, "#"); //the rest after #
        }
        uint8_t rowNr = rowNrC?strtol(rowNrC, nullptr, 10):UINT8_MAX;

        char pid[64];
        strlcpy(pid, pidid, sizeof(pid));
        char * id = strtok(pid, ".");
        if (id != nullptr ) {
          strlcpy(pid, id, sizeof(pid)); //copy the id part
          id = strtok(nullptr, "."); //the rest after .
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
              Variable(var).triggerEvent(onChange, rowNr);
              if (rowNr != UINT8_MAX) web->getResponseObject()[pidid]["rowNr"] = rowNr;
            }
            else {
              Variable(var).setValueJV(newValue, rowNr);
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