/*
   @title     StarBase
   @file      UserModLive.h
   @date      20241014
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors, asmParser © https://github.com/hpwit/ASMParser
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "UserModLive.h"

#include "ESPLiveScript.h" //note: contains declarations AND definitions, therefore can only be included once!

// #include <Arduino.h>

#include "../Sys/SysModPrint.h"
#include "../Sys/SysModUI.h"
#include "../Sys/SysModSystem.h"
#include "../Sys/SysModFiles.h"

// #define __RUN_CORE 0

long time1;
long time4;
static float _min = 9999;
static float _max = 0;
static uint32_t _nb_stat = 0;
static float _totfps;
static float fps = 0; //integer?
static unsigned long frameCounter = 0;
static uint8_t loopState = 0; //waiting on Live Script

//external function implementation (tbd: move into class)

static void show()
{
  frameCounter++;
    
  // SKIPPED: check nargs (must be 3 because arg[0] is self)
  long time2 = ESP.getCycleCount();

  // driver.showPixels(WAIT); // LEDS specific

  //show is done in LedModEffects!

  long time3 = ESP.getCycleCount();
  float k = (float)(time2 - time1) / 240000000;
  fps = 1 / k; //StarBase: class variable so it can be shown in UI!!!
  float k2 = (float)(time3 - time2) / 240000000;
  float fps2 = 1 / k2;
  float k3 = (float)(time2 - time4) / 240000000;
  float fps3 = 1 / k3;
  _nb_stat++;
  if (_min > fps && fps > 10 && _nb_stat > 10)
    _min = fps;
  if (_max < fps && fps < 5000 && _nb_stat > 10)
    _max = fps;
  if (_nb_stat > 10)
    _totfps += fps;
  if (_nb_stat%10000 == 0) //every 10 sec. (temp)
    //Serial.printf("current show fps:%.2f\tglobal fps:%.2f\tfps animation:%.2f\taverage:%.2f\tmin:%.2f\tmax:%.2f\r\n", fps2, fps3, fps, _totfps / (_nb_stat - 10), _min, _max);
    ppf("current show fps:%.2f\tglobal fps:%.2f\tfps animation:%.2f  average:%.2f min:%.2f max:%.2f\r\n",fps2, fps3,  fps, _totfps / (_nb_stat - 10), _min, _max);
  time1 = ESP.getCycleCount();
  time4 = time2;

  // SKIPPED: check that both v1 and v2 are int numbers
  // RETURN_VALUE(VALUE_FROM_INT(0), rindex);
  delay(1); //to feed the watchdog (also if loopState == 0)
  while (loopState != 0) { //not waiting on Live Script
    delay(1); //to feed the watchdog
    // set to 0 by main loop
  }
  //do Live Script cycle
  loopState = 1; //Live Script produced a frame, main loop will deal with it
  // ppf("loopState %d\n", loopState);
}

static void preKill()
{
  ppf("ELS preKill\n");
  // driver.__enableDriver=false;
  // while(driver.isDisplaying){};
  // //delay(20);
}
static void postKill()
{
  ppf("ELS postKill\n");
  //delay(10);
  // driver.__enableDriver=true;
}

static void resetShowStats()
{
    float min = 999;
    float max = 0;
    _nb_stat = 0;
    _totfps = 0;
}

static float _hypot(float x,float y) {return hypot(x,y);}
static float _atan2(float x,float y) { return atan2(x,y);}
static float _sin(float j) {return sin(j);}
static float _cos(float j) {return cos(j);}
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

    parentVar = ui->initUserMod(parentVar, name, 6310);

    ui->initSelect(parentVar, "script", UINT8_MAX, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onUI: {
        // ui->setComment(var, "Fixture to display effect on");
        JsonArray options = ui->setOptions(var);
        options.add("None");
        files->dirToJson(options, true, ".sc"); //only files containing F(ixture), alphabetically

        return true; }
      case onChange: {
        //set script
        uint8_t fileNr = var["value"];

        ppf("%s script.onChange f:%d\n", name, fileNr);

        char fileName[32] = "";

        if (fileNr > 0) { //not None and setup done
          fileNr--;  //-1 as none is no file
          files->seqNrToName(fileName, fileNr, ".sc");
          ppf("script.onChange f:%d n:%s\n", fileNr, fileName);

          scPreBaseScript = ""; //externals etc generated (would prefer String for esp32...)

          addExternals();

          //to run blinkSL.sc
          addExternalFun("void", "pinMode", "(int a1, int a2)", (void *)&pinMode);
          addExternalFun("void", "digitalWrite", "(int a1, int a2)", (void *)&digitalWrite);
          addExternalFun("void", "delay", "(int a1)", (void *)&delay);

          run(fileName, "main", "void main(){resetStat();setup();while(2>1){loop();show();}}");
        }
        else {
          kill();
          ppf("script.onChange set to None\n");
        }

        return true; }
      default: return false; 
    }}); //script

    ui->initText(parentVar, "fps1", nullptr, 10, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onLoop1s:
        mdl->setValue(var, "%.0f /s", fps, 0); //0 is to force format overload used
        return true;
      default: return false; 
    }});
    ui->initText(parentVar, "fps2", nullptr, 10, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onLoop1s:
        mdl->setValue(var, "%d /s", frameCounter, 0); //0 is to force format overload used
        frameCounter = 0;
        return true;
      default: return false; 
    }});

    JsonObject tableVar = ui->initTable(parentVar, "scripts", nullptr, true);

    ui->initText(tableVar, "name", nullptr, 32, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onSetValue:
        for (size_t rowNr = 0; rowNr < _MAX_PROG_AT_ONCE; rowNr++) {
          if (runningPrograms.execPtr[rowNr]) {
            const char *name = runningPrograms.execPtr[rowNr]->name.c_str();
            mdl->setValue(var, JsonString(name?name:"xx", JsonString::Copied), rowNr);
          }
        }
        return true;
      default: return false;
    }});
    ui->initCheckBox(tableVar, "running", UINT8_MAX, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onSetValue:
        for (size_t rowNr = 0; rowNr < _MAX_PROG_AT_ONCE; rowNr++) {
          if (runningPrograms.execPtr[rowNr])
            mdl->setValue(var, runningPrograms.execPtr[rowNr]->isRunning(), rowNr);
        }
        return true;
      default: return false;
    }});
    ui->initCheckBox(tableVar, "halted", UINT8_MAX, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onSetValue:
        for (size_t rowNr = 0; rowNr < _MAX_PROG_AT_ONCE; rowNr++) {
          if (runningPrograms.execPtr[rowNr])
            mdl->setValue(var, runningPrograms.execPtr[rowNr]->isHalted, rowNr);
        }
        return true;
      default: return false;
    }});
    ui->initCheckBox(tableVar, "exe", UINT8_MAX, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onSetValue:
        for (size_t rowNr = 0; rowNr < _MAX_PROG_AT_ONCE; rowNr++) {
          if (runningPrograms.execPtr[rowNr])
            mdl->setValue(var, runningPrograms.execPtr[rowNr]->exeExist, rowNr);
        }
        return true;
      default: return false;
    }});
    ui->initNumber(tableVar, "handle", UINT16_MAX, 0, UINT16_MAX, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onSetValue:
        for (size_t rowNr = 0; rowNr < _MAX_PROG_AT_ONCE; rowNr++) {
          if (runningPrograms.execPtr[rowNr])
            mdl->setValue(var, runningPrograms.execPtr[rowNr]->__run_handle_index, rowNr);
        }
        return true;
      default: return false;
    }});
    ui->initButton(tableVar, "Kill", false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
        if (runningPrograms.execPtr[rowNr])
          kill(runningPrograms.execPtr[rowNr]->name.c_str());
        return true;
      default: return false;
    }});

    addExternals();

    runningPrograms.setPrekill(preKill, postKill); //for clockless driver...
    runningPrograms.setFunctionToSync(show);

  } //setup

  void UserModLive::addExternals() {
    //Live Scripts defaults
    addExternalFun("void", "show", "()", (void *)&show); //comment if setup/loop model works
    // addExternalFun("void", "showM", "()", (void *)&UserModLive::showM); // warning: converting from 'void (UserModLive::*)()' to 'void*' [-Wpmf-conversions]
    addExternalFun("void", "resetStat", "()", (void *)&resetShowStats);

    addExternalFun("float", "atan2", "(float a1, float a2)",(void*)_atan2);
    addExternalFun("float", "hypot", "(float a1, float a2)",(void*)_hypot);
    addExternalFun("float", "sin", "(float a1)", (void *)_sin);
    addExternalFun("float", "cos", "(float a1)", (void *)_cos);
    addExternalFun("float", "time", "(float a1)", (void *)_time);
    addExternalFun("float", "triangle", "(float a1)", (void *)_triangle);
    addExternalFun("uint32_t", "millis", "()", (void *)millis);

    // addExternalFun("delay", [](int ms) {delay(ms);});
    // addExternalFun("digitalWrite", [](int pin, int val) {digitalWrite(pin, val);});

  }

  void UserModLive::addExternalVal(string result, string name, void * ptr) {
    if (findLink(name, externalType::value) == -1) //allready added earlier
      addExternal(name, externalType::value, ptr);
    scPreBaseScript += "external " + result + " " + name + ";\n";
  }

  void UserModLive::addExternalFun(string result, string name, string parameters, void * ptr) {
    if (findLink(name, externalType::function) == -1) //allready added earlier
      addExternal(name, externalType::function, ptr);
    scPreBaseScript += "external " + result + " " + name + parameters + ";\n";
  }

  // void UserModLive::addExternalFun(string name, std::function<void(int)> fun) {
  //   addExternal(name, externalType::function, (void *)&fun)); //unfortionately InstructionFetchError, why does it work in initText etc?
  //   ppf("external %s(int arg1);\n", name.c_str()); //add to string
  // }
  // void UserModLive::addExternalFun(string name, std::function<void(int, int)> fun) {
  //   addExternal(name, externalType::function, (void *)&fun); //unfortionately InstructionFetchError
  //   ppf("external %s(int arg1, int arg2);\n", name.c_str()); //add to string
  // }

  //testing class functions instead of static
  void UserModLive::showM() {
    long time2 = ESP.getCycleCount();
    // driver.showPixels(WAIT);
    frameCounter++;

    float k = (float)(time2 - time1) / 240000000; //always 240MHz?
    fps = 1 / k;
    time1 = ESP.getCycleCount();
  }

  void UserModLive::loop() {
    if (loopState == 2) {// show has been called (in other loop)
      loopState = 0; //waiting on Live Script
      // ppf("loopState %d\n", loopState);
    }
    else if (loopState == 1) {
      loopState = 2; //other loop can call show (or preview)
      // ppf("loopState %d\n", loopState);
    }
  }

  void UserModLive::loop20ms() {
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
  }

  void UserModLive::loop1s() {
    for (JsonObject childVar: Variable(mdl->findVar("LiveScripts", "scripts")).children())
      ui->callVarFun(childVar, UINT8_MAX, onSetValue); //set the value (WIP)
  }

  void UserModLive::run(const char *fileName, const char * main, const char * post) {
    ppf("live run n:%s o:%s (f:%d)\n", fileName, this->fileName);

    kill(); //kill any old script

    File f = files->open(fileName, "r");
    if (!f)
      ppf("UserModLive setup script open %s for %s failed\n", fileName, "r");
    else {

      string scScript = scPreBaseScript;

      unsigned preScriptNrOfLines = 0;

      for (size_t i = 0; i < scScript.length(); i++)
      {
        if (scScript[i] == '\n')
          preScriptNrOfLines++;
      }

      ppf("preScript of %s has %d lines\n", fileName, preScriptNrOfLines+1); //+1 to subtract the line from parser error line reported

      scScript += string(f.readString().c_str()); // add sc file

      if (post) scScript += post;

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

      ppf("Before parsing of %s\n", fileName);
      ppf("%s:%d f:%d / t:%d (l:%d) B [%d %d]\n", __FUNCTION__, __LINE__, ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap(), esp_get_free_heap_size(), esp_get_free_internal_heap_size());

      Executable *executable = new Executable();
      Parser parser = Parser();

      *executable = parser.parseScript(&scScript);
      executable->name = string(fileName);

      if (executable->exeExist)
      {
        ppf("parsing %s done\n", fileName);
        ppf("%s:%d f:%d / t:%d (l:%d) B [%d %d]\n", __FUNCTION__, __LINE__, ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap(), esp_get_free_heap_size(), esp_get_free_internal_heap_size());

        executable->executeAsTask(main);
        // ppf("setup done\n");
        // strlcpy(this->fileName, fileName, sizeof(this->fileName));
      }
      f.close();
    }
  }

  void UserModLive::kill(const char * fileName) {
    //tbd: kill specific task...
    for (size_t rowNr = 0; rowNr < _MAX_PROG_AT_ONCE; rowNr++) {
      if (runningPrograms.execPtr[rowNr] && runningPrograms.execPtr[rowNr]->isRunning()) {
        //if fileName kill only process with this name
        if (!fileName || !runningPrograms.execPtr[rowNr]->name.c_str() || strncmp(runningPrograms.execPtr[rowNr]->name.c_str(), fileName, 32) == 0) {
          ppf("kill %s\n", runningPrograms.execPtr[rowNr]->name.c_str());
          runningPrograms.execPtr[rowNr]->_kill();
          fps = 0;
          strlcpy(this->fileName, "", sizeof(this->fileName));
          delete runningPrograms.execPtr[rowNr]; //delete the Execution created in Run
        }
      }
    }
  }