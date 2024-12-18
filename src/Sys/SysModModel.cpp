/*
   @title     StarBase
   @file      SysModModel.cpp
   @date      20241105
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModModel.h"
#include "SysModule.h"
#include "SysModFiles.h"
#include "SysStarJson.h"
#include "SysModUI.h"
#include "SysModInstances.h"

  Variable::Variable() {
    var = JsonObject(); //undefined variable
  }

  Variable::Variable(JsonObject var) {
    this->var = var;
  }

  Variable::Variable(const char *pid, const char *id) {
    var = mdl->findVar(pid, id);
  }

  String Variable::valueString(uint8_t rowNr) {
    if (rowNr == UINT8_MAX)
      return value().as<String>();
    else
      return value()[rowNr].as<String>();
  }

  JsonArray Variable::children() {
    return var["n"];
  }

  void Variable::removeValuesForRow(uint8_t rowNr) {
    for (JsonObject childVar: children()) {
      Variable childVariable = Variable(childVar);
      JsonArray valArray = childVariable.valArray();
      if (!valArray.isNull()) {
        valArray.remove(rowNr);
        //recursive
        childVariable.removeValuesForRow(rowNr);
      }
    }
  }

  void Variable::rows(std::function<void(Variable, uint8_t)> fun) {
    //tbd table check ... 
    //tbd move to table subclass??
    // get the first child
    JsonObject firstChild = children()[0];
    //loop through its rows
    uint8_t rowNr = 0;
    for (JsonVariant value: Variable(firstChild).valArray()) {
      if (fun) fun(*this, rowNr);
      // find the other columns
      //loop over children to get the table columns
      // ppf("row %d:", rowNr);
      // for (JsonObject child: children()) {
      //   Variable childVariable = Variable(child);
      //   ppf(" %s: %s", childVariable.id(), childVariable.valueString(rowNr));
      //   //process each ...
      // }
      // ppf("\n");
      rowNr++;
    }
  }

  void Variable::preDetails() {
    print->printJson("preDetails pre", var);
    if (var["oldValue"].isNull()) //no previous effect: during boot
      for (JsonObject varChild: children()) { //for all controls
        varChild.remove("o");
      }
    else {
      var["n"].to<JsonArray>(); //delete old values
    }

    ppf("preDetails %s.%s post ", pid(), id());
    print->printVar(var);
    ppf("\n");
  }

  void Variable::postDetails(uint8_t rowNr) {

    ppf("postDetails %s.%s pre ", pid(), id());
    print->printVar(var);
    ppf("\n");

    //check if post init added: parent is already >=0
    if (order() >= 0) {
      for (JsonArray::iterator childVarIt=children().begin(); childVarIt!=children().end(); ++childVarIt) { //use iterator to make .remove work!!!
        JsonObject childVar = *childVarIt;
        Variable childVariable = Variable(childVar);
        JsonArray valArray = childVariable.valArray();
        if (!valArray.isNull())
        {
          if (rowNr != UINT8_MAX) {
            if (childVar["o"].isNull()) { //if not updated, or order == 0 (which should not happen so better delete also)
              valArray[rowNr] = (char*)0; // set element in valArray to 0 (is content deleted from memory?)

              ppf("varPostDetails %s.%s[%d] <- null\n", id(), childVariable.id(), rowNr);
            }

            //if all values null, remove value
            bool allNull = true;
            for (JsonVariant element: valArray) {
              if (!element.isNull())
                allNull = false;
            }
            if (allNull) {
              ppf("remove allnulls %s\n", childVariable.id());
              children().remove(childVarIt);
            }
          }
          else
            print->printJson("dev array but not rowNr", var);
        }
        else {
          if (childVar["o"].isNull()) { //if not updated
            ppf("varPostDetails %s.%s <- null\n", id(), childVariable.id());
            print->printJson("remove", childVar);
            children().remove(childVarIt);
          }
        }

      }
    } //if new added
    ppf("postDetails %s.%s post ", pid(), id());
    print->printJson("  Var", var);
    ppf("\n");

    //post update details
    if (rowNr != UINT8_MAX)
      web->getResponseObject()["details"]["rowNr"] = rowNr;
    web->getResponseObject()["details"]["var"] = var;
  }

  bool Variable::triggerEvent(uint8_t eventType, uint8_t rowNr, bool init) {

    if (eventType == onChange) {
      if (!init) {
        if (!var["dash"].isNull())
          instances->changedVarsQueue.push_back(var); //tbd: check value arrays / rowNr is working
      }

      //if var is bound by pointer, set the pointer value before calling onChange
      if (!var["p"].isNull()) {
        JsonVariant value;
        if (rowNr == UINT8_MAX) {
          value = this->value(); 
        } else {
          value = this->value(rowNr);
        }

        //pointer is an array if set by setValueRowNr, used for controls as each control has a seperate variable
        bool isPointerArray = var["p"].is<JsonArray>();
        int pointer;
        if (isPointerArray)
          pointer = var["p"][rowNr];
        else
          pointer = var["p"];

        if (pointer != 0) {

          if (this->value().is<JsonArray>() && !isPointerArray) { //vector if val array but not if control (each var in array stored in seperate variable)
            if (rowNr != UINT8_MAX) {
              //pointer checks
              if (var["type"] == "select" || var["type"] == "range" || var["type"] == "pin") {
                std::vector<uint8_t> *valuePointer = (std::vector<uint8_t> *)pointer;
                while (rowNr >= (*valuePointer).size()) (*valuePointer).push_back(UINT8_MAX); //create vector space if needed...
                ppf("%s.%s[%d]:%s (%d - %d - %s)\n", pid(), id(), rowNr, valueString().c_str(), pointer, (*valuePointer).size(), var["p"].as<String>().c_str());
                (*valuePointer)[rowNr] = value;
              }
              else if (var["type"] == "number") {
                std::vector<uint16_t> *valuePointer = (std::vector<uint16_t> *)pointer;
                while (rowNr >= (*valuePointer).size()) (*valuePointer).push_back(UINT16_MAX); //create vector space if needed...
                (*valuePointer)[rowNr] = value;
              }
              else if (var["type"] == "checkbox") {
                std::vector<bool3State> *valuePointer = (std::vector<bool3State> *)pointer;
                while (rowNr >= (*valuePointer).size()) (*valuePointer).push_back(UINT8_MAX); //create vector space if needed...
                (*valuePointer)[rowNr] = value;
              }
              else if (var["type"] == "text" || var["type"] == "fileEdit") {
                std::vector<VectorString> *valuePointer = (std::vector<VectorString> *)pointer;
                while (rowNr >= (*valuePointer).size()) (*valuePointer).push_back(VectorString()); //create vector space if needed...
                strlcpy((*valuePointer)[rowNr].s, value.as<const char *>(), sizeof(VectorString().s));
              }
              else if (var["type"] == "coord3D") {
                std::vector<Coord3D> *valuePointer = (std::vector<Coord3D> *)pointer;
                while (rowNr >= (*valuePointer).size()) (*valuePointer).push_back({-1,-1,-1}); //create vector space if needed...
                (*valuePointer)[rowNr] = value;
              }
              else
                print->printJson("dev triggerChange type not supported yet (arrays)", var);

              // ppf("triggerChange set pointer to vector %s[%d]: v:%s p:%d\n", id(), rowNr, value.as<String>().c_str(), pointer);
            } else 
              print->printJson("dev value is array but no rowNr\n", var);
          } else { //no array
            //pointer checks
            if (var["type"] == "select" || var["type"] == "range" || var["type"] == "pin") {
              uint8_t *valuePointer = (uint8_t *)pointer;
              *valuePointer = value;
            }
            else if (var["type"] == "number") {
              uint16_t *valuePointer = (uint16_t *)pointer;
              *valuePointer = value;
            }
            else if (var["type"] == "checkbox") {
              bool3State *valuePointer = (bool3State *)pointer;
              *valuePointer = value;
            }
            else if (var["type"] == "coord3D") {
              Coord3D *valuePointer = (Coord3D *)pointer;
              *valuePointer = value;
            }
            else
              print->printJson("dev triggerChange type not supported yet", var);

            // ppf("triggerChange set pointer %s[%d]: v:%s p:%d\n", id(), rowNr, valueString().c_str(), pointer);
          }

          // else if (var["type"] == "text") {
          //   const char *valuePointer = (const char *)pointer;
          //   if (valuePointer != nullptr) {
          //     *valuePointer = value;
          //     ppf("pointer set16 %s: v:%d (p:%p) (r:%d v:%s p:%d)\n", id(), *valuePointer, valuePointer, rowNr, valueString().c_str(), pointer);
          //   }
          //   else
          //     ppf("dev pointer set16 %s: v:%d (p:%p) (r:%d v:%s p:%d)\n", id(), *valuePointer, valuePointer, rowNr, valueString().c_str(), pointer);
          // }
        }
        else
          // ppf("dev pointer of type %s is 0\n", var["type"].as<String>().c_str());
          print->printJson("dev pointer is 0", var);
      } //pointer

      //reset presets if not using presets controls and if updated by UI, except if updated by ui via presets
      if (var["id"] != "preset" && var["id"] != "assignPreset" && var["id"] != "clearPreset" && mdl->resetPresetThreshold > 1) {
        JsonObject moduleVar = mdl->findModule(pid(), id());
        JsonObject presetVar = mdl->findVar(moduleVar["id"], "preset");
        if (!presetVar.isNull()) {
          ppf("reset Preset %s.%s\n", pid(), id());
          Variable(presetVar).setValue(0, rowNr);
        }
      }
    }

    bool result = false;

    //call varEvent if exists
    if (!var["fun"].isNull()) { //isNull needed here!
      size_t funNr = var["fun"];
      if (funNr < mdl->varEvents.size()) {
        // ppf("voor v1 call %s.%s[%d] %d %d %d\n", pid(), id(), rowNr, funNr, eventType, mdl->varEvents.size());
        result = mdl->varEvents[funNr](*this, rowNr, eventType);

        //all ppf here:
        if (result && !readOnly()) { //send rowNr = 0 if no rowNr
          //only print vars with a value and not onSetValue as that changes a lot due to instances clients etc (tbd)
          //don't print if onSetValue or oldValue is null
          if (eventType != onSetValue && (!var["oldValue"].isNull() || ((rowNr != UINT8_MAX) && !var["oldValue"][rowNr].isNull()))) {
            ppf("%sEvent %s.%s", eventType==onSetValue?"val":eventType==onUI?"ui":eventType==onChange?"ch":eventType==onAdd?"add":eventType==onDelete?"del":"other", pid(), id());
            if (rowNr != UINT8_MAX) {
              ppf("[%d] (", rowNr);
              if (eventType == onChange) ppf("%s ->", var["oldValue"][rowNr].as<String>().c_str());
              ppf("%s)\n", valueString().c_str());
            }
            else {
              ppf(" (");
              if (eventType == onChange) ppf("%s ->", var["oldValue"].as<String>().c_str());
              ppf("%s)\n", valueString().c_str());
            }
          }
        } //varEvent exists
      }
      else if (funNr == UINT8_MAX)
        result = publish(eventType, rowNr);
      else
        ppf("dev triggerEvent function nr %s.%s outside bounds %d >= %d\n", pid(), id(), funNr, mdl->varEvents.size());
    } //varEvent exists


    //delete pointers after calling var.onDelete as var.onDelete might need the values
    if (eventType == onAdd || eventType == onDelete) {

      print->printJson("triggerEvent add/del", var);
      //if delete, delete also from vector ...
      //find the columns of the table
      if (eventType == onDelete) {
        for (JsonObject childVar: children()) {
          int pointer;
          if (childVar["p"].is<JsonArray>())
            pointer = childVar["p"][rowNr];
          else
            pointer = childVar["p"];

          ppf("  delete vector %s[%d] %d\n", Variable(childVar).id(), rowNr, pointer);

          if (pointer != 0) {
            //pointer checks
            // check rowNr as it can be 255 
            if (childVar["type"] == "select" || childVar["type"] == "range" || childVar["type"] == "pin") {
              std::vector<uint8_t> *valuePointer = (std::vector<uint8_t> *)pointer;
              if (rowNr < (*valuePointer).size())
                (*valuePointer).erase((*valuePointer).begin() + rowNr);
            } else if (childVar["type"] == "number") {
              std::vector<uint16_t> *valuePointer = (std::vector<uint16_t> *)pointer;
              if (rowNr < (*valuePointer).size())
                (*valuePointer).erase((*valuePointer).begin() + rowNr);
            } else if (childVar["type"] == "checkbox") {
              std::vector<bool3State> *valuePointer = (std::vector<bool3State> *)pointer;
              if (rowNr < (*valuePointer).size())
                (*valuePointer).erase((*valuePointer).begin() + rowNr);
            } else if (childVar["type"] == "text" || childVar["type"] == "fileEdit") {
              std::vector<VectorString> *valuePointer = (std::vector<VectorString> *)pointer;
              if (rowNr < (*valuePointer).size())
                (*valuePointer).erase((*valuePointer).begin() + rowNr);
            } else if (childVar["type"] == "coord3D") {
              std::vector<Coord3D> *valuePointer = (std::vector<Coord3D> *)pointer;
              if (rowNr < (*valuePointer).size())
                (*valuePointer).erase((*valuePointer).begin() + rowNr);
            }
            else
              print->printJson("dev triggerEvent onDelete type not supported yet", childVar);
          }
        }
      } //onDelete
      web->getResponseObject()[eventType==onAdd?"onAdd":"onDelete"]["rowNr"] = rowNr;
      print->printJson("triggerEvent add/del response", web->getResponseObject());
    } //onAdd onDelete

    //for ro variables, call onSetValue to add also the value in responseDoc (as it is not stored in the model)
    if (eventType == onUI && readOnly()) {
      triggerEvent(onSetValue, rowNr);
    }

    return result; //varEvent exists
  }

  void Variable::setLabel(const char * text) {
    web->addResponse(var, "label", text);
  }
  void Variable::setComment(const char * text) {
    web->addResponse(var, "comment", text);
  }
  JsonArray Variable::setOptions() {
    JsonObject responseObject = web->getResponseObject();
    char pidid[64];
    print->fFormat(pidid, sizeof(pidid), "%s.%s", var["pid"].as<const char *>(), var["id"].as<const char *>());
    return responseObject[pidid]["options"].to<JsonArray>();
  }
  //return the options from onUI (don't forget to clear responseObject)
  JsonArray Variable::getOptions() {
    triggerEvent(onUI); //rebuild options
    char pidid[64];
    print->fFormat(pidid, sizeof(pidid), "%s.%s", var["pid"].as<const char *>(), var["id"].as<const char *>());
    return web->getResponseObject()[pidid]["options"];
  }
  void Variable::clearOptions() {
    char pidid[64];
    print->fFormat(pidid, sizeof(pidid), "%s.%s", var["pid"].as<const char *>(), var["id"].as<const char *>());
    web->getResponseObject()[pidid].remove("options");
  }

  void Variable::getOption(char *option, uint8_t index) {
    char pidid[64];
    print->fFormat(pidid, sizeof(pidid), "%s.%s", var["pid"].as<const char *>(), var["id"].as<const char *>());
    bool optionsExisted = !web->getResponseObject()[pidid]["options"].isNull();
    JsonArray options = getOptions();
    strlcpy(option, options[index], 64);
    if (!optionsExisted)
      clearOptions(); //if created here then also remove 
  }

  //find options text in a hierarchy of options
  void Variable::findOptionsText(uint8_t value, char * groupName, char * optionName) {
    uint8_t startValue = 0;
    char pidid[64];
    print->fFormat(pidid, sizeof(pidid), "%s.%s", var["pid"].as<const char *>(), var["id"].as<const char *>());
    bool optionsExisted = !web->getResponseObject()[pidid]["options"].isNull();
    JsonString groupNameJS;
    JsonString optionNameJS;
    JsonArray options = getOptions();
    if (!findOptionsTextRec(options, &startValue, value, &groupNameJS, &optionNameJS))
      ppf("findOptions select option not found %d %s %s\n", value, groupNameJS.isNull()?"X":groupNameJS.c_str(), optionNameJS.isNull()?"X":optionNameJS.c_str());
    strlcpy(groupName, groupNameJS.c_str(), 32); //groupName is char[32]
    strlcpy(optionName, optionNameJS.c_str(), 32); //optionName is char[32]
    if (!optionsExisted)
      clearOptions(); //if created here then also remove 
  }

  // (groupName and optionName as pointers? String is already a pointer?)
  bool Variable::findOptionsTextRec(JsonVariant options, uint8_t * startValue, uint8_t value, JsonString *groupName, JsonString *optionName, JsonString parentGroup) {
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

   void Variable::setValueJV(JsonVariant value, uint8_t rowNr) {
    if (value.is<JsonArray>()) {
      uint8_t rowNr = 0;
      // ppf("   %s is Array\n", value.as<String>().c_str);
      for (JsonVariant el: value.as<JsonArray>()) {
        setValueJV(el, rowNr++);
      }
    }
    else if (value.is<const char *>())
      setValue(value, rowNr);
    else if (value.is<Coord3D>()) //otherwise it will be treated as JsonObject and toJson / fromJson will not be triggered!!!
      setValue(value.as<Coord3D>(), rowNr);
    else
      setValue(value, rowNr);
  }

  //Set value with argument list
  void Variable::setValueF(const char * format, ...) {
    va_list args;
    va_start(args, format);

    char value[128];
    vsnprintf(value, sizeof(value)-1, format, args);

    va_end(args);

    setValue(JsonString(value));
  }

  JsonVariant Variable::getValue(uint8_t rowNr) {
    if (var["value"].is<JsonArray>()) {
      JsonArray valueArray = valArray();
      if (rowNr == UINT8_MAX) rowNr = mdl->getValueRowNr;
      if (rowNr != UINT8_MAX && rowNr < valueArray.size())
        return valueArray[rowNr];
      else if (valueArray.size())
        return valueArray[0]; //return the first element
      else {
        ppf("dev getValue no array or rownr wrong %s.%s %s %d\n", pid(), id(), valueString().c_str(), rowNr);
        return JsonVariant(); // return null
      }
    }
    else
      return var["value"];
  }

  bool Variable::initValue(int min, int max, int pointer) {

    if (pointer != 0) {
      if (mdl->setValueRowNr == UINT8_MAX)
        var["p"] = pointer; //store pointer!
      else
        var["p"][mdl->setValueRowNr] = pointer; //store pointer in array!
      // ppf("initValue pointer stored %s: %s\n", id(), (void *)(var["p"].as<String>().c_str()));
    }

    if (min) var["min"] = min;
    if (max && max != UINT16_MAX) var["max"] = max;

    //value needs update if varVal not set yet or varVal is array and setValueRowNr is not in it
    bool doSetValue = false;
    if (var["type"] != "button") { //button never gets a value
      if (var["value"].isNull()) {
        doSetValue = true;
        // print->printJson("initValue varEvent value is null", var);
      } else if (var["value"].is<JsonArray>()) {
        JsonArray valueArray = valArray();
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
      if (!var["fun"].isNull()) { // && var["fun"] != UINT8_MAX
        onSetValueExists = triggerEvent(onSetValue, mdl->setValueRowNr);
      }
      if (!onSetValueExists) { //setValue provided (if not null)
        return true;
      }
    }
    else { //do onChange on existing value
      //no call of onChange for buttons otherwise all buttons will be fired which is highly undesirable
      if (var["type"] != "button") { // && !var["fun"].isNull(): also if no varEvent to update pointers   !isPointer because 0 is also a value then && (!isPointer || value)
        bool onChangeExists = false;
        if (var["value"].is<JsonArray>()) {
          //refill the vector
          for (uint8_t rowNr = 0; rowNr < valArray().size(); rowNr++) {
            onChangeExists |= triggerEvent(onChange, rowNr, true); //init, also set var["p"]
          }
        }
        else {
          onChangeExists = triggerEvent(onChange, mdl->setValueRowNr, true); //init, also set var["p"] 
        }

        if (onChangeExists)
          ppf("initValue onChange %s.%s <- %s\n", pid(), id(), valueString().c_str());
      }
    }
    return false;
  }

SysModModel::SysModModel() :SysModule("Model") {
  model = new JsonDocument(&allocator);
  presets = new JsonDocument(&allocator);

  JsonArray root = model->to<JsonArray>(); //create

  ppf("Reading model from /model.json... (deserializeConfigFromFS)\n");
  if (files->readObjectFromFile("/model.json", model)) {//not part of success...
    // print->printJson("Read model", *model);
    // web->sendDataWs(*model);
  } else {
    root = model->to<JsonArray>(); //re create the model as it is corrupted by readFromFile
  }

  files->readObjectFromFile("/presets.json", presets); //do not create if not exists

}

void SysModModel::setup() {
  SysModule::setup();

  const Variable parentVar = ui->initSysMod(Variable(), name, 4303);
  parentVar.var["s"] = true; //setup

  ui->initButton(parentVar, "saveModel", false, [this](EventArguments) { switch (eventType) {
    case onUI:
      variable.setComment("Write to model.json");
      return true;
    case onChange:
      doWriteModel = true;
      return true;
    default: return false;
  }});

  #ifdef STARBASE_DEVMODE

  ui->initButton(parentVar, "deleteObsolete", false, [this](EventArguments) { switch (eventType) {
    case onUI:
      variable.setComment("Delete unused variables");
      return true;
    case onChange:
      walkThroughModel([this](JsonObject parentVar, JsonObject var) {

        Variable parentVariable = Variable(parentVar);
        Variable variable = Variable(var);
        JsonArray vars = parentVar.isNull()?model->as<JsonArray>() : parentVariable.children();

        //no cleanup of o in case of ro value removal
        if (var["o"].isNull()) { //!variable.var.isNull() &&  || variable.order() <= 0
          ppf("deleteObsolete remove var %s.%s (no order)\n", variable.pid()?variable.pid():"-", variable.id());          
            // vars.remove(var); //remove the obsolete var (no o or )
          for (JsonArray::iterator it=vars.begin(); it!=vars.end(); ++it) if ((*it)["id"] == var["id"]) vars.remove(it); //use iterator to make .remove work!!!
        }
        return JsonObject(); //don't stop
      });
      return true;
    default: return false;
  }});

  Variable currentVar;
  currentVar = ui->initText(parentVar, "eventsVar", nullptr, 16, true);
  currentVar.subscribe(onLoop1s, [this](EventArguments) {
    variable.setValueF("%d x %d = %d", varEvents.size(), sizeof(VarEvent), varEvents.size() * sizeof(VarEvent));
  });
  currentVar = ui->initText(parentVar, "eventsPS", nullptr, 16, true);
  currentVar.subscribe(onLoop1s, [this](EventArguments) {
    variable.setValueF("%d x %d = %d (%d + %d + %d)", varEventsPS.size(), sizeof(VarEventPS), varEventsPS.size() * sizeof(VarEventPS), sizeof(Variable), sizeof(VarFunction), sizeof(uint8_t));
  });

  #endif //STARBASE_DEVMODE
}

void SysModModel::loop20ms() {

  if (doWriteModel) {
    ppf("Writing model to /model.json... (serializeConfig)\n");

    // files->writeObjectToFile("/model.json", model);

    walkThroughModel([](JsonObject parentVar, JsonObject var) {

      Variable variable = Variable(var);

      //remove ro values (ro vars cannot be deleted as SM uses these vars)
      // remove if var is ro or table is instance table (exception here, values don't need to be saved)
      if (parentVar["id"] == "instances" || variable.readOnly()) {// && !value().isNull())
        // ppf("remove ro value %s.%s\n", variable.pid(), variable.id());          
        var.remove("value");
      }

      return JsonObject(); //don't stop
    });

    StarJson starJson("/model.json", FILE_WRITE); //open fileName for deserialize
    //comment exclusions out in case of generating model.json for github
    starJson.addExclusion("fun");
    starJson.addExclusion("dash");
    starJson.addExclusion("o"); //order: this must be deleted as it will be used to check on reboot 
    starJson.addExclusion("p"); //pointer
    starJson.addExclusion("oldValue");
    starJson.writeJsonDocToFile(model);

    // print->printJson("Write model", *model); //this shows the model before exclusion

    if (!presets->isNull())
      files->writeObjectToFile("/presets.json", presets);


    doWriteModel = false;
  }
}

void SysModModel::loop1s() {
  walkThroughModel([](JsonObject parentVar, JsonObject var) {
    Variable(var).triggerEvent(onLoop1s);
    return JsonObject(); //don't stop
  });
}

Variable SysModModel::initVar(Variable parent, const char * id, const char * type, bool readOnly, const VarEvent &varEvent) {
  const char * parentId = parent.var["id"];
  if (!parentId) parentId = "m"; //m=module
  JsonObject var = findVar(parentId, id);
  Variable variable = Variable(var);

  //create new var
  if (var.isNull()) {
    // ppf("initVar new %s: %s.%s\n", type, parentId, id); //parentId not null otherwise crash
    if (parent.var.isNull()) {
      JsonArray vars = model->as<JsonArray>();
      var = vars.add<JsonObject>();
    } else {
      if (parent.var["n"].isNull()) parent.var["n"].to<JsonArray>(); //TO!!! if parent exist and no "n" array, create it
      var = parent.var["n"].add<JsonObject>();
      // serializeJson(model, Serial);Serial.println();
    }
    var["id"] = JsonString(id);
  }
  // else {
  //   ppf("initVar Var %s->%s already defined\n", modelParentId, id);
  // }

  if (!var.isNull()) {
    if (var["type"].isNull() || var["type"] != type) {
      var["type"] = JsonString(type);
      // print->printJson("initVar set type", var);
    }

    variable = Variable(var);

    var["pid"] = parentId;

    if (var["ro"].isNull() || variable.readOnly() != readOnly) variable.readOnly(readOnly);

    //set order
    if (variable.order() < 1000) //predefined! (modules) - positive as saved in model.json
      variable.order( varCounter++); //redefine order

    //if varEvent, add it to the list
    if (varEvent) {
      //if fun already in ucFunctions then reuse, otherwise add new fun in ucFunctions
      //lambda update: when replacing typedef void(*UCFun)(JsonObject); with typedef std::function<void(JsonObject)> UCFun; this gives error:
      //  mismatched types 'T*' and 'std::function<void(ArduinoJson::V6213PB2::JsonObject)>' { return *__it == _M_value; }
      //  it also looks like functions are not added more then once anyway
      // std::vector<UCFun>::iterator itr = find(ucFunctions.begin(), ucFunctions.end(), varEvent);
      // if (itr!=ucFunctions.end()) //found
      //   var["varEvent"] = distance(ucFunctions.begin(), itr); //assign found function
      // else { //not found
        varEvents.push_back(varEvent); //add new function
        var["fun"] = varEvents.size()-1;
      // }
      
      if (varEvent(variable, UINT8_MAX, onLoop)) { //test run if it supports loop
        //no need to check if already in...
        VarLoop loop;
        loop.loopFun = varEvent;
        loop.variable = variable;

        ui->loopFunctions.push_back(loop);
        var["loopFun"] = ui->loopFunctions.size()-1;
        // ppf("iObject loopFun %s %u %u %d %d\n", variable.id());
      }
    }
  }
  else
    ppf("initVar could not find or create var %s with %s\n", id, type);

  return variable;
}

void Variable::subscribe(uint8_t eventType, const VarFunction &varFunction) {
  ppf("subscribe %d %s.%s\n", eventType, pid(), id());
  mdl->varEventsPS.push_back({*this, eventType, varFunction}); //add new function
  var["fun"] = UINT8_MAX; //to trigger response from ui
}

bool Variable::publish(uint8_t eventType, uint8_t rowNr) {
  bool found = false;
  for (VarEventPS &varEventPS: mdl->varEventsPS) {
    if (eventType == varEventPS.eventType && strncmp(pid(), varEventPS.variable.pid(), 32) == 0 && strncmp(id(), varEventPS.variable.id(), 32) == 0) {
      if (strcmp(id(), "effect") == 0 && eventType!= onLoop1s)
        ppf("publish %s.%s[%d] %d=%d %s.%s\n", pid(), id(), rowNr, eventType, varEventPS.eventType , varEventPS.variable.pid(), varEventPS.variable.id());
      varEventPS.varFunction(*this, rowNr, eventType);
      found = true;
    }
  }
  return found;
}


JsonObject SysModModel::walkThroughModel(std::function<JsonObject(JsonObject, JsonObject)> fun, JsonObject parentVar) {
  for (JsonObject var : parentVar.isNull()?model->as<JsonArray>(): parentVar["n"]) {
    // ppf(" %s", var["id"].as<String>());
    JsonObject foundVar = fun(parentVar, var);
    if (!foundVar.isNull()) return foundVar;

    if (!var["n"].isNull()) {
      foundVar = walkThroughModel(fun, var);
      if (!foundVar.isNull()) return foundVar;
    }
  }
  return JsonObject(); //don't stop
}

JsonObject SysModModel::findVar(const char * pid, const char * id, JsonObject parentVar) {
  for (JsonObject var : parentVar.isNull()?model->as<JsonArray>():parentVar["n"]) {
    if (var["pid"] == pid && var["id"] == id) { //(!pid && var["pid"] == pid) && 
      // Serial.printf("findVar found %s.%s!!\n", pid, id);
      return var;
    }
    else if (!var["n"].isNull()) {
      JsonObject foundVar = findVar(pid, id, var);
      if (!foundVar.isNull()) {
        return foundVar;
      }
    }
  }
  // if (parent.isNull())
  //   Serial.printf("dev findVar not found %s.%s!!\n", pid?pid:"x", id?id:"y");
  return JsonObject();
}

JsonObject SysModModel::findModule(const char * pid, const char * id) {
  // if (model->isNull()) return JsonObject();

  for (JsonObject moduleVar : model->as<JsonArray>()) {
    bool pididFound = false;
    walkThroughModel([&pididFound, pid, id](JsonObject parentVar, JsonObject var) {
      pididFound = var["pid"] == pid && var["id"] == id;
      return pididFound?var:JsonObject(); //stop if found
    }, moduleVar);
    if (pididFound) return moduleVar;
  }

  return JsonObject();
}

void SysModModel::findVars(const char * property, bool value, FindFun fun, JsonObject parentVar) {
  // print ->print("findVar %s %s\n", id, parent.isNull()?"root":"n");

  for (JsonObject var : parentVar.isNull()?model->as<JsonArray>():parentVar["n"]) {
    if (var[property] == value)
      fun(Variable(var));
    if (!var["n"].isNull())
      findVars(property, value, fun, var);
  }
}