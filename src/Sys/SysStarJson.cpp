/*
   @title     StarBase
   @file      SysStarJson.h
   @date      20241219
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysStarJson.h"

#include "SysModFiles.h"
#include "SysModPrint.h"

//Lazy Json Read Deserialize Write Serialize (write / serialize not implemented yet)
//ArduinoJson won't work on very large fixture.json, this does
//only support what is currently needed: read / deserialize uint8/16/char var elements (arrays not yet)
  StarJson::StarJson(const char * path, const char * mode) {
    // ppf("StarJson constructing %s %s\n", path, mode);
    f = files->open(path, mode);
    if (!f)
      ppf("StarJson open %s for %s failed", path, mode);
  }

  StarJson::~StarJson() {
    // ppf("StarJson destructing\n");
    f.close();
  }

  void StarJson::addExclusion(const char * key) {
    charList.push_back((char *)key);
  }

  //serializeJson
  void StarJson::writeJsonDocToFile(JsonDocument* dest) {
    writeJsonVariantToFile(dest->as<JsonVariant>());
    f.close();
    files->filesChanged = true;
  }

  void StarJson::lookFor(const char * id, uint8_t * value) {
    uint8List.push_back(value);
    addToVars(id, "uint8", uint8List.size()-1);
  }

  // void StarJson::lookFor(const char * id, uint16_t * value) {
  //   uint16List.push_back(value);
  //   addToVars(id, "uint16", uint16List.size()-1);
  // }

  // void StarJson::lookFor(const char * id, int * value) {
  //   intList.push_back(value);
  //   addToVars(id, "int", intList.size()-1);
  // }

  void StarJson::lookFor(const char * id, char * value) {
    charList.push_back(value);
    addToVars(id, "char", charList.size()-1);
  }

  //look for array of integers
  void StarJson::lookFor(const char * id, const std::function<void(std::vector<uint16_t>)>& fun) {
    funList.push_back(fun);
    addToVars(id, "fun", funList.size()-1);
  }

  //reads from file until all vars have been found (then stops reading)
  //returns false if not all vars to look for are found
  bool StarJson::deserialize(const bool lazy) {
    f.read(&character, sizeof(byte));
    while (f.available() && (!foundAll || !lazy))
      next();
    if (foundAll)
      ppf("StarJson found all what it was looking for %d >= %d\n", foundCounter, varDetails.size());
    else
      ppf("StarJson Not all vars looked for where found %d < %d\n", foundCounter, varDetails.size());
    f.close();
    return foundAll;
  }

  //called by lookedFor, store the var details in varDetails
  void StarJson::addToVars(const char * id, const char * type, const size_t index) {
    VarDetails vd;
    vd.id = id;
    vd.type = type;
    vd.index = index;
    varDetails.push_back(vd);
  }

  void StarJson::next() {
    if (character=='{') { //object begin
      // ppf("Object %c\n", character);
      varStack.push_back(lastVarId); //copy!!
      // ppf("Object push %s %d\n", lastVarId, varStack.size());
      strlcpy(lastVarId, "", sizeof(lastVarId));
      f.read(&character, sizeof(byte));
    }
    else if (character=='}') { //object end
      strlcpy(lastVarId, varStack[varStack.size()-1].c_str(), sizeof(lastVarId));
      // ppf("Object pop %s %d\n", lastVarId, varStack.size());
      check(lastVarId);
      varStack.pop_back();
      f.read(&character, sizeof(byte));
    }
    else if (character=='[') { //array begin
      // ppf("Array %c\n", character);
      varStack.push_back(lastVarId); //copy!!
      // ppf("Array push %s %d\n", lastVarId, varStack.size());
      strlcpy(lastVarId, "", sizeof(lastVarId));
      f.read(&character, sizeof(byte));

      //now we want to collect the array elements
      collectNumbers = true;
      uint16CollectList.clear(); //to be sure not to have old numbers (e.g. pin)
    }
    else if (character==']') { //array end
      //assign back the popped var id from [
      strlcpy(lastVarId, varStack[varStack.size()-1].c_str(), sizeof(lastVarId));
      // ppf("Array pop %s %d %d\n", lastVarId, varStack.size(), uint16CollectList.size());
      check(lastVarId);

      //check the parent array, if exists
      if (varStack.size() - 2 >= 0) {
        // ppf("  Parent check %s\n", varStack[varStack.size()-2].c_str());
        strlcpy(beforeLastVarId, varStack[varStack.size()-2].c_str(), sizeof(beforeLastVarId));
        check(beforeLastVarId);
      }
      varStack.pop_back(); //remove var id of this array
      collectNumbers = false;
      uint16CollectList.clear();
      f.read(&character, sizeof(byte));
    }
    else if (character=='"') { //parse String
      char value[128] = "";
      f.readBytesUntil('"', value, sizeof(value)-1);
    
      //if no lastVar then var found
      if (strncmp(lastVarId, "", sizeof(lastVarId)) == 0) {
        // ppf("Element [%s]\n", value);
        strlcpy(lastVarId, value, sizeof(lastVarId));
      }
      else { // if lastvar then string value found
        // ppf("String var %s: [%s]\n", lastVarId, value);
        check(lastVarId, value);
        strlcpy(lastVarId, "", sizeof(lastVarId));
      }

      f.read(&character, sizeof(byte));
    }
    else if (isDigit(character)) { //parse number
      char value[100] = "";

      size_t len = 0;
      //readuntil not number
      while (isDigit(character)) {
        // ppf("%c", character);
        value[len++] = character;
        f.read(&character, sizeof(byte));
      }
      value[len++] = '\0';

      //number value found
      // ppf("Number var %s: [%s]\n", lastVarId, value);
      if (collectNumbers)
        uint16CollectList.push_back(strtol(value, nullptr, 10));

      check(lastVarId, value);
  
      strlcpy(lastVarId, "", sizeof(lastVarId));
    }
    else if (character==':') {
      // ppf("semicolon %c\n", character);
      f.read(&character, sizeof(byte));
    }
    else if (character==',') {
      // ppf("sep %c\n", character);
      f.read(&character, sizeof(byte));
    }
    else if (character==']') {
      // ppf("close %c\n", character);
      f.read(&character, sizeof(byte));
    }
    else if (character=='}') {
      // ppf("close %c\n", character);
      f.read(&character, sizeof(byte));
    }
    else if (character=='\n') { //skip new lines
      // ppf("skip newline \n");
      f.read(&character, sizeof(byte));
    }
    else {
      // ppf("%c", character);
      f.read(&character, sizeof(byte));
    }
  } //next

  void StarJson::check(char * varId, char * value) {
    //check if var is in lookFor list
    for (std::vector<VarDetails>::iterator vd=varDetails.begin(); vd!=varDetails.end(); ++vd) {
      // ppf("check %s %s %s\n", vd->id, varId, value);
      if (strncmp(vd->id, varId, 32)==0) {
        // ppf("StarJson found %s:%s %d %s %d %d\n", varId, vd->type, vd->index, value?value:"", uint16CollectList.size(), funList.size());
        if (strncmp(vd->type, "uint8", 7) ==0 && value) *uint8List[vd->index] = strtol(value, nullptr, 10);
        // if (strncmp(vd->type, "uint16", 7) ==0 && value) *uint16List[vd->index] = strtol(value, nullptr, 10);
        // if (strncmp(vd->type, "int", 7) ==0 && value) *intList[vd->index] = strtol(value, nullptr, 10);
        if (strncmp(vd->type, "char", 5) ==0 && value) strlcpy(charList[vd->index], value, 32); //assuming size 32 here
        if (strncmp(vd->type, "fun", 4) ==0) funList[vd->index](uint16CollectList); //call for every found item (no value check)
        foundCounter++;
      }
    }

    foundAll = foundCounter >= varDetails.size();
  }

  //writeJsonVariantToFile calls itself recursively until whole json document has been parsed
  void StarJson::writeJsonVariantToFile(JsonVariant variant) {
    if (variant.is<JsonObject>()) {
      f.printf("{");
      char sep[2] = "";
      for (JsonPair pair: variant.as<JsonObject>()) {
        bool found = false;
        for (char *el:charList) {
          if (strncmp(el, pair.key().c_str(), 32)==0) {
            found = true;
            break;
          }
        }
        // std::vector<char *>::iterator itr = find(charList.begin(), charList.end(), pair.key().c_str());
        if (!found) { //not found
          f.printf("%s\"%s\":", sep, pair.key().c_str());
          strlcpy(sep, ",", sizeof(sep));
          writeJsonVariantToFile(pair.value());
        }
      }
      f.printf("}");
    }
    else if (variant.is<JsonArray>()) {
      f.printf("[");
      char sep[2] = "";
      for (JsonVariant variant2: variant.as<JsonArray>()) {
        f.print(sep);
        strlcpy(sep, ",", sizeof(sep));
        writeJsonVariantToFile(variant2);
      }      
      f.printf("]");
    }
    else if (variant.is<const char *>()) {
      f.printf("\"%s\"", variant.as<const char *>());      
    }
    else if (variant.is<int>()) {
      f.printf("%d", variant.as<int>());      
    }
    else if (variant.is<bool>()) {
      f.printf("%s", variant.as<bool>()?"true":"false");      
    }
    else if (variant.isNull()) {
      f.print("null");      
    }
    else
      ppf("dev StarJson write %s not supported\n", variant.as<String>().c_str());
  }
