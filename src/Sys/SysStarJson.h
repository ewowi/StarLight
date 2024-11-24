/*
   @title     StarBase
   @file      SysStarJson.h
   @date      20241105
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include <Arduino.h>
#include <LittleFS.h>
#include "ArduinoJson.h"

#include <vector>

//Lazy Json Read Deserialize Write Serialize (write / serialize not implemented yet)
//ArduinoJson won't work on very large fixture.json, this does
//only support what is currently needed: read / deserialize uint8/16/char var elements (arrays not yet)
class StarJson {

  public:

  explicit StarJson(const char * path, const char * mode = "r");

  ~StarJson();

  void addExclusion(const char * key);

  //serializeJson
  void writeJsonDocToFile(JsonDocument* dest);

  //look for uint8 var
  // void lookFor(const char * id, uint8_t * value) {
  //   // const char *p = (const char*)&value; //pointer trick
  //   uint8List.push_back(value);
  //   addToVars(id, "uint8", uint8List.size()-1);
  // }

  void lookFor(const char * id, uint8_t * value);
  // void lookFor(const char * id, uint16_t * value);
  // void lookFor(const char * id, int * value);
  void lookFor(const char * id, char * value);
  void lookFor(const char * id, const std::function<void(std::vector<uint16_t>)>& fun);

  //reads from file until all vars have been found (then stops reading)
  //returns false if not all vars to look for are found
  bool deserialize(bool lazy = false);

private:
  struct VarDetails {
    const char * id;
    const char * type;
    size_t index;
  };

  File f;
  byte character; //the last character parsed
  std::vector<VarDetails> varDetails; //details of vars looking for
  std::vector<uint8_t *> uint8List; //pointer of uint8 to assign found values to (index of list stored in varDetails)
  // std::vector<uint16_t *> uint16List; //same for uint16
  // std::vector<int *> intList; //same for int
  std::vector<char *> charList; //same for char
  std::vector<std::function<void(std::vector<uint16_t>)>> funList; //same for function calls
  std::vector<String> varStack; //objects and arrays store their names in a stack
  bool collectNumbers = false; //array can ask to store all numbers found in array (now used for x,y,z coordinates)
  std::vector<uint16_t> uint16CollectList; //collected numbers
  char lastVarId[128] = ""; //last found var id in json
  char beforeLastVarId[128] = ""; //last found var id in json
  size_t foundCounter = 0; //count how many of the id's to lookFor have been actually found
  bool foundAll = false;

  //called by lookedFor, store the var details in varDetails
  void addToVars(const char * id, const char * type, size_t index);

  void next();

  void check(char * varId, char * value = nullptr);

  //writeJsonVariantToFile calls itself recursively until whole json document has been parsed
  void writeJsonVariantToFile(JsonVariant variant);

};