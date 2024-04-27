/*
   @title     StarLeds
   @file      LedModFixture.h
   @date      20240228
   @repo      https://github.com/MoonModules/StarLeds
   @Authors   https://github.com/MoonModules/StarLeds/commits/main
   @Copyright © 2024 Github StarLeds Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

class LedModFixture:public SysModule {

public:

  uint8_t viewRotation = 0;

  LedModFixture() :SysModule("Fixture") {};

  void setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name, 1100);

    JsonObject currentVar = ui->initCheckBox(parentVar, "on", true, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "On");
        return true;
      case f_ChangeFun:
        ui->callVarFun("bri", UINT8_MAX, f_ChangeFun); //set FastLed brightness
        return true;
      default: return false;
    }});
    currentVar["dash"] = true;

    //logarithmic slider (10)
    currentVar = ui->initSlider(parentVar, "bri", 10, 0, 255, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "Brightness");
        return true;
      case f_ChangeFun: {
        stackUnsigned8 bri = var["value"];

        stackUnsigned8 result = mdl->getValue("on").as<bool>()?mdl->varLinearToLogarithm(var, bri):0;

        FastLED.setBrightness(result);

        ppf("Set Brightness to %d -> b:%d r:%d\n", var["value"].as<int>(), bri, result);
        return true; }
      default: return false; 
    }});
    currentVar["log"] = true; //logarithmic
    currentVar["dash"] = true; //these values override model.json???

    currentVar = ui->initCanvas(parentVar, "pview", UINT16_MAX, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "Preview");
        // ui->setComment(var, "Shows the fixture");
        // ui->setComment(var, "Click to enlarge");
        return true;
      case f_LoopFun: {
        var["interval"] =  max(eff->fixture.nrOfLeds * web->ws.count()/200, 16U)*10; //interval in ms * 10, not too fast //from cs to ms

        web->sendDataWs([this](AsyncWebSocketMessageBuffer * wsBuf) {
          byte* buffer;

          buffer = wsBuf->get();

          // send leds preview to clients
          for (size_t i = 0; i < eff->fixture.nrOfLeds; i++)
          {
            buffer[i*3+5] = eff->fixture.ledsP[i].red;
            buffer[i*3+5+1] = eff->fixture.ledsP[i].green;
            buffer[i*3+5+2] = eff->fixture.ledsP[i].blue;
          }
          //new values
          buffer[0] = 1; //userFun id
          //rotations
          if (viewRotation == 0) {
            buffer[1] = 0;
            buffer[2] = 0;
            buffer[3] = 0;
          } else if (viewRotation == 1) { //pan
            buffer[1] = 0;//beatsin8(4, 250, 5); //tilt
            buffer[2] = beat8(1);//, 0, 255); //pan
            buffer[3] = 0;//beatsin8(6, 255, 5); //roll
          } else if (viewRotation == 2) { //tilt
            buffer[1] = beat8(1);//, 0, 255); //pan
            buffer[2] = 0;//beatsin8(4, 250, 5); //tilt
            buffer[3] = 0;//beatsin8(6, 255, 5); //roll
          } else if (viewRotation == 3) { //roll
            buffer[1] = 0;//beatsin8(4, 250, 5); //tilt
            buffer[2] = 0;//beatsin8(6, 255, 5); //roll
            buffer[3] = beat8(1);//, 0, 255); //pan
          } else if (viewRotation == 4) {
            buffer[1] = eff->fixture.head.x;
            buffer[2] = eff->fixture.head.y;
            buffer[3] = eff->fixture.head.y;
          }

        }, eff->fixture.nrOfLeds * 3 + 5, true);
        return true;
      }
      default: return false;
    }});

    ui->initSelect(currentVar, "viewRot", viewRotation, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun: {
        ui->setLabel(var, "Rotation");
        // ui->setComment(var, "View rotation");
        JsonArray options = ui->setOptions(var);
        options.add("None");
        options.add("Pan");
        options.add("Tilt");
        options.add("Roll");
        #ifdef STARLEDS_USERMOD_WLEDAUDIO
          options.add("Moving heads GEQ");
        #endif
        return true; }
      case f_ChangeFun:
        this->viewRotation = var["value"];
        // if (!var["value"])
        //   eff->fixture.head = {0,0,0};
        return true;
      default: return false; 
    }});

    currentVar = ui->initSelect(parentVar, "fixture", eff->fixture.fixtureNr, false ,[](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun: {
        // ui->setComment(var, "Fixture to display effect on");
        JsonArray options = ui->setOptions(var);
        files->dirToJson(options, true, "F_"); //only files containing F(ixture), alphabetically

        // ui needs to load the file also initially
        char fileName[32] = "";
        if (files->seqNrToName(fileName, var["value"])) {
          web->addResponse("pview", "file", JsonString(fileName, JsonString::Copied));
        }
        return true; }
      case f_ChangeFun: {
        eff->fixture.fixtureNr = var["value"];
        eff->fixture.doMap = true;
        eff->fixture.doAllocPins = true;

        //remap all leds
        // for (std::vector<Leds *>::iterator leds=eff->fixture.projections.begin(); leds!=eff->fixture.projections.end(); ++leds) {
        for (Leds *leds: eff->fixture.projections) {
          leds->doMap = true;
        }

        char fileName[32] = "";
        if (files->seqNrToName(fileName, eff->fixture.fixtureNr)) {
          //send to pview a message to get file filename
          web->addResponse("pview", "file", JsonString(fileName, JsonString::Copied));
        }
        return true; }
      default: return false; 
    }}); //fixture

    ui->initCoord3D(currentVar, "fixSize", eff->fixture.fixSize, 0, NUM_LEDS_Max, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ValueFun:
        mdl->setValue(var, eff->fixture.fixSize);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Size");
        return true;
      default: return false;
    }});

    ui->initNumber(currentVar, "fixCount", eff->fixture.nrOfLeds, 0, UINT16_MAX, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ValueFun:
        mdl->setValue(var, eff->fixture.nrOfLeds);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Count");
        web->addResponseV(var["id"], "comment", "Max %d", NUM_LEDS_Max);
        return true;
      default: return false;
    }});

    ui->initNumber(parentVar, "fps", eff->fps, 1, 999, false , [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setComment(var, "Frames per second");
        return true;
      case f_ChangeFun:
        eff->fps = var["value"];
        return true;
      default: return false; 
    }});

    ui->initText(parentVar, "realFps", nullptr, 10, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun:
        web->addResponseV(var["id"], "comment", "f(%d leds)", eff->fixture.nrOfLeds);
        return true;
      default: return false;
    }});

    ui->initCheckBox(parentVar, "fShow", eff->fShow, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "FastLed show");
        ui->setComment(var, "dev performance tuning");
        return true;
      case f_ChangeFun:
        eff->fShow = var["value"];
        return true;
      default: return false; 
    }});

  }
};

extern LedModFixture *fix;