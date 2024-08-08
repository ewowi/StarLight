/*
   @title     StarBase
   @file      UserModLive.h
   @date      20240720
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors, asmParser © https://github.com/hpwit/ASMParser
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/


// #define __RUN_CORE 0
#pragma once
#include "newParser.h"

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
  if (_nb_stat%1000 == 0)
    //Serial.printf("current show fps:%.2f\tglobal fps:%.2f\tfps animation:%.2f\taverage:%.2f\tmin:%.2f\tmax:%.2f\r\n", fps2, fps3, fps, _totfps / (_nb_stat - 10), _min, _max);
    ppf("current show fps:%.2f\tglobal fps:%.2f\tfps animation:%.2f  average:%.2f min:%.2f max:%.2f\r\n",fps2, fps3,  fps, _totfps / (_nb_stat - 10), _min, _max);
  time1 = ESP.getCycleCount();
  time4 = time2;

  // SKIPPED: check that both v1 and v2 are int numbers
  // RETURN_VALUE(VALUE_FROM_INT(0), rindex);
  delay(1); //to feed the watchdog
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
static void displayfloat(float j) {ppf(" %f", j);}

static float _hypot(float x,float y) {return hypot(x,y);}
static float _atan2(float x,float y) { return atan2(x,y);}
static float _sin(float j) {return sin(j);}
static float _triangle(float j) {return 1.0 - fabs(fmod(2 * j, 2.0) - 1.0);}
static float _time(float j) {
      float myVal = sys->now();
      myVal = myVal / 65535 / j;           // PixelBlaze uses 1000/65535 = .015259. 
      myVal = fmod(myVal, 1.0);               // ewowi: with 0.015 as input, you get fmod(millis/1000,1.0), which has a period of 1 second, sounds right
      return myVal;
}

//LEDS specific
static CRGB POSV(uint8_t h, uint8_t s, uint8_t v) {return CHSV(h, s, v);} //why call POSV and not hsv?
static uint8_t _sin8(uint8_t a) {return sin8(a);}
static float _beatSin8(uint8_t a1, uint8_t a2, uint8_t a3) {return beatsin8(a1, a2, a3);}
static LedsLayer *gLeds = nullptr;
static void _fadeToBlackBy(uint8_t a1) {gLeds->fadeToBlackBy(a1);}
static void sPCLive(int t, uint16_t pixel, CRGB color) { // int t needed - otherwise wrong colors, very strange
  if (gLeds) 
  {
    // if (t == 0) ppf(" %d,%d,%d", color.r, color.g, color.b);
    gLeds->setPixelColor(pixel, color);
  }
}
static void sCFPLive(uint16_t pixel, uint8_t index, uint8_t brightness) { // int t needed - otherwise wrong colors, very strange
  if (gLeds) 
  {
    // if (t == 0) ppf(" %d,%d,%d", color.r, color.g, color.b);
    gLeds->setPixelColor(pixel, ColorFromPalette(gLeds->palette, index, brightness));
  }
}
uint8_t slider1 = 128;
uint8_t slider2 = 128;

//End LEDS specific

class UserModLive:public SysModule {

public:

  Parser p = Parser();
  char fileName[32] = ""; //running sc file
  string scPreBaseScript = ""; //externals etc generated (would prefer String for esp32...)
  string scPreCustomScript = ""; //externals etc generated (would prefer String for esp32...)
  // bool setupDone = false;

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

        if (fileNr > 0 && scPreBaseScript.length()) { //not None and setup done
          fileNr--;  //-1 as none is no file
          files->seqNrToName(fileName, fileNr, ".sc");
          // ppf("%s script f:%d f:%d\n", name, funType, fileNr);
        }

        if (strcmp(fileName, "") != 0)
          run(fileName, true); //force a new file to run
        else {
          kill();
        }

        return true; }
      default: return false; 
    }}); //script

    ui->initText(parentVar, "fps1", nullptr, 10, true);
    ui->initText(parentVar, "fps2", nullptr, 10, true);

    //Live scripts defaults
    addExternalFun("void", "show", "()", (void *)&show); //comment if setup/loop model works
    // addExternalFun("void", "showM", "()", (void *)&UserModLive::showM); // warning: converting from 'void (UserModLive::*)()' to 'void*' [-Wpmf-conversions]
    addExternalFun("void", "resetStat", "()", (void *)&resetShowStats);

    addExternalFun("void", "display", "(int a1)", (void *)&dispshit);
    addExternalFun("void", "dp", "(float a1)", (void *)displayfloat);
    addExternalFun("void", "error", "(int a1, int a2, int a3)", (void *)&showError);
    addExternalFun("void", "print", "(char * a1)", (void *)__print);

    addExternalFun("float", "atan2","(float a1, float a2)",(void*)_atan2);
    addExternalFun("float", "hypot","(float a1, float a2)",(void*)_hypot);
    addExternalFun("float", "sin", "(float a1)", (void *)_sin);
    addExternalFun("float", "time", "(float a1)", (void *)_time);
    addExternalFun("float", "triangle", "(float a1)", (void *)_triangle);
    addExternalFun("uint8_t", "beatSin8", "(uint8_t a1, uint8_t a2, uint8_t a3)", (void *)_beatSin8);

    // added by StarBase
    addExternalFun("void", "pinMode", "(int a1, int a2)", (void *)&pinMode);
    addExternalFun("void", "digitalWrite", "(int a1, int a2)", (void *)&digitalWrite);
    addExternalFun("void", "delay", "(int a1)", (void *)&delay);

    // addExternalFun("delay", [](int ms) {delay(ms);});
    // addExternalFun("digitalWrite", [](int pin, int val) {digitalWrite(pin, val);});

    //LEDS specific
    addExternalFun("CRGB", "hsv", "(int a1, int a2, int a3)", (void *)POSV);
    addExternalFun("uint8_t", "sin8","(uint8_t a1)",(void*)_sin8); //using int here causes value must be between 0 and 16 error!!!
    addExternalFun("void", "sPC", "(int a0, int a1, CRGB a2)", (void *)sPCLive); // int t needed - otherwise wrong colors, very strange
    addExternalFun("void", "sCFP", "(uint16_t pixel, uint8_t index, uint8_t brightness)", (void *)sCFPLive);
    addExternalFun("void", "fadeToBlackBy", "(uint8_t a1)", (void *)_fadeToBlackBy);
    //address of overloaded function with no contextual type information: setPixelColorLive
    //ISO C++ forbids taking the address of a bound member function to form a pointer to member function.  Say '&LedsLayer::setPixelColorLive' [-fpermissive]
    //converting from 'void (LedsLayer::*)(uint16_t, uint32_t)' {aka 'void (LedsLayer::*)(short unsigned int, unsigned int)'} to 'void*' [-Wpmf-conversions]
    addExternalVal("uint8_t", "slider1", &slider1); //used in map function
    addExternalVal("uint8_t", "slider2", &slider2); //used in map function

    //End LEDS specific

  } //setup

  void addExternalVal(string result, string name, void * ptr) {
    addExternal(name, externalType::value, ptr);
    scPreBaseScript += "external " + result + " " + name + ";\n";
  }

  void addExternalFun(string result, string name, string parameters, void * ptr) {
    addExternal(name, externalType::function, ptr);
    scPreBaseScript += "external " + result + " " + name + parameters + ";\n";
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

  void loop() {
    // this will result in: Stopping the program...
    // if (setupDone) {
    //   SCExecutable.executeAsTask("loop");
    //   ppf(".");
    //   show();
    // }
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

        string scScript = scPreBaseScript + scPreCustomScript;

        Serial.println(scScript.c_str());

        unsigned preScriptNrOfLines = 0;
        for(int i = 0; i < scScript.length(); i++)
        {
          if (scScript[i] == '\n') 
            preScriptNrOfLines++;
        }

        ppf("preScript has %d lines\n", preScriptNrOfLines);

        scScript += string(f.readString().c_str()); // add sc file

        ppf("Before parsing\n");
        ppf("%s:%d f:%d / t:%d (l:%d) B [%d %d]\n", __FUNCTION__, __LINE__, ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap(), esp_get_free_heap_size(), esp_get_free_internal_heap_size());

        if (p.parseScript(&scScript))
        {
          ppf("parsing done\n");
          ppf("%s:%d f:%d / t:%d (l:%d) B [%d %d]\n", __FUNCTION__, __LINE__, ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap(), esp_get_free_heap_size(), esp_get_free_internal_heap_size());

          SCExecutable.executeAsTask("main"); //"setup" not working
          // ppf("setup done\n");
          strcpy(this->fileName, fileName);
          // setupDone = true;
        }
        f.close();
      }
    }
    else
      ppf("UserModLive setup file for %s not found\n", fileName);
  }

  void kill() {
    ppf("kill %s\n", fileName);
    // setupDone = false;
    SCExecutable._kill(); //kill any old tasks
    fps = 0;
    strcpy(fileName, "");
  }

};

extern UserModLive *liveM;


//asm_parser.h:325:1: warning: control reaches end of non-void function 