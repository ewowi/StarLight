/*
   @title     StarBase
   @file      UserModLive.h
   @date      20240411
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors, asmParser © https://github.com/hpwit/ASMParser
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

// #define __RUN_CORE 0
#pragma once
#include <parser.h>

long time1;
long time4;
static float _min = 9999;
static float _max = 0;
static uint32_t _nb_stat = 0;
static float _totfps;

static float fps = 0; //integer?
static unsigned long frameCounter = 0;

//external function implementation (tbd: move into class)

static void show()
{
  frameCounter++;
    
  // SKIPPED: check nargs (must be 3 because arg[0] is self)
  long time2 = ESP.getCycleCount();

  // driver.showPixels(WAIT);
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
  if (_nb_stat%1000 == 0)
    //Serial.printf("current show fps:%.2f\tglobal fps:%.2f\tfps animation:%.2f\taverage:%.2f\tmin:%.2f\tmax:%.2f\r\n", fps2, fps3, fps, _totfps / (_nb_stat - 10), _min, _max);
    ppf("current show fps:%.2f\tglobal fps:%.2f\tfps animation:%.2f  average:%.2f min:%.2f max:%.2f\r\n",fps2, fps3,  fps, _totfps / (_nb_stat - 10), _min, _max);
  time1 = ESP.getCycleCount();
  time4 = time2;

  // SKIPPED: check that both v1 and v2 are int numbers
  // RETURN_VALUE(VALUE_FROM_INT(0), rindex);
}

static void resetShowStats()
{
    float min = 999;
    float max = 0;
    _nb_stat = 0;
    _totfps = 0;
}

static void dispshit(int g) { ppf("coming from assembly int %x %d", g, g);}
static void __print(char *s) {ppf("from assembly :%s\r\n", s);}
static void showError(int line, uint32_t size, uint32_t got) { ppf("Overflow error line %d max size: %d got %d", line, size, got);}
static void displayfloat(float j) {ppf("display float %f", j);}

static float _hypot(float x,float y) {return hypot(x,y);}
static float _atan2(float x,float y) { return atan2(x,y);}
static float _sin(float j) {return sin(j);}

class UserModLive:public SysModule {

public:

  Parser p = Parser();
  char fileName[32] = ""; //running sc file
  string scPreScript = ""; //externals etc generated (would prefer String for esp32...)

  UserModLive() :SysModule("Live") {
    isEnabled = false; //need to enable after fresh setup
  };

  void setup() override {
    SysModule::setup();

    parentVar = ui->initUserMod(parentVar, name, 6310);

    ui->initSelect(parentVar, "script", UINT16_MAX, false , [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI: {
        // ui->setComment(var, "Fixture to display effect on");
        JsonArray options = ui->setOptions(var);
        options.add("None");
        files->dirToJson(options, true, ".sc"); //only files containing F(ixture), alphabetically

        return true; }
      case onChange: {
        //set script
        uint8_t fileNr = var["value"];

        ppf("%s script f:%d f:%d\n", name, funType, fileNr);

        char fileName[32] = "";

        if (fileNr > 0) { //not None
          fileNr--;  //-1 as none is no file
          files->seqNrToName(fileName, fileNr, ".sc");
          // ppf("%s script f:%d f:%d\n", name, funType, fileNr);
        }

        if (strcmp(fileName, "") != 0)
          run(fileName, true); //force a new file to run
        else {
          SCExecutable._kill(); //kill any old tasks
          fps = 0;
        }

        return true; }
      default: return false; 
    }}); //script

    ui->initText(parentVar, "fps1", nullptr, 10, true);
    ui->initText(parentVar, "fps2", nullptr, 10, true);

    // ui->initButton

    //Live scripts defaults
    addExternalFun("show", "()", (void *)&show);
    addExternalFun("showM", "()", (void *)&UserModLive::showM); // warning: converting from 'void (UserModLive::*)()' to 'void*' [-Wpmf-conversions]
    addExternalFun("resetStat", "()", (void *)&resetShowStats);

    addExternalFun("display", "(int a1)", (void *)&dispshit);
    addExternalFun("dp", "(float a1)", (void *)displayfloat);
    addExternalFun("error", "(int a1, int a2, int a3)", (void *)&showError);
    addExternalFun("print", "(char * a1)", (void *)__print);

    addExternalFun("atan2","(float a1, float a2)",(void*)_atan2);
    addExternalFun("hypot","(float a1, float a2)",(void*)_hypot);
    addExternalFun("sin", "(float a1)", (void *)_sin);

    //added by StarBase
    addExternalFun("pinMode", "(int a1, int a2)", (void *)&pinMode);
    addExternalFun("digitalWrite", "(int a1, int a2)", (void *)&digitalWrite);
    addExternalFun("delay", "(int a1)", (void *)&delay);

    // addExternalFun("delay", [](int ms) {delay(ms);});
    // addExternalFun("digitalWrite", [](int pin, int val) {digitalWrite(pin, val);});
  }

  void addExternalFun(string name, string parameters,void * ptr) {
    addExternal(name, externalType::function, ptr);
    scPreScript += "external void " + name + parameters + ";\n";
  }

  // void addExternalFun(string name, std::function<void(int)> fun) {
  //   addExternal(name, externalType::function, (void *)&fun)); //unfortionately InstructionFetchError, why does it work in initText etc?
  //   ppf("external %s(int arg1);\n", name.c_str()); //add to string
  // }
  // void addExternalFun(string name, std::function<void(int, int)> fun) {
  //   addExternal(name, externalType::function, (void *)&fun); //unfortionately InstructionFetchError
  //   ppf("external %s(int arg1, int arg2);\n", name.c_str()); //add to string
  // }

  //testing class functions instead of static
  void showM() {
    long time2 = ESP.getCycleCount();
    // driver.showPixels(WAIT);
    frameCounter++;

    float k = (float)(time2 - time1) / 240000000; //always 240MHz?
    fps = 1 / k;
    time1 = ESP.getCycleCount();
  }

  void loop20ms() {
    //workaround
    if (strstr(web->lastFileUpdated, ".sc") != nullptr) {
      run(web->lastFileUpdated);
      strcpy(web->lastFileUpdated, "");
    }
  }

  void loop1s() {
    mdl->setUIValueV("fps1", "%.0f /s", fps);
    mdl->setUIValueV("fps2", "%d /s", frameCounter);
    frameCounter = 0;
  }

  void run(const char *fileName, bool force = false) {
    ppf("live run n:%s o:%s (f:%d)\n", fileName, this->fileName, force);

    if (!force && strcmp(fileName, this->fileName) != 0) // if another fileName then force should be true;
      return;

    kill();

    if (strcmp(fileName, "") != 0) {

      File f = files->open(fileName, "r");
      if (!f)
        ppf("UserModLive setup script open %s for %s failed\n", fileName, "r");
      else {

        string scScript = scPreScript + string(f.readString().c_str());

        ppf("%s\n", scScript.c_str());

        if (p.parse_c(&scScript))
        {
          SCExecutable.executeAsTask("main");
          strcpy(this->fileName, fileName);
        }
        f.close();
      }
    }
    else
      ppf("UserModLive setup file for %s not found\n", fileName);
  }

  void kill() {
    SCExecutable._kill(); //kill any old tasks
    fps = 0;
    strcpy(fileName, "");
  }

};

extern UserModLive *liveM;

//warnings
//.pio/libdeps/esp32dev/asmParser/src/asm_struct_enum.h:93:1: warning: 'typedef' was ignored in this declaration
//.pio/libdeps/esp32dev/asmParser/src/asm_parser.h:1612:45: warning: 'void heap_caps_aligned_free(void*)' is deprecated [-Wdeprecated-declarations]
//asm_parser.h:325:1: warning: control reaches end of non-void function 

//crash reports
// E (31053) task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time:
// E (31053) task_wdt:  - IDLE (CPU 0)
// E (31053) task_wdt: Tasks currently running:
// E (31053) task_wdt: CPU 0: _run_task
// E (31053) task_wdt: CPU 1: loopTask
// E (31053) task_wdt: Aborting.