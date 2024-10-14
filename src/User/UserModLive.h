/*
   @title     StarBase
   @file      UserModLive.h
   @date      20240819
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors, asmParser © https://github.com/hpwit/ASMParser
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/


// #define __RUN_CORE 0
#pragma once
#include "ESPLiveScript.h"

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
      float myVal = sys->now;
      myVal = myVal / 65535 / j;           // PixelBlaze uses 1000/65535 = .015259. 
      myVal = fmod(myVal, 1.0);               // ewowi: with 0.015 as input, you get fmod(millis/1000,1.0), which has a period of 1 second, sounds right
      return myVal;
}
// static millis()

uint8_t slider1 = 128;
uint8_t slider2 = 128;
uint8_t slider3 = 128;

//LEDS specific
static CRGB POSV(uint8_t h, uint8_t s, uint8_t v) {return CHSV(h, s, v);} //why call POSV and not hsv?
static CRGB rgb(uint8_t r, uint8_t g, uint8_t b) {return CRGB(r, g, b);} //why call POSV and not hsv?
static uint8_t _sin8(uint8_t theta) {return sin8(theta);}
static uint8_t _cos8(uint8_t theta) {return cos8(theta);}
static uint8_t _beatSin8(uint8_t bpm, uint8_t lowest, uint8_t highest) {return beatsin8(bpm, lowest, highest);}
static uint8_t _inoise8(uint16_t x, uint16_t y, uint16_t z) {return inoise8(x, y, z);}
static uint8_t _random8() {return random8();}
static LedsLayer *gLeds = nullptr;
static void _fadeToBlackBy(uint8_t fadeBy) {if (gLeds) gLeds->fadeToBlackBy(fadeBy);}

static void _addPixelsPre() {if (gLeds) gLeds->fixture->projectAndMapPre();}
static void _addPixel(uint8_t a1, uint8_t a2, uint8_t a3) {if (gLeds) gLeds->fixture->projectAndMapPixel({a1, a2, a3});}
static void _addPin(uint8_t a1) {if (gLeds) gLeds->fixture->projectAndMapPin(a1);}
static void _addPixelsPost() {if (gLeds) gLeds->fixture->projectAndMapPost();}
static void sPCLive(uint16_t pixel, CRGB color) {if (gLeds) gLeds->setPixelColor(pixel, color);} //setPixelColor with color
static void sCFPLive(uint16_t pixel, uint8_t index, uint8_t brightness) {if (gLeds) gLeds->setPixelColor(pixel, ColorFromPalette(gLeds->palette, index, brightness));} //setPixelColor within palette
//End LEDS specific

class UserModLive:public SysModule {

public:

  Parser p = Parser();
  Executable myexec;
  char fileName[32] = ""; //running sc file
  string scPreBaseScript = ""; //externals etc generated (would prefer String for esp32...)

  UserModLive() :SysModule("LiveScripts") {
    isEnabled = false; //need to enable after fresh setup
  };

  void setup() override {
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

        if (fileNr > 0) { //not None and setup done
          fileNr--;  //-1 as none is no file
          files->seqNrToName(web->lastFileUpdated, fileNr, ".sc");
          ppf("script.onChange f:%d n:%s\n", fileNr, web->lastFileUpdated);
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


    //Live Scripts defaults
    addExternalFun("void", "show", "()", (void *)&show); //comment if setup/loop model works
    // addExternalFun("void", "showM", "()", (void *)&UserModLive::showM); // warning: converting from 'void (UserModLive::*)()' to 'void*' [-Wpmf-conversions]
    addExternalFun("void", "resetStat", "()", (void *)&resetShowStats);

    addExternalFun("void", "display", "(int a1)", (void *)&dispshit);
    addExternalFun("void", "dp", "(float a1)", (void *)displayfloat);
    addExternalFun("void", "error", "(int a1, int a2, int a3)", (void *)&showError);
    addExternalFun("void", "print", "(char * a1)", (void *)__print);

    addExternalFun("float", "atan2", "(float a1, float a2)",(void*)_atan2);
    addExternalFun("float", "hypot", "(float a1, float a2)",(void*)_hypot);
    addExternalFun("float", "sin", "(float a1)", (void *)_sin);
    addExternalFun("float", "time", "(float a1)", (void *)_time);
    addExternalFun("float", "triangle", "(float a1)", (void *)_triangle);
    addExternalFun("uint32_t", "millis", "()", (void *)millis);

    // added by StarBase
    addExternalFun("void", "pinMode", "(int a1, int a2)", (void *)&pinMode);
    addExternalFun("void", "digitalWrite", "(int a1, int a2)", (void *)&digitalWrite);
    addExternalFun("void", "delay", "(int a1)", (void *)&delay);

    // addExternalFun("delay", [](int ms) {delay(ms);});
    // addExternalFun("digitalWrite", [](int pin, int val) {digitalWrite(pin, val);});

    addExternalVal("uint8_t", "slider1", &slider1); //used in map function
    addExternalVal("uint8_t", "slider2", &slider2); //used in map function
    addExternalVal("uint8_t", "slider3", &slider3); //used in map function

    //LEDS specific
    //For Live Effects
    addExternalFun("CRGB", "hsv", "(int a1, int a2, int a3)", (void *)POSV); //uint8_t?
    addExternalFun("CRGB", "rgb", "(int a1, int a2, int a3)", (void *)rgb);//uint8_t?
    addExternalFun("uint8_t", "beatSin8", "(uint8_t a1, uint8_t a2, uint8_t a3)", (void *)_beatSin8);
    addExternalFun("uint8_t", "inoise8", "(uint16_t a1, uint16_t a2, uint16_t a3)", (void *)_inoise8);
    addExternalFun("uint8_t", "random8", "()", (void *)_random8);
    addExternalFun("uint8_t", "sin8", "(uint8_t a1)",(void*)_sin8); //using int here causes value must be between 0 and 16 error!!!
    addExternalFun("uint8_t", "cos8", "(uint8_t a1)",(void*)_cos8); //using int here causes value must be between 0 and 16 error!!!
    addExternalFun("void", "sPC", "(uint16_t a1, CRGB a2)", (void *)sPCLive);
    addExternalFun("void", "sCFP", "(uint16_t a1, uint8_t a2, uint8_t a3)", (void *)sCFPLive);
    addExternalFun("void", "fadeToBlackBy", "(uint8_t a1)", (void *)_fadeToBlackBy);
    //address of overloaded function with no contextual type information: setPixelColorLive
    //ISO C++ forbids taking the address of a bound member function to form a pointer to member function.  Say '&LedsLayer::setPixelColorLive' [-fpermissive]
    //converting from 'void (LedsLayer::*)(uint16_t, uint32_t)' {aka 'void (LedsLayer::*)(short unsigned int, unsigned int)'} to 'void*' [-Wpmf-conversions]

    addExternalFun("void", "addPixelsPre", "()", (void *)_addPixelsPre);
    addExternalFun("void", "addPixel", "(uint16_t a1, uint16_t a2, uint16_t a3)", (void *)_addPixel);
    addExternalFun("void", "addPin", "(uint16_t a1)", (void *)_addPin);
    addExternalFun("void", "addPixelsPost", "()", (void *)_addPixelsPost);

    //End LEDS specific

    // runningPrograms.setPrekill(pre, post); //for clockless driver...
    runningPrograms.setFunctionToSync(show);

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
    if (myexec.isRunning()) {
      if (loopState == 2) {// show has been called (in other loop)
        loopState = 0; //waiting on Live Script
        // ppf("loopState %d\n", loopState);
      }
      else if (loopState == 1) {
        loopState = 2; //other loop can call show (or preview)
        // ppf("loopState %d\n", loopState);
      }
    }
  }

  void loop20ms() {
    //workaround
    if (strnstr(web->lastFileUpdated, ".sc", sizeof(web->lastFileUpdated)) != nullptr) {
      if (strnstr(web->lastFileUpdated, "del:/", sizeof(web->lastFileUpdated)) != nullptr) {
        if (strncmp(this->fileName, web->lastFileUpdated+4, sizeof(this->fileName)) == 0) { //+4 remove del:
          ppf("loop20ms kill %s\n", web->lastFileUpdated);
          kill();
          // ui->callVarFun("effect", "script", UINT8_MAX, onUI); //LEDs specific. rebuild options LEDs specific
        }
        //else nothing
      }
      else {
        ppf("loop20ms run %s -> %s\n", this->fileName, web->lastFileUpdated);
        run(web->lastFileUpdated);
        // ui->callVarFun("effect, "script", UINT8_MAX, onUI); //LEDs specific. rebuild options
      }
      strlcpy(web->lastFileUpdated, "", sizeof(web->lastFileUpdated));
    }
  }

  void loop1s() {
    for (JsonObject childVar: Variable(mdl->findVar("LiveScripts", "scripts")).children())
      ui->callVarFun(childVar, UINT8_MAX, onSetValue); //set the value (WIP)
  }

  void run(const char *fileName) {
    ppf("live run n:%s o:%s (f:%d)\n", fileName, this->fileName);

    kill(); //kill any old script

    if (fileName && strnlen(fileName, 32) > 0) {

      File f = files->open(fileName, "r");
      if (!f)
        ppf("UserModLive setup script open %s for %s failed\n", fileName, "r");
      else {

        string scScript = scPreBaseScript;

        //LEDs specific
        if (gLeds != nullptr) {
          scScript += "define width " + to_string(gLeds->size.x) + "\n";
          scScript += "define height " + to_string(gLeds->size.y) + "\n";
          scScript += "define NUM_LEDS " + to_string(gLeds->nrOfLeds) + "\n";
          scScript += "define panel_width " + to_string(gLeds->size.x) + "\n"; //isn't panel_width always the same as width?
        }
        //end LEDs specific

        Serial.println(scScript.c_str()); //ppf has a max

        unsigned preScriptNrOfLines = 0;

        for (size_t i = 0; i < scScript.length(); i++)
        {
          if (scScript[i] == '\n')
            preScriptNrOfLines++;
        }

        ppf("preScript of %s has %d lines\n", fileName, preScriptNrOfLines+1); //+1 to subtract the line from parser error line reported

        scScript += string(f.readString().c_str()); // add sc file

        scScript += "void main(){resetStat();setup();while(2>1){loop();show();}}"; //add main which calls setup and loop

        ppf("Before parsing of %s\n", fileName);
        ppf("%s:%d f:%d / t:%d (l:%d) B [%d %d]\n", __FUNCTION__, __LINE__, ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap(), esp_get_free_heap_size(), esp_get_free_internal_heap_size());

        myexec = p.parseScript(&scScript);
        myexec.name = string(fileName);

        if (myexec.exeExist)
        {
          ppf("parsing %s done\n", fileName);
          ppf("%s:%d f:%d / t:%d (l:%d) B [%d %d]\n", __FUNCTION__, __LINE__, ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap(), esp_get_free_heap_size(), esp_get_free_internal_heap_size());

          myexec.executeAsTask("main");
          // ppf("setup done\n");
          strlcpy(this->fileName, fileName, sizeof(this->fileName));
        }
        f.close();
      }
    }
    else
      ppf("UserModLive setup file for %s not found\n", fileName);
  }

  void kill() {
    if (myexec.isRunning()) {
      ppf("kill %s\n", fileName);
      myexec._kill();
      fps = 0;
      strlcpy(fileName, "", sizeof(fileName));
      if (gLeds) gLeds->fadeToBlackBy(255); // LEDs specific
    }
  }

};

extern UserModLive *liveM;


/*
Pre script:

external void show();
external void resetStat();
external void display(int a1);
external void dp(float a1);
external void error(int a1, int a2, int a3);
external void print(char * a1);
external float atan2(float a1, float a2);
external float hypot(float a1, float a2);
external float sin(float a1);
external float time(float a1);
external float triangle(float a1);
external uint32_t millis();
external void pinMode(int a1, int a2);
external void digitalWrite(int a1, int a2);
external void delay(int a1);
external uint8_t slider1;
external uint8_t slider2;
external uint8_t slider3;
external CRGB hsv(int a1, int a2, int a3);
external CRGB rgb(int a1, int a2, int a3);
external uint8_t beatSin8(uint8_t a1, uint8_t a2, uint8_t a3);
external uint8_t inoise8(uint16_t a1, uint16_t a2, uint16_t a3);
external uint8_t random8();
external uint8_t sin8(uint8_t a1);
external uint8_t cos8(uint8_t a1);
external void sPC(uint16_t a1, CRGB a2);
external void sCFP(uint16_t a1, uint8_t a2, uint8_t a3);
external void fadeToBlackBy(uint8_t a1);
define width 32
define height 32
define NUM_LEDS 1024
define panel_width 32
*/