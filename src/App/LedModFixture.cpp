/*
   @title     StarLight
   @file      LedModFixture.h
   @date      20241014
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "LedModFixture.h"

#include "../Sys/SysModUI.h"
#include "../Sys/SysModFiles.h"

#include "LedModEffects.h"

  void LedModFixture::setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name, 1100);

    JsonObject currentVar = ui->initCheckBox(parentVar, "on", true, false, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
        mdl->callVarOnChange(mdl->findVar("Fixture", "brightness"), UINT8_MAX, true); //set brightness (init is true so bri value not send via udp)
        return true;
      default: return false;
    }});
    currentVar["dash"] = true;

    //logarithmic slider (10)
    currentVar = ui->initSlider(parentVar, "brightness", &bri, 0, 255, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange: {
        //bri set by StarMod during onChange
        uint8_t result = mdl->getValue("Fixture", "on").as<bool>()?mdl->linearToLogarithm(bri):0;

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

    currentVar = ui->initCanvas(parentVar, "preview", UINT16_MAX, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onUI:
        eff->fixture.mappingStatus = 1; //rebuild the fixture - so it is send to ui
        return true;
      case onLoop: {
        if (eff->fixture.mappingStatus == 0 && bytesPerPixel) { //not remapping
          var["interval"] = max(eff->fixture.nrOfLeds * web->ws.count()/200, 16U)*10; //interval in ms * 10, not too fast //from cs to ms

          #define headerBytes 5
          // ppf("(%d %d %d,%d,%d)", len, headerBytes + eff->fixture.nrOfLeds * bytesPerPixel, eff->fixture.fixSize.x, eff->fixture.fixSize.y, eff->fixture.fixSize.z);
          size_t len = min(headerBytes + eff->fixture.nrOfLeds * bytesPerPixel, 4096);
          AsyncWebSocketMessageBuffer *wsBuf= web->ws.makeBuffer(len); //global wsBuf causes crash in audio sync module!!!
          if (wsBuf) {
            wsBuf->lock();
            byte* buffer = wsBuf->get();
            //new values
            buffer[0] = 2; //userFun id
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
            buffer[4] = bytesPerPixel;
            uint16_t previewBufferIndex = headerBytes;

            // send leds preview to clients
            for (size_t indexP = 0; indexP < eff->fixture.nrOfLeds; indexP++) {

              if (previewBufferIndex + bytesPerPixel > 4096) {
                //send the buffer and create a new one
                web->sendBuffer(wsBuf, true);
                delay(10);
                buffer[0] = 2; //userFun id
                buffer[1] = UINT8_MAX; //indicates follow up package
                buffer[2] = indexP/256; //fixSize.x%256;
                buffer[3] = indexP%256; //fixSize.x%256;
                buffer[4] = bytesPerPixel;
                // ppf("@");
                // ppf("new buffer created i:%d p:%d r:%d r6:%d\n", indexP, previewBufferIndex, (eff->fixture.nrOfLeds - indexP), (eff->fixture.nrOfLeds - indexP) * 6);
                previewBufferIndex = 5;
              }

              if (bytesPerPixel == 1) {
                //encode rgb in 8 bits: 3 for red, 3 for green, 2 for blue (0xE0 = 01110000)
                buffer[previewBufferIndex++] = (eff->fixture.ledsP[indexP].red & 0xE0) | ((eff->fixture.ledsP[indexP].green & 0xE0)>>3) | (eff->fixture.ledsP[indexP].blue >> 6);
              }
              else if (bytesPerPixel == 2) {
                //encode rgb in 16 bits: 5 for red, 6 for green, 5 for blue
                buffer[previewBufferIndex++] = (eff->fixture.ledsP[indexP].red & 0xF8) | (eff->fixture.ledsP[indexP].green >> 5); // Take 5 bits of Red component and 3 bits of G component
                buffer[previewBufferIndex++] = ((eff->fixture.ledsP[indexP].green & 0x1C) << 3) | (eff->fixture.ledsP[indexP].blue  >> 3); // Take remaining 3 Bits of G component and 5 bits of Blue component
              }
              else {
                buffer[previewBufferIndex++] = eff->fixture.ledsP[indexP].red;
                buffer[previewBufferIndex++] = eff->fixture.ledsP[indexP].green;
                buffer[previewBufferIndex++] = eff->fixture.ledsP[indexP].blue;
              }
            } //loop

            web->sendBuffer(wsBuf, true);

            wsBuf->unlock();
            web->ws._cleanBuffers();
          }

        }

        return true;}
      default: return false;
    }});

    ui->initSelect(currentVar, "rotation", &viewRotation, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onUI: {
        JsonArray options = ui->setOptions(var);
        options.add("None");
        options.add("Tilt");
        options.add("Pan");
        options.add("Roll");
        #ifdef STARLIGHT_USERMOD_AUDIOSYNC
          options.add("Moving heads GEQ");
        #endif
        return true; }
      default: return false; 
    }});

    ui->initSelect(currentVar, "PixelBytes", &bytesPerPixel, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onUI: {
        JsonArray options = ui->setOptions(var);
        options.add("None");
        options.add("1-byte RGB");
        options.add("2-byte RGB");
        options.add("3-byte RGB");
        return true; }
      default: return false; 
    }});

    currentVar = ui->initSelect(parentVar, "fixture", &eff->fixture.fixtureNr, false ,[](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onUI: {
        // ui->setComment(var, "Fixture to display effect on");
        JsonArray options = ui->setOptions(var);
        files->dirToJson(options, true, "F_"); //only files containing F(ixture), alphabetically

        // ui needs to load the file also initially
        char fileName[32] = "";
        if (files->seqNrToName(fileName, var["value"])) {
          web->addResponse(mdl->findVar("Fixture", "preview"), "file", JsonString(fileName, JsonString::Copied));
        }
        return true; }
      case onChange: {
        eff->fixture.doAllocPins = true;

        //remap all leds
        // for (std::vector<LedsLayer *>::iterator leds=eff->fixture.layers.begin(); leds!=eff->fixture.layers.end(); ++leds) {
        for (LedsLayer *leds: eff->fixture.layers) {
          leds->triggerMapping();
        }

        // char fileName[32] = "";
        // if (files->seqNrToName(fileName, eff->fixture.fixtureNr)) {
        //   //send to preview a message to get file fileName
        //   web->addResponse(mdl->findVar("Fixture", "preview"), "file", JsonString(fileName, JsonString::Copied));
        // }
        return true; }
      default: return false; 
    }}); //fixture

    ui->initCoord3D(currentVar, "size", &eff->fixture.fixSize, 0, NUM_LEDS_Max, true, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      default: return false;
    }});

    ui->initNumber(currentVar, "count", &eff->fixture.nrOfLeds, 0, UINT16_MAX, true, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onUI:
        web->addResponse(var, "comment", "Max %d", NUM_LEDS_Max, 0); //0 is to force format overload used
        return true;
      default: return false;
    }});

    ui->initNumber(parentVar, "fps", &eff->fps, 1, 999, false, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onUI:
        ui->setComment(var, "Frames per second");
        return true;
      default: return false; 
    }});

    ui->initNumber(parentVar, "realFps", uint16_t(0), 0, UINT16_MAX, true, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onUI:
        web->addResponse(var, "comment", "f(%d leds)", eff->fixture.nrOfLeds, 0); //0 is to force format overload used
        return true;
      case onLoop1s:
          mdl->setValue(var, eff->frameCounter);
          eff->frameCounter = 0;
        return true;
      default: return false;
    }});

    ui->initCheckBox(parentVar, "driverShow", &eff->driverShow, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
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

