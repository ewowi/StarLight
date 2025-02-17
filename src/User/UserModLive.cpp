/*
   @title     StarBase
   @file      UserModLive.h
   @date      20241219
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors, asmParser © https://github.com/hpwit/ASMParser
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#ifdef STARBASE_USERMOD_LIVE //don't know why to exclude the .cpp as the .h is not included in this case ...

#include "UserModLive.h"

// #include <Arduino.h>

#include "../Sys/SysModPrint.h"
#include "../Sys/SysModUI.h"
#include "../Sys/SysModSystem.h"
#include "../Sys/SysModFiles.h"

//Leds specifiv
#include "../App/LedModFixture.h" //for fix->driver (temp)
//End Leds specific

// #define __RUN_CORE 0

#define USE_FASTLED //as ESPLiveScript.h calls hsv !

#include "ESPLiveScript.h" //note: contains declarations AND definitions, therefore can only be included once!

  static long previousCycleCount;
  static uint16_t fps;
  static unsigned long frameCounter; //temp, can be removed if syncing tested
  static bool waitingOnLiveScript = true;
  static bool syncActive = false;

Parser parser = Parser();

 void UserModLive::preKill()
{
  ppf("ELS preKill\n");
  // LEDS specific
  //tbd: move this to LedModFixture...
  #if STARLIGHT_PHYSICAL_DRIVER || STARLIGHT_VIRTUAL_DRIVER
    driver.__enableDriver=false;
    while (driver.isDisplaying){};
    //delay(20);
  #endif
}
 void UserModLive::postKill()
{
  ppf("ELS postKill\n");
  // LEDS specific
  #if STARLIGHT_PHYSICAL_DRIVER || STARLIGHT_VIRTUAL_DRIVER
    // delay(10);
    driver.__enableDriver=true;
  #endif
}

static float _triangle(float j) {return 1.0 - fabs(fmod(2 * j, 2.0) - 1.0);}
static float _time(float j) {
  float myVal = sys->now;
  myVal = myVal / 65535 / j;           // PixelBlaze uses 1000/65535 = .015259. 
  myVal = fmod(myVal, 1.0);               // ewowi: with 0.015 as input, you get fmod(millis/1000,1.0), which has a period of 1 second, sounds right
  return myVal;
}

  void UserModLive::setup() {
    SysModule::setup();

    //note: -mtext-section-literals needed in pio.ini, first only for s2, now for all due to something in setup...

    const Variable parentVar = ui->initUserMod(Variable(), name, 6310);

    ui->initSelect(parentVar, "script", UINT8_MAX, false, [this](EventArguments) { switch (eventType) {
      case onUI: {
        // variable.setComment("Fixture to display effect on");
        JsonArray options = variable.setOptions();
        options.add("None");
        files->dirToJson(options, true, ".sc"); //only files containing F(ixture), alphabetically

        return true; }
      case onChange: {
        //set script
        uint8_t fileNr = variable.value();

        ppf("%s.script.onChange f:%d\n", name, fileNr);

        char path[64] = "";

        if (fileNr > 0 && fileNr != UINT8_MAX) { //not None and setup done
          fileNr--;  //-1 as none is no file
          files->seqNrToName(path, fileNr, ".sc");
          ppf("script.onChange f:%d n:%s\n", fileNr, path);

          uint8_t exeID = liveM->findExecutable(path);
          if (exeID == UINT8_MAX) {

            addDefaultExternals();

            //to run blinkSL.sc
            addExternalFun("void", "pinMode", "(int a1, int a2)", (void *)&pinMode);
            addExternalFun("void", "digitalWrite", "(int a1, int a2)", (void *)&digitalWrite);
            addExternalFun("void", "delay", "(int a1)", (void *)&delay);

            exeID = compile(path, "void main(){setup();while(2>1){loop();sync();}}");
          }

          if (exeID != UINT8_MAX)
            liveM->executeBackgroundTask(exeID);
          else 
            ppf("mapInitAlloc task not created (compilation error?) %s\n", path);
        }
        else {
          // kill();
          ppf("LiveScripts.script.onChange set to None\n");
        }

        return true; }
      default: return false; 
    }}); //script

    ui->initText(parentVar, "fpsCycles", nullptr, 10, true, [this](EventArguments) { switch (eventType) {
      case onLoop1s:
        variable.setValueF("%d /s", fps, 0); //0 is to force format overload used
        return true;
      default: return false; 
    }});

    //temp
    ui->initText(parentVar, "fpsSync", nullptr, 10, true, [this](EventArguments) { switch (eventType) {
      case onLoop1s:
        variable.setValueF("%d /s", frameCounter, 0); //0 is to force format overload used
        frameCounter = 0;
        return true;
      default: return false; 
    }});

    Variable tableVar = ui->initTable(parentVar, "scripts", nullptr, true);

    //set the values every second
    tableVar.subscribe(onLoop1s, [](Variable variable, uint8_t rowNr, uint8_t eventType) {
      for (JsonObject childVar: variable.children())
        Variable(childVar).triggerEvent(onSetValue);
    }); 

    ui->initText(tableVar, "name", nullptr, 32, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        variable.var["value"].to<JsonArray>(); web->addResponse(variable.var, "value", variable.value()); // empty the value
        rowNr = 0;
        for (Executable &exec: scriptRuntime._scExecutables) {
          const char *name = exec.name.c_str();
          variable.setValue(JsonString(exec.name.c_str()), rowNr++);
        }
        return true;
      default: return false;
    }});

    ui->initCheckBox(tableVar, "running", UINT8_MAX, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        variable.var["value"].to<JsonArray>(); web->addResponse(variable.var, "value", variable.value()); // empty the value
        rowNr = 0;
        for (Executable &exec: scriptRuntime._scExecutables) {
          variable.setValue(exec.isRunning(), rowNr++);
        }
        return true;
      default: return false;
    }});

    ui->initCheckBox(tableVar, "halted", UINT8_MAX, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        variable.var["value"].to<JsonArray>(); web->addResponse(variable.var, "value", variable.value()); // empty the value
        rowNr = 0;
        for (Executable &exec: scriptRuntime._scExecutables) {
          variable.setValue(exec.isHalted, rowNr++);
        }
        return true;
      default: return false;
    }});

    ui->initCheckBox(tableVar, "exe", UINT8_MAX, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        variable.var["value"].to<JsonArray>(); web->addResponse(variable.var, "value", variable.value()); // empty the value
        rowNr = 0;
        for (Executable &exec: scriptRuntime._scExecutables) {
          variable.setValue(exec.exeExist, rowNr++);
        }
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "handle", UINT16_MAX, 0, UINT16_MAX, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        variable.var["value"].to<JsonArray>(); web->addResponse(variable.var, "value", variable.value()); // empty the value
        rowNr = 0;
        for (Executable &exec: scriptRuntime._scExecutables) {
          variable.setValue(exec.__run_handle_index, rowNr++);
        }
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "size", nullptr, 32, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        variable.var["value"].to<JsonArray>(); web->addResponse(variable.var, "value", variable.value()); // empty the value
        rowNr = 0;
        for (Executable &exec: scriptRuntime._scExecutables) {
          exe_info exeInfo = scriptRuntime.getExecutableInfo(exec.name);
          StarString text;
          text.format("%d+%d=%d B", exeInfo.binary_size, exeInfo.data_size, exeInfo.total_size);
          variable.setValue(JsonString(text.getString()), rowNr++);
        }
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "error", nullptr, 32, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        variable.var["value"].to<JsonArray>(); web->addResponse(variable.var, "value", variable.value()); // empty the value
        rowNr = 0;
        for (Executable &exec: scriptRuntime._scExecutables) {
          if (exec.error.error) {
            StarString text;
            const char *error_message = exec.error.error_message.c_str();
            text.format("%d-%d %s (%d)", exec.error.line, exec.error.pos, error_message?error_message:"dev", exec.error.error);
            variable.setValue(JsonString(text.getString()), rowNr++);
          }
        }
        return true;
      default: return false;
    }});

    ui->initButton(tableVar, "Kill", false, [this](EventArguments) { switch (eventType) {
      case onChange:
        if (rowNr < scriptRuntime._scExecutables.size())
          killAndDelete(scriptRuntime._scExecutables[rowNr].name.c_str());
        else
          ppf("dev try to kill a script which does not exist anymore... (%d)\n", rowNr);
        return true;
      default: return false;
    }});

    runningPrograms.setPrekill(preKill, postKill); //for clockless driver...
    runningPrograms.setFunctionToSync(sync);

  } //setup

  //Live Scripts defaults
  void UserModLive::addDefaultExternals() {

    scScript = "";

    addExternalFun("uint32_t", "millis", "", (void *)millis);
    float (*_sin)(float) = sin;
    addExternalFun("float", "sin", "float", (void *)_sin);
    float (*_cos)(float) = cos;
    addExternalFun("float", "cos", "float", (void *)_cos);
    float (*_atan2)(float, float) = atan2;
    addExternalFun("float", "atan2", "float,float",(void*)_atan2);
    float (*_hypot)(float, float) = hypot;
    addExternalFun("float", "hypot", "float,float",(void*)_hypot);
    addExternalFun("float", "time", "float", (void *)_time);
    addExternalFun("float", "triangle", "float", (void *)_triangle);
  }

  void UserModLive::addExternalVal(string result, string name, void * ptr) {
    if (findLink(name, externalType::value) == -1) //not allready added earlier
      addExternalVariable(name, result, "", ptr);
  }

  void UserModLive::addExternalFun(string result, string name, string parameters, void * ptr) {
    if (findLink(name, externalType::function) == -1) //not allready added earlier
      addExternalFunction(name, result, parameters, ptr);
  }

  // void UserModLive::addExternalFun(string name, std::function<void(int)> fun) {
  //   addExternal(name, externalType::function, (void *)&fun)); //unfortionately InstructionFetchError, why does it work in initText etc?
  //   ppf("external %s(int arg1);\n", name.c_str()); //add to string
  // }
  // void UserModLive::addExternalFun(string name, std::function<void(int, int)> fun) {
  //   addExternal(name, externalType::function, (void *)&fun); //unfortionately InstructionFetchError
  //   ppf("external %s(int arg1, int arg2);\n", name.c_str()); //add to string
  // }

  void UserModLive::sync() {
    frameCounter++; //temp
      
    fps = ESP.getCpuFreqMHz() * 1000000 / (ESP.getCycleCount() - previousCycleCount);
    previousCycleCount = ESP.getCycleCount();

      // Show fps (fps2): is driver.showpixels() this allows to check that there is no issue with the driver
      // The global (fps3): is the result of having driver.showPixel and animation sequentially
      // Fps animation (fps): fps for the calculation of one frame
      // The global fps equals theorically (fpsshow x fpsanimation) / (fpsshow + fpsanimation)
      // So if what you display is the seen fps( animation & driver.showPixel) you should see the global FPS.
      // fps is shown as fps1 in the ui, frameCounter is shown as fps2 in the ui. 

    delay(1); //to feed the watchdog (also if loopState == 0)
    while (!waitingOnLiveScript) delay(1); //to feed the watchdog
    //do Live Script cycle
    waitingOnLiveScript = false; //Live Script produced a frame, main loop will deal with it
  }

  void UserModLive::syncWithSync() {

    if (syncActive && !waitingOnLiveScript) {// show has been called (in other loop)
      waitingOnLiveScript = true; //waiting on Live Script
      while (waitingOnLiveScript) delay(1); // so live can continue
    }
  }

  void UserModLive::loop1s() {
    //check if sync is active (to do: only if background process? check hpwit)
    bool scriptsRunning = false;
    for (Executable &exec: scriptRuntime._scExecutables) {
      if (exec.isRunning())
        scriptsRunning = true;
    }
    syncActive = scriptsRunning;
    if (!syncActive) waitingOnLiveScript = true; //reset to default

  }

  // void UserModLive::loop20ms() {
    //workaround temporary disabled (replace by run?)
    // if (strnstr(web->lastFileUpdated, ".sc", sizeof(web->lastFileUpdated)) != nullptr) {
    //   if (strnstr(web->lastFileUpdated, "del:/", sizeof(web->lastFileUpdated)) != nullptr) {
    //     if (strncmp(this->fileName, web->lastFileUpdated+4, sizeof(this->fileName)) == 0) { //+4 remove del:
    //       ppf("loop20ms kill %s\n", web->lastFileUpdated);
    //       kill();
    //     }
    //     //else nothing
    //   }
    //   else {
    //     ppf("loop20ms run %s -> %s\n", this->fileName, web->lastFileUpdated);
    //     run(web->lastFileUpdated);
    //   }
    //   strlcpy(web->lastFileUpdated, "", sizeof(web->lastFileUpdated));
    // }
  // }

  void UserModLive::executeTask(uint8_t exeID, const char * function, int val)
  {
    if (val == UINT16_MAX)
      scriptRuntime._scExecutables[exeID].execute(string(function));
    else {
      Arguments arguments;
      arguments.add(val);
      scriptRuntime._scExecutables[exeID].execute(string(function), arguments);
    }
  }

  void UserModLive::executeBackgroundTask(uint8_t exeID, const char * function)
  {
    scriptRuntime._scExecutables[exeID].executeAsTask(string(function));
  }

  uint8_t UserModLive::compile(const char * path, const char * post) {
    ppf("live compile n:%s o:%s \n", path, this->path);

    File f = files->open(path, FILE_READ);
    if (!f) {
      ppf("UserModLive setup script open %s for %s failed\n", path, FILE_READ);
      return UINT8_MAX;
    } else {

      if (strnstr(path, ".sc.bin", 6) != nullptr) {
        Binary bin2;
        loadBinary((char *)path, LittleFS, &bin2);
        Executable ex;
        ex.createExecutableFromBinary(&bin2);
        ppf("UserModLive setup script %s binary\n", path);
        scriptRuntime.addExe(ex);
        return scriptRuntime._scExecutables.size() - 1;;
      }

      unsigned preScriptNrOfLines = 0;

      for (size_t i = 0; i < scScript.length(); i++)
      {
        if (scScript[i] == '\n')
          preScriptNrOfLines++;
      }
      ppf("preScript of %s has %d lines\n", path, preScriptNrOfLines+1); //+1 to subtract the line from parser error line reported

      scScript += string(f.readString().c_str()); // add sc file
      f.close();

      if (post) scScript += post;

      //print the script
      size_t scripLines = 0;
      size_t lastIndex = 0;
      for (size_t i = 0; i < scScript.length(); i++)
      {
        if (scScript[i] == '\n' || i == scScript.length()-1) {
          ppf("%3d %s", scripLines+1, scScript.substr(lastIndex, i-lastIndex+1).c_str());
          scripLines++;
          lastIndex = i + 1;
        }
      }
      ppf("\n");

      ppf("Before parsing of %s\n", path);
      ppf("Heap %s:%d f:%d / t:%d (l:%d) B [%d %d]\n", __FUNCTION__, __LINE__, ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap(), esp_get_free_heap_size(), esp_get_free_internal_heap_size());
      ppf("Stack %d of %d B %d\n", sys->sysTools_get_arduino_maxStackUsage(), getArduinoLoopTaskStackSize(), uxTaskGetStackHighWaterMark(xTaskGetCurrentTaskHandle()));

      Executable executable = parser.parseScript(&scScript);
      executable.name = string(path);
      Binary bin = parser.parseScriptBinary(&scScript);
      if (!bin.error.error) {
        char binName[32];
        strcpy(binName, path);
        strcat(binName, ".bin");
        saveBinary(binName, LittleFS, &bin);
        freeBinary(&bin);
      }

      ppf("parsing %s done\n", path);
      ppf("%s:%d f:%d / t:%d (l:%d) B [%d %d]\n", __FUNCTION__, __LINE__, ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap(), esp_get_free_heap_size(), esp_get_free_internal_heap_size());

      scriptRuntime.addExe(executable);

      if (executable.exeExist) {
        ppf("exe created %d\n", scriptRuntime._scExecutables.size());
        return scriptRuntime._scExecutables.size() - 1;
      } else {
        ppf("exe failed %d\n", scriptRuntime._scExecutables.size());
        return UINT8_MAX;
      }
    } //file open
  } //compile

  void UserModLive::killAndDelete(const char *name) {
    if (name != nullptr) { 
      scriptRuntime.kill(string(name));
      scriptRuntime.deleteExe(string(name));
    } else {
      scriptRuntime.killAndFreeRunningProgram();
    }

    if (waitingOnLiveScript) {
      waitingOnLiveScript = false;
      ppf("waitingOnLiveScript killAndDelete %d\n", waitingOnLiveScript);
    }

    // fix->liveFixtureID = nullptr; //to be sure! todo: nullify exec pointers fix->liveFixtureID and leds.liveEffectID
  }

  void UserModLive::killAndDelete(uint8_t exeID) {
    if (exeID < scriptRuntime._scExecutables.size())
      killAndDelete(scriptRuntime._scExecutables[exeID].name.c_str());
  }

  uint8_t UserModLive::findExecutable(const char *path) {
    uint8_t exeID = 0;
    for (Executable &exec: scriptRuntime._scExecutables) {
      if (exec.name.compare(string(path)) == 0 && exec.isExeExists()) //successfully compiled
        return exeID++;
    }
    return UINT8_MAX;
  }

  #endif
