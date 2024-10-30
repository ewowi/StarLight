/*
   @title     StarLight
   @file      LedModEffects.h
   @date      20241014
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "LedModEffects.h"

#include "../Sys/SysModUI.h"
#include "../Sys/SysModFiles.h"
#include "../Sys/SysModSystem.h"

#include "LedEffects.h"
#include "LedProjections.h"
#include "LedLayer.h"



// #define FASTLED_RGBW

//https://www.partsnotincluded.com/fastled-rgbw-neopixels-sk6812/
inline uint16_t getRGBWsize(uint16_t nleds){
	uint16_t nbytes = nleds * 4;
	if(nbytes % 3 > 0) return nbytes / 3 + 1;
	else return nbytes / 3;
}

//https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino
//https://blog.ja-ke.tech/2019/06/02/neopixel-performance.html


  LedModEffects::LedModEffects() :SysModule("Effects") {

    //load effects

    //1D Basis
    effects.push_back(new SolidEffect);
    // 1D FastLed
    effects.push_back(new BPMEffect);
    effects.push_back(new ConfettiEffect);
    effects.push_back(new JuggleEffect);
    effects.push_back(new RainbowWithGlitterEffect);
    effects.push_back(new SinelonEffect);
    //1D StarLight
    effects.push_back(new RingRandomFlowEffect);
    effects.push_back(new RunningEffect);
    // 1D WLED
    effects.push_back(new BouncingBallsEffect);
    effects.push_back(new DripEffect);
    effects.push_back(new FlowEffect);
    effects.push_back(new HeartBeatEffect);
    effects.push_back(new PopCornEffect); //contains wledaudio: useaudio, conditional compile
    effects.push_back(new RainEffect);
    effects.push_back(new RainbowEffect);

    #ifdef STARLIGHT_USERMOD_AUDIOSYNC
      //1D Volume
      effects.push_back(new FreqMatrixEffect);
      effects.push_back(new NoiseMeterEffect);
      //1D frequency
      effects.push_back(new AudioRingsEffect);
      effects.push_back(new DJLightEffect);
    #endif

    //2D StarLight
    effects.push_back(new GameOfLifeEffect); //2D & 3D
    effects.push_back(new LinesEffect);
    effects.push_back(new ParticleTestEffect); //2D & 3D
    effects.push_back(new StarFieldEffect);
    effects.push_back(new PraxisEffect);
    
    //2D WLED
    effects.push_back(new BlackHoleEffect);
    effects.push_back(new DNAEffect);
    effects.push_back(new DistortionWavesEffect);
    effects.push_back(new FrizzlesEffect);
    effects.push_back(new LissajousEffect);
    effects.push_back(new Noise2DEffect);
    effects.push_back(new OctopusEffect);
    effects.push_back(new ScrollingTextEffect);
    #ifdef STARLIGHT_USERMOD_AUDIOSYNC
      //2D WLED
      effects.push_back(new FunkyPlankEffect);
      effects.push_back(new GEQEffect);
      effects.push_back(new LaserGEQEffect);
      effects.push_back(new PaintbrushEffect);
      effects.push_back(new WaverlyEffect);
      effects.push_back(new VUMeterEffect);
    #endif
    //3D
    effects.push_back(new RipplesEffect);
    effects.push_back(new RubiksCubeEffect);
    effects.push_back(new SphereMoveEffect);
    effects.push_back(new PixelMapEffect);
    effects.push_back(new MarioTestEffect);

    #ifdef STARBASE_USERMOD_LIVE
      effects.push_back(new LiveEffect);
    #endif

    //load projections
    projections.push_back(new NoneProjection);
    projections.push_back(new DefaultProjection);
    projections.push_back(new PinwheelProjection);
    projections.push_back(new MirrorReverseTransposeProjection);
    projections.push_back(new GroupingSpacingProjection);
    projections.push_back(new MultiplyProjection);
    projections.push_back(new TiltPanRollProjection);
    projections.push_back(new DistanceFromPointProjection);
    projections.push_back(new Preset1Projection);
    projections.push_back(new RandomProjection);
    // projections.push_back(new ReverseProjection);
    // projections.push_back(new MirrorProjection);
    // projections.push_back(new TransposeProjection);
    projections.push_back(new ScrollingProjection);
    projections.push_back(new AccelerationProjection);
    projections.push_back(new CheckerboardProjection);
    projections.push_back(new RotateProjection);
  };

  void LedModEffects::setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name, 1201);

    JsonObject currentVar;

    JsonObject tableVar = ui->initTable(parentVar, "layers", nullptr, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onUI:
        ui->setComment(var, "List of effects");
        return true;
      case onAdd:
        if (rowNr >= fix->layers.size()) {
          ppf("layers creating new LedsLayer instance %d\n", rowNr);
          LedsLayer *leds = new LedsLayer();
          fix->layers.push_back(leds);
        }
        return true;
      case onDelete:
        // ppf("layers onDelete %s[%d]\n", Variable(var).id(), rowNr);
        //tbd: fade to black
        if (rowNr <fix->layers.size()) {
          LedsLayer *leds = fix->layers[rowNr];
          fix->layers.erase(fix->layers.begin() + rowNr); //remove from vector
          delete leds; //remove leds itself
        }
        return true;
      default: return false;
    }});

    currentVar = ui->initSelect(tableVar, "effect", (uint8_t)0, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      // case onSetValue:
      //   for (size_t rowNr = 0; rowNr < fix->layers.size(); rowNr++)
      //     mdl->setValue(var, fix->layers[rowNr]->effectNr, rowNr);
      //   return true;
      case onUI: {
        ui->setComment(var, "Effect to show");
        JsonArray options = ui->setOptions(var);
        for (Effect *effect:effects) {
          char buf[32] = "";
          strlcat(buf, effect->name(), sizeof(buf));
          strlcat(buf, effect->dim()==_1D?" ┊":effect->dim()==_2D?" ▦":" 🧊", sizeof(buf));
          strlcat(buf, " ", sizeof(buf));
          strlcat(buf, effect->tags(), sizeof(buf));
          options.add(JsonString(buf, JsonString::Copied)); //copy!
        }
        return true; }
      case onChange:

        if (rowNr == UINT8_MAX) rowNr = 0; // in case effect without a rowNr

        //create a new leds instance if a new row is created
        if (rowNr >= fix->layers.size()) {
          ppf("layers effect[%d] onChange #:%d v:%s\n", rowNr, fix->layers.size(), Variable(var).valueString().c_str());
          ppf("effect creating new LedsLayer instance %d\n", rowNr);
          LedsLayer *leds = new LedsLayer();
          fix->layers.push_back(leds);
        }

        if (rowNr < fix->layers.size()) {
          LedsLayer *leds = fix->layers[rowNr];

          // leds->doMap = true; //stop the effects loop already here

          #ifdef STARBASE_USERMOD_LIVE
            //kill Live Script if moving to other effect
            // if (leds->effectNr < effects.size()) {
            //   Effect* effect = effects[leds->effectNr];
              if (leds->effect && strncmp(leds->effect->name(), "Live Effect", 12) == 0) {
                // liveM->kill();
              }
            // }
          #endif

          uint16_t effectNr = mdl->getValue(var, rowNr);

          if (effectNr < effects.size()) {
            leds->effect = effects[effectNr];
            ppf("setEffect effect[%d]: %s\n", rowNr, leds->effect->name());
            strlcat(fix->tickerTape, leds->effect->name(), sizeof(fix->tickerTape));

            if (leds->effect->dim() != leds->effectDimension) {
              leds->effectDimension = leds->effect->dim();
              leds->triggerMapping();
              //initEffect is called after mapping done to make sure dimensions are right before controls are done...
              doInitEffectRowNr = rowNr;
            }
            else {
              initEffect(*leds, rowNr);
            }
          } // effectNr < size

        }
        return true;
      default: return false;
    }});
    currentVar["dash"] = true;

    //projection, default projection is 'default'
    currentVar = ui->initSelect(tableVar, "projection", 1, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      // case onSetValue:
      //   for (size_t rowNr = 0; rowNr < fix->layers.size(); rowNr++)
      //     mdl->setValue(var, fix->layers[rowNr]->projectionNr, rowNr);
      //   return true;
      case onUI: {
        ui->setComment(var, "How to project effect");

        JsonArray options = ui->setOptions(var);
        for (Projection *projection:projections) {
          char buf[32] = "";
          strlcat(buf, projection->name(), sizeof(buf));
          // strlcat(buf, projection->dim()==_1D?" ┊":projection->dim()==_2D?" ▦":" 🧊");
          strlcat(buf, " ", sizeof(buf));
          strlcat(buf, projection->tags(), sizeof(buf));
          options.add(JsonString(buf, JsonString::Copied)); //copy!
        }
        return true; }
      case onChange:

        if (rowNr == UINT8_MAX) rowNr = 0; // in case effect without a rowNr

        if (rowNr < fix->layers.size()) {
          LedsLayer *leds = fix->layers[rowNr];

          // leds->doMap = true; //stop the effects loop already here

          uint8_t proValue = mdl->getValue(var, rowNr);

          if (proValue < projections.size()) {
            if (proValue == 0) //none
              leds->projection = nullptr; //not projections[0] so test on if (leds->projection) can be used
            else
              leds->projection = projections[proValue];

            // leds->addPixelCached = &Projection::addPixel;
            // leds->XYZCached = &Projection::XYZ;

            ppf("initProjection leds[%d] effect:%s a:%d\n", rowNr, leds->effect->name(), leds->projectionData.bytesAllocated);

            leds->projectionData.clear(); //delete effectData memory so it can be rebuild

            Variable(var).preDetails(); //set all positive var N orders to negative
            mdl->setValueRowNr = rowNr;
            if (leds->projection) leds->projection->setup(*leds, var); //not if None projection
            Variable(var).postDetails(rowNr);
            mdl->setValueRowNr = UINT8_MAX;

            leds->projectionData.alertIfChanged = true; //find out when it is changing, eg when projections change, in that case controls are lost...solution needed for that...
          }
          else
            leds->projection = nullptr;
          // ppf("onChange pro[%d] <- %d (%d)\n", rowNr, proValue, fix->layers.size());

          leds->triggerMapping(); //set also fix->mappingStatus
        }
        return true;
      default: return false;
    }});
    currentVar["dash"] = true;

    ui->initCoord3D(tableVar, "start", {0,0,0}, 0, NUM_LEDS_Max, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onSetValue:
        //is this needed?
        for (size_t rowNr = 0; rowNr < fix->layers.size(); rowNr++) {
          ppf("ledsStart[%d] onSetValue %d,%d,%d\n", rowNr, fix->layers[rowNr]->start.x, fix->layers[rowNr]->start.y, fix->layers[rowNr]->start.z);
          mdl->setValue(var, fix->layers[rowNr]->start, rowNr);
        }
        return true;
      case onUI:
        ui->setComment(var, "In pixels");
        return true;
      case onChange:
        if (rowNr < fix->layers.size()) {
          fix->layers[rowNr]->start = mdl->getValue(var, rowNr).as<Coord3D>().minimum(fix->fixSize - Coord3D{1,1,1});

          ppf("ledsStart[%d] onChange %d,%d,%d\n", rowNr, fix->layers[rowNr]->start.x, fix->layers[rowNr]->start.y, fix->layers[rowNr]->start.z);

          fix->layers[rowNr]->fadeToBlackBy();
          fix->layers[rowNr]->triggerMapping();
        }
        else {
          ppf("ledsStart[%d] onChange rownr not in range > %d\n", rowNr, fix->layers.size());
        }
        return true;
      default: return false;
    }});

    ui->initCoord3D(tableVar, "middle", {0,0,0}, 0, NUM_LEDS_Max, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onSetValue:
        //is this needed?
        for (size_t rowNr = 0; rowNr < fix->layers.size(); rowNr++) {
          ppf("ledsMid[%d] onSetValue %d,%d,%d\n", rowNr, fix->layers[rowNr]->middle.x, fix->layers[rowNr]->middle.y, fix->layers[rowNr]->middle.z);
          mdl->setValue(var, fix->layers[rowNr]->middle, rowNr);
        }
        return true;
      case onUI:
        ui->setComment(var, "In pixels");
        return true;
      case onChange:
        if (rowNr < fix->layers.size()) {
          fix->layers[rowNr]->middle = mdl->getValue(var, rowNr).as<Coord3D>().minimum(fix->fixSize - Coord3D{1,1,1});

          ppf("ledsMid[%d] onChange %d,%d,%d\n", rowNr, fix->layers[rowNr]->middle.x, fix->layers[rowNr]->middle.y, fix->layers[rowNr]->middle.z);

          fix->layers[rowNr]->fadeToBlackBy();
          fix->layers[rowNr]->triggerMapping();
        }
        else {
          ppf("ledsMid[%d] onChange rownr not in range > %d\n", rowNr, fix->layers.size());
        }
        return true;
      default: return false;
    }});

    ui->initCoord3D(tableVar, "end", {8,8,0}, 0, NUM_LEDS_Max, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onSetValue:
        //is this needed?
        for (size_t rowNr = 0; rowNr < fix->layers.size(); rowNr++) {
          ppf("ledsEnd[%d] onSetValue %d,%d,%d\n", rowNr, fix->layers[rowNr]->end.x, fix->layers[rowNr]->end.y, fix->layers[rowNr]->end.z);
          mdl->setValue(var, fix->layers[rowNr]->end, rowNr);
        }
        return true;
      case onUI:
        ui->setComment(var, "In pixels");
        return true;
      case onChange:
        if (rowNr < fix->layers.size()) {
          fix->layers[rowNr]->end = mdl->getValue(var, rowNr).as<Coord3D>().minimum(fix->fixSize - Coord3D{1,1,1});

          ppf("ledsEnd[%d] onChange %d,%d,%d\n", rowNr, fix->layers[rowNr]->end.x, fix->layers[rowNr]->end.y, fix->layers[rowNr]->end.z);

          fix->layers[rowNr]->fadeToBlackBy();
          fix->layers[rowNr]->triggerMapping();
        }
        else {
          ppf("ledsEnd[%d] onChange rownr not in range > %d\n", rowNr, fix->layers.size());
        }
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "size", nullptr, 32, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onSetValue: {
        // for (std::vector<LedsLayer *>::iterator leds=fix->layers.begin(); leds!=fix->layers.end(); ++leds) {
        uint8_t rowNr = 0;
        for (LedsLayer *leds:fix->layers) {
          char message[32];
          print->fFormat(message, sizeof(message), "%d x %d x %d -> %d", leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds);
          ppf("onSetValue ledsSize[%d] = %s\n", rowNr, message);
          mdl->setValue(var, JsonString(message, JsonString::Copied), rowNr); //rowNr
          rowNr++;
        }
        return true; }
      default: return false;
    }});

    // ui->initSelect(parentVar, "layout", 0, false, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    //   case onUI: {
    //     ui->setComment(var, "WIP");
    //     JsonArray options = ui->setOptions(var);
    //     options.add("□"); //0
    //     options.add("="); //1
    //     options.add("||"); //2
    //     options.add("+"); //3
    //     return true;
    //   }
    //   default: return false;
    // }}); //effect Layout

    ui->initSlider(parentVar, "Blending", &fix->globalBlend);

    #ifdef STARBASE_USERMOD_E131
      // if (e131mod->isEnabled) {
          e131mod->patchChannel(0, "Fixture", "brightness", 255); //should be 256??
          e131mod->patchChannel(1, "layers", "effect", effects.size());
          e131mod->patchChannel(2, "effect", "palette", 8); //tbd: calculate nr of palettes (from select)
          // //add these temporary to test remote changing of this values do not crash the system
          // e131mod->patchChannel(3, "projection", Projections::count);
          // e131mod->patchChannel(4, "fixture", 5); //assuming 5!!!

          // ui->dashVarChanged = true;
          // //rebuild the table
          for (JsonObject childVar: Variable(mdl->findVar("E131", "watches")).children())
            ui->callVarFun(childVar, UINT8_MAX, onSetValue); //set the value (WIP)

      // }
      // else
      //   ppf("e131 not enabled\n");
    #endif

    //for use in loop
    varSystem = mdl->findVar("m", "System");
  }

  //this loop is run as often as possible so coding should also be as efficient as possible (no findVars etc)
  void LedModEffects::loop() {
    // SysModule::loop();

    random16_set_seed(sys->now);

    //set new frame
    if (sys->now - frameMillis >= 1000.0/fix->fps && fix->mappingStatus == 0) {

      //reset pixelsToBlend if multiple leds effects
      // ppf(" %d-%d", fix->pixelsToBlend.size(), fix->nrOfLeds);
      if (fix->layers.size()) //if more then one effect
        for (uint16_t indexP=0; indexP < fix->pixelsToBlend.size(); indexP++)
          fix->pixelsToBlend[indexP] = false;

      frameMillis = sys->now;

      newFrame = true;

      //for each programmed effect
      //  run the next frame of the effect
      for (uint8_t rowNr = 0; rowNr < fix->layers.size(); rowNr++) {
        LedsLayer *leds = fix->layers[rowNr];
        if (leds->effect && !leds->doMap) { // don't run effect while remapping or non existing effect (default UINT16_MAX)
          // ppf(" %s %d,%d,%d - %d,%d,%d (%d,%d,%d)", leds->effect->name(), leds->start.x, leds->start.y, leds->start.z, leds->end.x, leds->end.y, leds->end.z, leds->size.x, leds->size.y, leds->size.z );

          leds->effectData.begin(); //sets the effectData pointer back to 0 so loop effect can go through it

          mdl->getValueRowNr = rowNr;
          leds->effect->loop(*leds);
          mdl->getValueRowNr = UINT8_MAX;

          if (fix->showTicker && rowNr == fix->layers.size() -1) { //last effect, add sysinfo
            char text[20];
            if (leds->size.x > 48)
              print->fFormat(text, sizeof(text), "%d @ %.3d %s", fix->fixSize.x * fix->fixSize.y, fix->realFps, fix->tickerTape);
            else
              print->fFormat(text, sizeof(text), "%.3d %s", fix->realFps, fix->tickerTape);
            leds->drawText(text, 16, 0, 1); //16 should be 0 after I have my first panel working ;-)
          }

          // if (leds->projectionNr == p_TiltPanRoll || leds->projectionNr == p_Preset1)
          //   leds->fadeToBlackBy(50);

          //loop over mapped pixels and set pixelsToBlend to true
          if (fix->layers.size()) { //if more then one effect
            for (std::vector<uint16_t> mappingTableIndex: leds->mappingTableIndexes) {
              for (uint16_t indexP: mappingTableIndex)
                fix->pixelsToBlend[indexP] = true;
            }
            for (PhysMap physMap: leds->mappingTable) {
              if (physMap.mapType == m_onePixel)
                fix->pixelsToBlend[physMap.indexP] = true;
            }
          }
        }
      }

      frameCounter++;
    }
    else {
      newFrame = false;
    }

    // JsonObject varSystem = mdl->findVar("m", "System");
    if (!varSystem["canvasData"].isNull()) { //tbd: pubsub system
      const char * canvasData = varSystem["canvasData"]; //0 - 494 - 140,150,0
      ppf("LedModEffects loop canvasData %s\n", canvasData);

      uint8_t rowNr = 0; //currently only leds[0] supported
      if (fix->layers.size()) { //if more then one effect
        fix->layers[rowNr]->fadeToBlackBy();

        char * token = strtok((char *)canvasData, ":");
        bool isStart = strncmp(token, "start", 6) == 0;
        bool isEnd = strncmp(token, "end", 4) == 0;

        Coord3D midCoord; //placeHolder for mid

        Coord3D *newCoord = isStart? &fix->layers[rowNr]->start: isEnd? &fix->layers[rowNr]->end : &midCoord;

        if (newCoord) {
          token = strtok(NULL, ",");
          if (token != NULL) newCoord->x = atoi(token) / fix->factor; else newCoord->x = 0; //should never happen
          token = strtok(NULL, ",");
          if (token != NULL) newCoord->y = atoi(token) / fix->factor; else newCoord->y = 0;
          token = strtok(NULL, ",");
          if (token != NULL) newCoord->z = atoi(token) / fix->factor; else newCoord->z = 0;

          mdl->setValue("layers", isStart?"start":isEnd?"end":"middle", *newCoord, 0); //assuming row 0 for the moment

          fix->layers[rowNr]->triggerMapping();
        }

        varSystem.remove("canvasData"); //convasdata has been processed
      }
    }

  } //loop

  void LedModEffects::initEffect(LedsLayer &leds, uint8_t rowNr) {
      ppf("initEffect leds[%d] effect:%s a:%d (%d,%d,%d)\n", rowNr, leds.effect->name(), leds.effectData.bytesAllocated, leds.size.x, leds.size.y, leds.size.z);

      leds.effectData.clear(); //delete effectData memory so it can be rebuild

      leds.effect->loop(leds); leds.effectData.begin(); //do a loop to set effectData right

      JsonObject var = mdl->findVar("layers", "effect");
      Variable(var).preDetails();
      mdl->setValueRowNr = rowNr;
      leds.effect->setup(leds, var); //if changed then run setup once (like call==0 in WLED) and set all defaults in effectData
      Variable(var).postDetails(rowNr);
      mdl->setValueRowNr = UINT8_MAX;

      leds.effectData.alertIfChanged = true; //find out when it is changing, eg when projections change, in that case controls are lost...solution needed for that...


  }
