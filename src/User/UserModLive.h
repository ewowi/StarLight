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
#include "parser.h"

long time1;
static float _min = 9999;
static float _max = 0;
static uint32_t _nb_stat = 0;
static float _totfps;

static float fps = 0; //integer?
static unsigned long frameCounter = 0;

static void show()
{
    // SKIPPED: check nargs (must be 3 because arg[0] is self)
    long time2 = ESP.getCycleCount();
    // driver.showPixels(WAIT);
    frameCounter++;

    float k = (float)(time2 - time1) / 240000000; //always 240MHz?
    fps = 1 / k;
    _nb_stat++;
    if (_min > fps && fps > 10 && _nb_stat > 10)
        _min = fps;
    if (_max < fps && fps < 200 && _nb_stat > 10)
        _max = fps;
    if (_nb_stat > 10)
        _totfps += fps;
    if (_nb_stat%10000 == 0)
      ppf("current fps:%.2f  average:%.2f min:%.2f max:%.2f\r\n", fps, _totfps / (_nb_stat - 10), _min, _max);
    time1 = ESP.getCycleCount();

    // SKIPPED: check that both v1 and v2 are int numbers
    // RETURN_VALUE(VALUE_FROM_INT(0), rindex);
}

class UserModLive:public SysModule {

public:

  Parser p = Parser();

  UserModLive() :SysModule("Live") {
    isEnabled = false; //need to enable after fresh setup
  };

  void setup() override {
    SysModule::setup();

    parentVar = ui->initUserMod(parentVar, name, 6310);

    ui->initSelect(parentVar, "script", UINT16_MAX, false ,[this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI: {
        // ui->setComment(var, "Fixture to display effect on");
        JsonArray options = ui->setOptions(var);
        options.add("None");
        files->dirToJson(options, true, ".sc"); //only files containing F(ixture), alphabetically

        return true; }
      case onChange: {
        //set script
        uint8_t fileNr = var["value"];

        SCExecutable._kill(); //kill any old tasks
        fps = 0;

        ppf("%s script f:%d f:%d\n", name, funType, fileNr);

        if (fileNr > 0) { //not None

          fileNr--;  //-1 as none is no file

          char fileName[32] = "";

          files->seqNrToName(fileName, fileNr, ".sc");

          // ppf("%s script f:%d f:%d\n", name, funType, fileNr);

          if (strcmp(fileName, "") != 0) {

            File f = files->open(fileName, "r");
            if (!f)
              ppf("UserModLive setup script  open %s for %s failed", fileName, "r");
            else {

              string script = string(f.readString().c_str());

              ppf("%s\n", script);

              if (p.parse_c(&script))
              {
                  SCExecutable.executeAsTask("main");
              }
              f.close();
            }
          }
          else
            ppf("UserModLive setup file for %d not found", fileNr);
        }
        return true; }
      default: return false; 
    }}); //script

    ui->initText(parentVar, "fps1", nullptr, 10, true);
    ui->initText(parentVar, "fps2", nullptr, 10, true);

    // ui->initButton

    addExternal("show", externalType::function, (void *)&show);
    addExternal("showM", externalType::function, (void *)&UserModLive::showM); // warning: converting from 'void (UserModLive::*)()' to 'void*' [-Wpmf-conversions]

  }

  //testing class functions instead of static
  void showM() {
    long time2 = ESP.getCycleCount();
    // driver.showPixels(WAIT);
    frameCounter++;

    float k = (float)(time2 - time1) / 240000000; //always 240MHz?
    fps = 1 / k;
    time1 = ESP.getCycleCount();
  }

  void loop1s() {
    mdl->setUIValueV("fps1", "%.0f /s", fps);
    mdl->setUIValueV("fps2", "%d /s", frameCounter);
    frameCounter = 0;
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