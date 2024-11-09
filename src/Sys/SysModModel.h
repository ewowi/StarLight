/*
   @title     StarBase
   @file      SysModModel.h
   @date      20241105
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
// #include "SysModule.h"
#include "SysModPrint.h"
#include "SysModWeb.h"
// #include "SysModules.h" //isConnected

struct Coord3D {
  int x;
  int y;
  int z;

  //int as Coordinates can go negative in some effects
  
  // Coord3D() {
  //   x = 0;
  //   y = 0;
  //   z = 0;
  // }
  // Coord3D(uint16_t x, uint16_t y, uint16_t z) {
  //   this->x = x;
  //   this->y = y;
  //   this->z = y;
  // }

  //comparisons
  bool operator!=(Coord3D rhs) {
    // ppf("Coord3D compare%d %d %d %d %d %d\n", x, y, z, rhs.x, rhs.y, rhs.z);
    // return x != rhs.x || y != rhs.y || z != rhs.z;
    return !(*this==rhs);
  }
  bool operator==(Coord3D rhs) {
    return x == rhs.x && y == rhs.y && z == rhs.z;
  }
  bool operator>=(Coord3D rhs) {
    return x >= rhs.x && y >= rhs.y && z >= rhs.z;
  }
  bool operator<=(Coord3D rhs) {
    return x <= rhs.x && y <= rhs.y && z <= rhs.z;
  }
  bool operator<(Coord3D rhs) {
    return x < rhs.x && y < rhs.y && z < rhs.z;
  }
  bool operator>=(int rhs) {
    return x >= rhs && y >= rhs && z >= rhs;
  }

  //assignments
  Coord3D operator=(Coord3D rhs) {
    // ppf("Coord3D assign %d,%d,%d\n", rhs.x, rhs.y, rhs.z);
    x = rhs.x;
    y = rhs.y;
    z = rhs.z;
    return *this;
  }
  Coord3D operator+=(Coord3D rhs) {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
  }
  Coord3D operator-=(Coord3D rhs) {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
  }
  Coord3D operator*=(Coord3D rhs) {
    x *= rhs.x;
    y *= rhs.y;
    z *= rhs.z;
    return *this;
  }
  Coord3D operator/=(Coord3D rhs) {
    if (rhs.x) x /= rhs.x;
    if (rhs.y) y /= rhs.y;
    if (rhs.z) z /= rhs.z;
    return *this;
  }
  //Minus / delta (abs)
  Coord3D operator-(Coord3D rhs) {
    Coord3D result;
    // result.x = x > rhs.x? x - rhs.x : rhs.x - x;
    // result.y = y > rhs.y? y - rhs.y : rhs.y - y;
    // result.z = z > rhs.z? z - rhs.z : rhs.z - z;
    result.x = x - rhs.x;
    result.y = y - rhs.y;
    result.z = z - rhs.z;
    return result;
  }
  Coord3D operator+(Coord3D rhs) {
    return Coord3D{x + rhs.x, y + rhs.y, z + rhs.z};
  }
  Coord3D operator*(Coord3D rhs) {
    return Coord3D{x * rhs.x, y * rhs.y, z * rhs.z};
  }
  Coord3D operator/(Coord3D rhs) {
    return Coord3D{x / rhs.x, y / rhs.y, z / rhs.z};
  }
  Coord3D operator%(Coord3D rhs) {
    return Coord3D{x % rhs.x, y % rhs.y, z % rhs.z};
  }
  Coord3D minimum(Coord3D rhs) {
    return Coord3D{min(x, rhs.x), min(y, rhs.y), min(z, rhs.z)};
  }
  Coord3D maximum(Coord3D rhs) {
    return Coord3D{max(x, rhs.x), max(y, rhs.y), max(z, rhs.z)};
  }
  Coord3D operator*(uint8_t rhs) {
    return Coord3D{x * rhs, y * rhs, z * rhs};
  }
  Coord3D operator/(uint8_t rhs) {
    return Coord3D{x / rhs, y / rhs, z / rhs};
  }
  //move the coordinate one step closer to the goal, if difference in coordinates (used in GenFix)
  Coord3D advance(Coord3D goal, uint8_t step) {
    if (x != goal.x) x += (x<goal.x)?step:-step;
    if (y != goal.y) y += (y<goal.y)?step:-step;
    if (z != goal.z) z += (z<goal.z)?step:-step;
    return *this;
  }
  unsigned distance(Coord3D rhs) {
    Coord3D delta = (*this-rhs);
    return sqrt((delta.x)*(delta.x) + (delta.y)*(delta.y) + (delta.z)*(delta.z));
  }
  unsigned distanceSquared(Coord3D rhs) {
    Coord3D delta = (*this-rhs);
    return (delta.x)*(delta.x) + (delta.y)*(delta.y) + (delta.z)*(delta.z);
  }
  bool isOutofBounds(Coord3D rhs) {
    return x < 0 || y < 0 || z < 0 || x >= rhs.x || y >= rhs.y || z >= rhs.z;
  }
};

//used to sort keys of jsonobjects
struct ArrayIndexSortValue {
  size_t index;
  uint32_t value;
};

//https://arduinojson.org/news/2021/05/04/version-6-18-0/
namespace ArduinoJson {
  template <>
  struct Converter<Coord3D> {
    static bool toJson(const Coord3D& src, JsonVariant dst) {
      // JsonObject obj = dst.to<JsonObject>();
      dst["x"] = src.x;
      dst["y"] = src.y;
      dst["z"] = src.z;
      // ppf("Coord3D toJson %d,%d,%d -> %s\n", src.x, src.y, src.z, dst.as<String>().c_str());
      return true;
    }

    static Coord3D fromJson(JsonVariantConst src) {
      // ppf("Coord3D fromJson %s\n", src.as<String>().c_str());
      return Coord3D{src["x"], src["y"], src["z"]};
    }

    static bool checkJson(JsonVariantConst src) {
      return src["x"].is<uint16_t>() && src["y"].is<uint16_t>() && src["z"].is<uint16_t>();
    }
  };
}

// https://arduinojson.org/v7/api/jsondocument/
struct RAM_Allocator: ArduinoJson::Allocator {
  void* allocate(size_t size) override {
    if (psramFound()) return ps_malloc(size); // use PSRAM if it exists
    else              return malloc(size);    // fallback
    // return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  }
  void deallocate(void* pointer) override {
    free(pointer);
    // heap_caps_free(pointer);
  }
  void* reallocate(void* ptr, size_t new_size) override {
    if (psramFound()) return ps_realloc(ptr, new_size); // use PSRAM if it exists
    else              return realloc(ptr, new_size);    // fallback
    // return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
  }
};

enum eventTypes
{
  onSetValue,
  onUI,
  onChange,
  onLoop,
  onLoop1s,
  onAdd,
  onDelete,
  f_count
};


class Variable {
  public:

  JsonObject var;

  Variable() {this->var = JsonObject();} //undefined variable
  Variable(JsonObject var) {this->var = var;}

  //core methods 
  const char *pid() {return var["pid"];}
  const char *id() {return var["id"];}

  JsonVariant value() {return var["value"];}
  JsonVariant value(uint8_t rowNr) {return var["value"][rowNr];}

  String valueString(uint8_t rowNr = UINT8_MAX);

  int order() {return var["o"];}
  void order(int value) {var["o"] = value;}

  bool readOnly() {return var["ro"];}
  void readOnly(bool value) {var["ro"] = value;}

  JsonArray children() {return var["n"];}

  void defaultOrder(int value) {if (order() > -1000) order(- value); } //set default order (in range >=1000). Don't use auto generated order as order can be changed in the ui (WIP)

  //recursively remove all value[rowNr] from children of var
  void removeValuesForRow(uint8_t rowNr);

  JsonArray valArray() {if (var["value"].is<JsonArray>()) return var["value"]; else return JsonArray(); }

  //if variable is a table, loop through its rows
  void rows(std::function<void(Variable, uint8_t)> fun = nullptr);

  //extra methods

  void preDetails();

  void postDetails(uint8_t rowNr);

  //checks if var has fun of type eventType implemented by calling it and checking result (for onUI on RO var, also onSetValue is called)
  //onChange: sends dash var change to udp (if init),  sets pointer if pointer var and run onChange
  bool triggerEvent(uint8_t eventType = onSetValue, uint8_t rowNr = UINT8_MAX, bool init = false);

  void setLabel(const char * text);
  void setComment(const char * text);
  JsonArray setOptions();
  //return the options from onUI (don't forget to clear responseObject)
  JsonArray getOptions();
  void clearOptions();

  //find options text in a hierarchy of options
  void findOptionsText(uint8_t value, char * groupName, char * optionName);

  // (groupName and optionName as pointers? String is already a pointer?)
  bool findOptionsTextRec(JsonVariant options, uint8_t * startValue, uint8_t value, JsonString *groupName, JsonString *optionName, JsonString parentGroup = JsonString());

  //setValue for JsonVariants (extract the StarMod supported types)
  JsonObject setValueJV(JsonVariant value, uint8_t rowNr = UINT8_MAX);

  template <typename Type>
  JsonObject setValue(Type value, uint8_t rowNr = UINT8_MAX) {
    bool changed = false;

    if (rowNr == UINT8_MAX) { //normal situation
      if (var["value"].isNull() || var["value"].as<Type>() != value) { //const char * will be JsonString so comparison works
        if (!var["value"].isNull() && !readOnly()) var["oldValue"] = var["value"];
        var["value"] = value;
        //trick to remove null values
        if (var["value"].isNull() || var["value"].as<uint16_t>() == UINT16_MAX) {
          var.remove("value");
          // ppf("dev setValue value removed %s %s\n", id(), var["oldValue"].as<String>().c_str());
        }
        else {
          //only print if ! read only
          // if (!readOnly())
          //   ppf("setValue changed %s.%s %s -> %s\n", pid(), id(), var["oldValue"].as<String>().c_str(), valueString().c_str());
          // else
          //   ppf("setValue changed %s %s\n", id(), var["value"].as<String>().c_str());
          web->addResponse(var, "value", var["value"]);
          changed = true;
        }
      }
    }
    else {
      //if we deal with multiple rows, value should be an array, if not we create one

      if (var["value"].isNull() || !var["value"].is<JsonArray>()) {
        // ppf("setValue var %s[%d] value %s not array, creating\n", id(), rowNr, var["value"].as<String>().c_str());
        var["value"].to<JsonArray>();
      }

      if (var["value"].is<JsonArray>()) {
        JsonArray valueArray = valArray();
        //set the right value in the array (if array did not contain values yet, all values before rownr are set to false)
        bool notSame = true; //rowNr >= size

        if (rowNr < valueArray.size())
          notSame = valueArray[rowNr].isNull() || valueArray[rowNr].as<Type>() != value;

        if (notSame) {
          // if (rowNr >= valueArray.size())
          //   ppf("notSame %d %d\n", rowNr, valueArray.size());
          valueArray[rowNr] = value; //if valueArray[<rowNr] not exists it will be created
          // ppf("  assigned %d %d %s\n", rowNr, valueArray.size(), valueArray[rowNr].as<String>().c_str());
          web->addResponse(var, "value", var["value"]); //send the whole array to UI as response is in format value:<value> !!
          changed = true;
        }
      }
      else {
        ppf("setValue %s.%s could not create value array\n", pid(), id());
      }
    }

    if (changed) triggerEvent(onChange, rowNr);
    
    return var;    
  }

  //Set value with argument list
  JsonObject setValueF(const char * format = nullptr, ...);

  JsonVariant getValue(uint8_t rowNr = UINT8_MAX);

}; //class Variable

typedef std::function<void(Variable)> FindFun;

#define EventArguments Variable variable, uint8_t rowNr, uint8_t eventType

// https://stackoverflow.com/questions/59111610/how-do-you-declare-a-lambda-function-using-typedef-and-then-use-it-by-passing-to
typedef std::function<uint8_t(EventArguments)> VarEvent;

class SysModModel: public SysModule {

public:

  RAM_Allocator allocator;
  JsonDocument *model = nullptr;

  bool doWriteModel = false;

  uint8_t setValueRowNr = UINT8_MAX;
  uint8_t getValueRowNr = UINT8_MAX;
  int varCounter = 1; //start with 1 so it can be negative, see var["o"]

  std::vector<VarEvent> varEvents;

  SysModModel();
  void setup();
  void loop20ms();
  void loop1s();
  
  //scan all vars in the model and remove vars where var["o"] is negative or positive, if ro then remove ro values
  void cleanUpModel(Variable parent = Variable(), bool oPos = true, bool ro = false);

  //sets the value of var with id
  template <typename Type>
  JsonObject setValue(const char * pid, const char * id, Type value, uint8_t rowNr = UINT8_MAX) {
    JsonObject var = findVar(pid, id);
    if (!var.isNull()) {
      return Variable(var).setValue(value, rowNr);
    }
    else {
      ppf("setValue var %s.%s not found\n", pid, id);
      return JsonObject();
    }
  }

  JsonVariant getValue(const char * pid, const char * id, uint8_t rowNr = UINT8_MAX) {
    JsonObject var = findVar(pid, id);
    if (!var.isNull()) {
      return Variable(var).getValue(rowNr);
    }
    else {
      // ppf("getValue: Var %s does not exist!!\n", id);
      return JsonVariant();
    }
  }

  //returns the var defined by id (parent to recursively call findVar)
  bool walkThroughModel(std::function<bool(JsonObject)> fun, JsonObject parent = JsonObject());
  JsonObject findVar(const char * pid, const char * id, JsonObject parent = JsonObject());
  void findVars(const char * id, bool value, FindFun fun, JsonArray parent = JsonArray());

  uint8_t linearToLogarithm(uint8_t value, uint8_t minp = 0, uint8_t maxp = UINT8_MAX) {
    if (value == 0) return 0;

    // float minp = var["min"].isNull()?var["min"]:0;
    // float maxp = var["max"].isNull()?var["max"]:255;

    // The result should be between 100 an 10000000
    float minv = minp?log(minp):0;
    float maxv = log(maxp);

    // calculate adjustment factor
    float scale = (maxv-minv) / (maxp-minp);

    return round(exp(minv + scale*((float)value-minp)));
  }

private:
  bool cleanUpModelDone = false;

};

extern SysModModel *mdl;