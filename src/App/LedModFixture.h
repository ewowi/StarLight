/*
   @title     StarLight
   @file      LedModFixture.h
   @date      20240720
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

class LedModFixture:public SysModule {

public:

  uint8_t viewRotation = 0;
  uint8_t bri = 10;
  bool rgb1B = true;

  LedModFixture() :SysModule("Fixture") {};

  void setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name, 1100);

    JsonObject currentVar = ui->initCheckBox(parentVar, "on", true, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "On");
        return true;
      case onChange:
        mdl->callVarChangeFun(mdl->findVar("bri"), UINT8_MAX, true); //set brightness (init is true so bri value not send via udp)
        return true;
      default: return false;
    }});
    currentVar["dash"] = true;

    //logarithmic slider (10)
    currentVar = ui->initSlider(parentVar, "bri", &bri, 0, 255, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Brightness");
        return true;
      case onChange: {
        //bri set by StarMod during onChange
        stackUnsigned8 result = mdl->getValue("on").as<bool>()?mdl->varLinearToLogarithm(var, bri):0;

        #ifdef STARLIGHT_CLOCKLESS_LED_DRIVER
          eff->driver.setBrightness(result * eff->fixture.setMaxPowerBrightness / 256);
        #else
          FastLED.setBrightness(result);
        #endif

        ppf("Set Brightness to %d -> b:%d r:%d\n", var["value"].as<int>(), bri, result);
        return true; }
      default: return false; 
    }});
    currentVar["log"] = true; //logarithmic
    currentVar["dash"] = true; //these values override model.json???

    currentVar = ui->initCanvas(parentVar, "pview", UINT16_MAX, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Preview");
        // ui->setComment(var, "Shows the fixture");
        // ui->setComment(var, "Click to enlarge");
        return true;
      case onLoop: {
        var["interval"] =  max(eff->fixture.nrOfLeds * web->ws.count()/200, 16U)*10; //interval in ms * 10, not too fast //from cs to ms

        web->sendDataWs([this](AsyncWebSocketMessageBuffer * wsBuf) {
          byte* buffer;

          buffer = wsBuf->get();

          #define headerBytes 4

          //new values
          buffer[0] = 1; //userFun id
          //rotations
          if (viewRotation == 0) {
            buffer[1] = 0;
            buffer[2] = 0;
            buffer[3] = 0;
          } else if (viewRotation == 1) { //tilt
            buffer[1] = beat8(1);//, 0, 255);
            buffer[2] = 0;//beatsin8(4, 250, 5);
            buffer[3] = 0;//beatsin8(6, 255, 5);
          } else if (viewRotation == 2) { //pan
            buffer[1] = 0;//beatsin8(4, 250, 5);
            buffer[2] = beat8(1);//, 0, 255);
            buffer[3] = 0;//beatsin8(6, 255, 5);
          } else if (viewRotation == 3) { //roll
            buffer[1] = 0;//beatsin8(4, 250, 5);
            buffer[2] = 0;//beatsin8(6, 255, 5);
            buffer[3] = beat8(1);//, 0, 255);
          } else if (viewRotation == 4) {
            buffer[1] = eff->fixture.head.x;
            buffer[2] = eff->fixture.head.y;
            buffer[3] = eff->fixture.head.z;
          }

          // send leds preview to clients
          for (size_t i = 0; i < eff->fixture.nrOfLeds; i++)
          {
            if (rgb1B) {
              //encode rgb in 8 bits: 3 for red, 3 for green, 2 for blue
              buffer[headerBytes + i] = (eff->fixture.ledsP[i].red & 0xE0) | ((eff->fixture.ledsP[i].green & 0xE0)>>3) | (eff->fixture.ledsP[i].blue >> 6);
            }
            else {
              buffer[headerBytes + i*3] = eff->fixture.ledsP[i].red;
              buffer[headerBytes + i*3+1] = eff->fixture.ledsP[i].green;
              buffer[headerBytes + i*3+2] = eff->fixture.ledsP[i].blue;
            }
          }
        }, headerBytes + eff->fixture.nrOfLeds * (rgb1B?1:3), true);
        return true;
      }
      default: return false;
    }});

    ui->initSelect(currentVar, "viewRot", &viewRotation, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI: {
        ui->setLabel(var, "Rotation");
        // ui->setComment(var, "View rotation");
        JsonArray options = ui->setOptions(var);
        options.add("None");
        options.add("Tilt");
        options.add("Pan");
        options.add("Roll");
        #ifdef STARLIGHT_USERMOD_WLEDAUDIO
          options.add("Moving heads GEQ");
        #endif
        return true; }
      default: return false; 
    }});

    ui->initCheckBox(currentVar, "rgb1B", &rgb1B, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "1-byte RGB");
        return true;
      default: return false;
    }});

    currentVar = ui->initSelect(parentVar, "fixture", &eff->fixture.fixtureNr, false ,[](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI: {
        // ui->setComment(var, "Fixture to display effect on");
        JsonArray options = ui->setOptions(var);
        files->dirToJson(options, true, "F_"); //only files containing F(ixture), alphabetically

        // ui needs to load the file also initially
        char fileName[32] = "";
        if (files->seqNrToName(fileName, var["value"])) {
          web->addResponse("pview", "file", JsonString(fileName, JsonString::Copied));
        }
        return true; }
      case onChange: {
        eff->fixture.doAllocPins = true;

        //remap all leds
        // for (std::vector<LedsLayer *>::iterator leds=eff->fixture.layers.begin(); leds!=eff->fixture.layers.end(); ++leds) {
        for (LedsLayer *leds: eff->fixture.layers) {
          leds->triggerMapping();
        }

        char fileName[32] = "";
        if (files->seqNrToName(fileName, eff->fixture.fixtureNr)) {
          //send to pview a message to get file fileName
          web->addResponse("pview", "file", JsonString(fileName, JsonString::Copied));
        }
        return true; }
      default: return false; 
    }}); //fixture

    ui->initCoord3D(currentVar, "fixSize", &eff->fixture.fixSize, 0, NUM_LEDS_Max, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Size");
        return true;
      default: return false;
    }});

    ui->initNumber(currentVar, "fixCount", &eff->fixture.nrOfLeds, 0, UINT16_MAX, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Count");
        web->addResponseV(var["id"], "comment", "Max %d", NUM_LEDS_Max);
        return true;
      default: return false;
    }});

    ui->initNumber(parentVar, "fps", &eff->fps, 1, 999, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setComment(var, "Frames per second");
        return true;
      default: return false; 
    }});

    ui->initText(parentVar, "realFps", nullptr, 10, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        web->addResponseV(var["id"], "comment", "f(%d leds)", eff->fixture.nrOfLeds);
        return true;
      default: return false;
    }});

    ui->initCheckBox(parentVar, "fShow", &eff->fShow, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        #ifdef STARLIGHT_CLOCKLESS_LED_DRIVER
          ui->setLabel(var, "CLD Show");
        #else
          ui->setLabel(var, "FastLED Show");
        #endif
        ui->setComment(var, "dev performance tuning");
        return true;
      default: return false; 
    }});

  }
};

extern LedModFixture *fix;