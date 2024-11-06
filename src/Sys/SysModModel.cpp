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

  String Variable::valueString(uint8_t rowNr) {
    if (rowNr == UINT8_MAX)
      return var["value"].as<String>();
    else
      return var["value"][rowNr].as<String>();
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
    for (JsonObject varChild: children()) { //for all controls
      if (Variable(varChild).order() >= 0) { //post init
        Variable(varChild).order( -Variable(varChild).order()); // set all negative
      }
    }
    ppf("preDetails post ");
    print->printVar(var);
    ppf("\n");
  }

  void Variable::postDetails(uint8_t rowNr) {

    ppf("varPostDetails pre ");
    print->printVar(var);
    ppf("\n");

    //check if post init added: parent is already >=0
    if (order() >= 0) {
      for (JsonArray::iterator childVarIt=children().begin(); childVarIt!=children().end(); ++childVarIt) { //use iterator to make .remove work!!!
      // for (JsonObject &childVarIt: children) { //use iterator to make .remove work!!!
        JsonObject childVar = *childVarIt;
        Variable childVariable = Variable(childVar);
        JsonArray valArray = childVariable.valArray();
        if (!valArray.isNull())
        {
          if (rowNr != UINT8_MAX) {
            if (childVariable.order() < 0) { //if not updated
              valArray[rowNr] = (char*)0; // set element in valArray to 0

              ppf("varPostDetails %s.%s[%d] <- null\n", id(), childVariable.id(), rowNr);
              // setValue(var, -99, rowNr); //set value -99
              childVariable.order(-childVariable.order());
              //if some values in array are not -99
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
            web->getResponseObject()["details"]["rowNr"] = rowNr;

          }
          else
            print->printJson("dev array but not rowNr", var);
        }
        else {
          if (childVariable.order() < 0) { //if not updated
            // childVar["value"] = (char*)0;
            ppf("varPostDetails %s.%s <- null\n", id(), childVariable.id());
              // setValue(var, -99, rowNr); //set value -99
            // childVariable.order(-childVariable.order());
            print->printJson("remove", childVar);
            children().remove(childVarIt);
          }
        }

      }
    } //if new added
    ppf("varPostDetails post ");
    print->printVar(var);
    ppf("\n");

    //post update details
    web->getResponseObject()["details"]["var"] = var;
  }

  bool Variable::triggerEvent(uint8_t funType, uint8_t rowNr, bool init) {

    if (funType == onChange) {
      if (!init) {
        if (!var["dash"].isNull())
          instances->changedVarsQueue.push_back(var); //tbd: check value arrays / rowNr is working
      }

      //if var is bound by pointer, set the pointer value before calling onChange
      if (!var["p"].isNull()) {
        JsonVariant value;
        if (rowNr == UINT8_MAX) {
          value = var["value"]; 
        } else {
          value = var["value"][rowNr];
        }

        //pointer is an array if set by setValueRowNr, used for controls as each control has a seperate variable
        bool isPointerArray = var["p"].is<JsonArray>();
        int pointer;
        if (isPointerArray)
          pointer = var["p"][rowNr];
        else
          pointer = var["p"];

        if (pointer != 0) {

          if (var["value"].is<JsonArray>() && !isPointerArray) { //vector if val array but not if control (each var in array stored in seperate variable)
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
          //     ppf("pointer set16 %s: v:%d (p:%p) (r:%d v:%s p:%d)\n", Variable(var).id(), *valuePointer, valuePointer, rowNr, var["value"].as<String>().c_str(), pointer);
          //   }
          //   else
          //     ppf("dev pointer set16 %s: v:%d (p:%p) (r:%d v:%s p:%d)\n", Variable(var).id(), *valuePointer, valuePointer, rowNr, var["value"].as<String>().c_str(), pointer);
          // }
        }
        else
          // ppf("dev pointer of type %s is 0\n", var["type"].as<String>().c_str());
          print->printJson("dev pointer is 0", var);
      } //pointer
    }

    bool result = false;

    //call varFun if exists
    if (!var["fun"].isNull()) {//isNull needed here!
      size_t funNr = var["fun"];
      if (funNr < mdl->varFunctions.size()) {
        result = mdl->varFunctions[funNr](var, rowNr, funType);
        if (result && !readOnly()) { //send rowNr = 0 if no rowNr
          //only print vars with a value and not onSetValue as that changes a lot due to instances clients etc (tbd)
          //don't print if onSetValue or oldValue is null
          if (funType != onSetValue && (!var["oldValue"].isNull() || ((rowNr != UINT8_MAX) && !var["oldValue"][rowNr].isNull()))) {
            ppf("%sFun %s.%s", funType==onSetValue?"val":funType==onUI?"ui":funType==onChange?"ch":funType==onAdd?"add":funType==onDelete?"del":"other", pid(), id());
            if (rowNr != UINT8_MAX) {
              ppf("[%d] (", rowNr);
              if (funType == onChange) ppf("%s ->", var["oldValue"][rowNr].as<String>().c_str());
              ppf("%s)\n", var["value"][rowNr].as<String>().c_str());
            }
            else {
              ppf(" (");
              if (funType == onChange) ppf("%s ->", var["oldValue"].as<String>().c_str());
              ppf("%s)\n", valueString().c_str());
            }
          }
        } //varFun exists
      }
      else    
        ppf("dev triggerEvent function nr %s.%s outside bounds %d >= %d\n", pid(), id(), funNr, mdl->varFunctions.size());
    } //varFun exists

    //delete pointers after calling var.onDelete as var.onDelete might need the values
    if (funType == onAdd || funType == onDelete) {

      print->printJson("triggerEvent add/del", var);
      //if delete, delete also from vector ...
      //find the columns of the table
      if (funType == onDelete) {
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
      web->getResponseObject()[funType==onAdd?"onAdd":"onDelete"]["rowNr"] = rowNr;
      print->printJson("triggerEvent add/del response", web->getResponseObject());
    } //onAdd onDelete

    //for ro variables, call onSetValue to add also the value in responseDoc (as it is not stored in the model)
    if (funType == onUI && readOnly()) {
      triggerEvent(onSetValue, rowNr);
    }

    return result; //varFun exists
  }

SysModModel::SysModModel() :SysModule("Model") {
  model = new JsonDocument(&allocator);

  JsonArray root = model->to<JsonArray>(); //create

  ppf("Reading model from /model.json... (deserializeConfigFromFS)\n");
  if (files->readObjectFromFile("/model.json", model)) {//not part of success...
    // print->printJson("Read model", *model);
    // web->sendDataWs(*model);
  } else {
    root = model->to<JsonArray>(); //re create the model as it is corrupted by readFromFile
  }
}

void SysModModel::setup() {
  SysModule::setup();

  parentVar = ui->initSysMod(parentVar, name, 4303);
  parentVar["s"] = true; //setup

  ui->initButton(parentVar, "saveModel", false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case onUI:
      ui->setComment(var, "Write to model.json");
      return true;
    case onChange:
      doWriteModel = true;
      return true;
    default: return false;
  }});

  #ifdef STARBASE_DEVMODE

  ui->initCheckBox(parentVar, "showObsolete", false, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case onUI:
      ui->setComment(var, "Show in UI (refresh)");
      return true;
    default: return false;
  }});

  ui->initButton(parentVar, "deleteObsolete", false, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case onUI:
      ui->setComment(var, "Delete obsolete variables 🚧");
      return true;
    // case onChange:
    //   model->to<JsonArray>(); //create
    //   if (files->readObjectFromFile("/model.json", model)) {//not part of success...
    //   }
    //   return true;
    default: return false;
  }});

  #endif //STARBASE_DEVMODE
}

void SysModModel::loop20ms() {

  if (!cleanUpModelDone) { //do after all setups
    cleanUpModelDone = true;
    cleanUpModel();
  }

  if (doWriteModel) {
    ppf("Writing model to /model.json... (serializeConfig)\n");

    // files->writeObjectToFile("/model.json", model);

    cleanUpModel(JsonObject(), false, true);//remove if var["o"] is negative (not cleanedUp) and remove ro values

    StarJson starJson("/model.json", "w"); //open fileName for deserialize
    //comment exclusions out in case of generating model.json for github
    starJson.addExclusion("fun");
    starJson.addExclusion("dash");
    starJson.addExclusion("o"); //order: this must be deleted as it will be used to check on reboot 
    starJson.addExclusion("p"); //pointer
    starJson.addExclusion("oldValue");
    starJson.writeJsonDocToFile(model);

    // print->printJson("Write model", *model); //this shows the model before exclusion

    doWriteModel = false;
  }
}

void SysModModel::loop1s() {
  mdl->walkThroughModel([](JsonObject var) {
    Variable(var).triggerEvent(onLoop1s);
    return false; //don't stop
  });
}

void SysModModel::cleanUpModel(JsonObject parent, bool oPos, bool ro) {

  JsonArray vars;
  if (parent.isNull()) //no parent
    vars = model->as<JsonArray>();
  else
    vars = Variable(parent).children();

  bool showObsolete = mdl->getValue("Model", "showObsolete");
  for (JsonArray::iterator varV=vars.begin(); varV!=vars.end(); ++varV) {
  // for (JsonVariant varV : vars) {
    if (varV->is<JsonObject>()) {
      JsonObject var = *varV;
      Variable variable = Variable(var);

      //no cleanup of o in case of ro value removal
      if (!ro) {
        if (oPos) {
          if (var["o"].isNull() || variable.order() >= 0) { //not set negative in initVar
            ppf("obsolete found %s removed: %d\n", variable.id(), showObsolete);
            if (!showObsolete)
              vars.remove(varV); //remove the obsolete var (no o or )
          }
          else {
            variable.order( -variable.order()); //make it possitive
          }
        } else { //!oPos
          if (var["o"].isNull() || variable.order() < 0) { 
            ppf("cleanUpModel remove var %s (""o""<0)\n", variable.id());          
            vars.remove(varV); //remove the obsolete var (no o or o is negative - not cleanedUp)
          }
        }
      }

      //remove ro values (ro vars cannot be deleted as SM uses these vars)
      // remove if var is ro or table is instance table (exception here, values don't need to be saved)
      if (ro && (parent["id"] == "instances" || variable.readOnly())) {// && !var["value"].isNull())
        // ppf("remove ro value %s\n", variable.id());          
        var.remove("value");
      }

      //recursive call
      if (!variable.children().isNull())
        cleanUpModel(var, oPos, ro);
    } 
  }
}

bool SysModModel::walkThroughModel(std::function<bool(JsonObject)> fun, JsonObject parent) {
  JsonArray root;
  if (parent.isNull())
    root = model->as<JsonArray>();
  else
    root = parent["n"];

  for (JsonObject var : root) {
    // ppf(" %s", var["id"].as<String>());
    if (fun(var)) return true;

    if (!var["n"].isNull()) {
      if (walkThroughModel(fun, var)) return true;
    }
  }
  return false; //don't stop
}

JsonObject SysModModel::findVar(const char * pid, const char * id, JsonObject parent) {
  JsonArray root;
  if (parent.isNull())
    root = model->as<JsonArray>();
  else
    root = parent["n"];

  for (JsonObject var : root) {
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

void SysModModel::findVars(const char * property, bool value, FindFun fun, JsonArray parent) {
  JsonArray root;
  // print ->print("findVar %s %s\n", id, parent.isNull()?"root":"n");
  if (parent.isNull())
    root = model->as<JsonArray>();
  else
    root = parent;

  for (JsonObject var : root) {
    if (var[property] == value)
      fun(var);
    if (!var["n"].isNull())
      findVars(property, value, fun, var["n"]);
  }
}