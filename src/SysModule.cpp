/*
   @title     StarBase
   @file      SysModule.cpp
   @date      20241105
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModule.h"

#include "Sys/SysModUI.h"

void SysModule::addPresets(JsonObject parentVar) {
  Variable parentVariable = Variable(parentVar);
  Variable currentVar = ui->initSelect(parentVariable, "preset", (uint8_t)0);

  currentVar.subscribe(onUI, [this](Variable variable, uint8_t rowNr, uint8_t eventType) {
    ppf("publish preset.onUI %s.%s [%d]\n", variable.pid(), variable.id(), rowNr);
    JsonArray options = variable.setOptions();

    JsonArray modulePresets = mdl->presets->as<JsonObject>()[name];

    options.add("None");
    for (int i=0; i<modulePresets.size(); i++) {
      StarString buf;
      if (modulePresets[i].isNull() || modulePresets[i]["name"].isNull()) {
        buf.format("%02d: Empty", i);
      } else {
        // buf.format("%02d: %s", i, modulePresets[i]["name"].as<const char *>()); //this causes crashes in asyncwebserver !!!
        buf = modulePresets[i]["name"].as<const char *>();
        ppf("preset.onUI %02d %s (%d)\n", i, buf.getString(), buf.length());
      }
      options.add(buf.getString()); //copy!
    }

    // print->printJson("  options", options);

  });

  currentVar.subscribe(onChange, [this](Variable variable, uint8_t rowNr, uint8_t eventType) {

    //on Change: save the old value, retrieve the new value and set all values
    //load the complete presets.json, make the changes, save it (using save button...

    uint8_t presetValue = variable.var["value"];
    ppf("publish preset.onchange %s.%s [%d] %d %s\n", variable.pid(), variable.id(), rowNr, presetValue, variable.valueString().c_str());

    if (presetValue == 0) return; else presetValue--;

    JsonObject allPresets = mdl->presets->as<JsonObject>();

    if (!allPresets.isNull()) {

      JsonArray modulePresets = allPresets[name];
      if (!modulePresets.isNull() && presetValue < modulePresets.size()) {

        mdl->resetPresetThreshold--;

        for (JsonPair pidPair: modulePresets[presetValue].as<JsonObject>()) {
          for (JsonPair idPair: pidPair.value().as<JsonObject>()) {
            ppf("load %s.%s: %s\n", pidPair.key().c_str(), idPair.key().c_str(), idPair.value().as<String>().c_str());
            if (pidPair.key() != "name") {
              JsonVariant jv = idPair.value();
              if (jv.is<JsonArray>()) {
                uint8_t rowNr = 0;
                for (JsonVariant element: jv.as<JsonArray>()) {
                  mdl->setValue(pidPair.key().c_str(), idPair.key().c_str(), element, rowNr++);
                }
              }
              else
                mdl->setValue(pidPair.key().c_str(), idPair.key().c_str(), jv);
            }
          }
        }

        mdl->resetPresetThreshold++;

      }
    }

  });

  currentVar = ui->initButton(parentVariable, "assignPreset", false);

  currentVar.subscribe(onUI, [this](Variable variable, uint8_t rowNr, uint8_t eventType) {
    variable.setLabel("✅");
  });

  currentVar.subscribe(onChange, [this, &parentVariable](Variable variable, uint8_t rowNr, uint8_t eventType) {
    ppf("assignPreset.onChange\n");
    //save this to the first free slot
    //give that a name
    //select that one

    Variable presetVariable = Variable(name, "preset");

    uint8_t presetValue = presetVariable.var["value"];

    presetValue--; // will be UINT8_MAX if preset None

    JsonObject allPresets = mdl->presets->as<JsonObject>();
    if (allPresets.isNull()) allPresets = mdl->presets->to<JsonObject>(); //create

    JsonArray modulePresets = allPresets[name];
    if (modulePresets.isNull()) modulePresets = allPresets[name].to<JsonArray>();

    JsonObject m = mdl->findVar("m", name); //find this module (effects)

    // print->printJson("m", m); ppf("\n");
    // print->printJson("pv", parentVariable.var); ppf("\n");

    uint8_t presetIndex = 0;
    if (presetValue < modulePresets.size() && modulePresets[presetValue].isNull()) { //if slot is null use it
      presetIndex = presetValue;
    } else {
      //find the first empty slot, starting from current position, if none, add 
      //loop over slots
      for (auto modulePreset: modulePresets) {
        if (presetIndex >= presetValue) {
          //if slot empty use it.
          if (modulePreset.isNull()) {
            break;
          }
        }
        presetIndex++;
      }
    }
    //post: presetIndex contains empty slot or is new entry

    StarString result;

    if (!m["n"].isNull()) {
      // print->printJson("walk for", m["n"]); ppf("\n");
      modulePresets[presetIndex].to<JsonObject>();//empty
      mdl->walkThroughModel([modulePresets, presetIndex, &result](JsonObject parentVar, JsonObject var) {
        Variable variable = Variable(var);
        if (!variable.readOnly() &&  strncmp(variable.id(), "preset", 32) != 0 ) { //exclude preset
          ppf("save %s.%s: %s\n", variable.pid(), variable.id(), variable.valueString().c_str());
          modulePresets[presetIndex][variable.pid()][variable.id()] = var["value"];

          if (var["type"] == "text") {
            result += variable.valueString().c_str(); //concat
            result.catSep(", ");
          } else if (var["type"] == "select") {
            char option[32];
            if (variable.valIsArray())
              variable.getOption(option, var["value"][0]); //only one for now
            else
              variable.getOption(option, var["value"]);
            ppf("add option %s.%s[0] %s\n", variable.pid(), variable.id(), option);
            result += option; //concat
            result.catSep(", ");
          }
        }
        return JsonObject(); //don't stop
      }, m); //walk using m["n"]
    }

    if (result.length() == 0) result.format("Preset %d", presetIndex); //if no text found then default text

    modulePresets[presetIndex]["name"] = result.getString(); //store the text

    presetVariable.publish(onUI); //reload ui for new list of values

    mdl->resetPresetThreshold--;
    if (presetIndex != presetValue) presetVariable.setValue(presetIndex+1); //set the new value, if changed, add 1 for None
    mdl->resetPresetThreshold++;

  });

  currentVar = ui->initButton(parentVariable, "clearPreset", false); //clear preset

  currentVar.subscribe(onUI, [this](Variable variable, uint8_t rowNr, uint8_t eventType) {
    variable.setLabel("❌");
  });
  
  currentVar.subscribe(onChange, [this](Variable variable, uint8_t rowNr, uint8_t eventType) {
    ppf("clearPreset.onChange\n");
    //free this slot
    //remove the name

    Variable presetVariable = Variable(name, "preset");

    uint8_t presetValue = presetVariable.var["value"];

    if (presetValue == 0) return; else presetValue--; //makes no sense to clear None

    JsonObject allPresets = mdl->presets->as<JsonObject>();

    if (!allPresets.isNull()) {

      JsonArray modulePresets = allPresets[name];
      if (!modulePresets.isNull()) {

        if (presetValue < modulePresets.size()) {

          modulePresets[presetValue] = (char*)0; // set element in valArray to 0 (is content deleted from memory?)

          // presetVariable.publish(onUI); //reload ui for new list of values
        }

        uint8_t presetIndex = presetValue;
        //cleanup
        while (modulePresets.size() && modulePresets[modulePresets.size()-1].isNull()) {
          modulePresets.remove(modulePresets.size()-1);
          presetIndex = modulePresets.size() - 1;
        }

        presetVariable.publish(onUI); //reload ui for new list of values

        mdl->resetPresetThreshold--;
        if (presetIndex != presetValue) presetVariable.setValue(presetIndex+1); //set the new value, if changed, add 1 for none
        mdl->resetPresetThreshold++;
      }
    }

  });

  // ui->initVCR(parentVariable, "vcr", false); //for next release

}