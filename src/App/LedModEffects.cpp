/*
   @title     StarLight
   @file      LedModEffects.h
   @date      20241219
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

    effects.push_back(new BlackHoleEffect);
    effects.push_back(new BouncingBallsEffect);
    effects.push_back(new BPMEffect);
    effects.push_back(new ConfettiEffect);
    effects.push_back(new DistortionWavesEffect);
    effects.push_back(new DNAEffect);
    effects.push_back(new DripEffect);
    effects.push_back(new FireEffect);
    effects.push_back(new FireworksEffect);
    effects.push_back(new FlowEffect); //10
    effects.push_back(new FrizzlesEffect);
    effects.push_back(new GameOfLifeEffect); //2D & 3D
    effects.push_back(new HeartBeatEffect);
    effects.push_back(new JuggleEffect);
    effects.push_back(new LinesEffect);
    effects.push_back(new LissajousEffect); //16
    effects.push_back(new MarioTestEffect);
    effects.push_back(new Noise2DEffect);
    effects.push_back(new OctopusEffect);
    effects.push_back(new ParticleTestEffect); //2D & 3D 20
    effects.push_back(new PopCornEffect); //contains wledaudio: useaudio, conditional compile
    effects.push_back(new PixelMapEffect);
    effects.push_back(new PraxisEffect);
    effects.push_back(new RainEffect);
    effects.push_back(new RainbowEffect);
    effects.push_back(new RainbowWithGlitterEffect);
    effects.push_back(new RingRandomFlowEffect);
    effects.push_back(new RipplesEffect); //28
    effects.push_back(new RubiksCubeEffect);
    effects.push_back(new RunningEffect);
    effects.push_back(new ScrollingTextEffect);
    effects.push_back(new SinelonEffect);
    effects.push_back(new SphereMoveEffect);
    effects.push_back(new StarFieldEffect);

    #ifdef STARLIGHT_USERMOD_AUDIOSYNC
      effects.push_back(new AudioRingsEffect);
      effects.push_back(new DJLightEffect);
      effects.push_back(new FreqMatrixEffect);
      effects.push_back(new FunkyPlankEffect);
      effects.push_back(new GEQEffect);
      effects.push_back(new LaserGEQEffect);
      effects.push_back(new NoiseMeterEffect);
      effects.push_back(new PaintbrushEffect);
      effects.push_back(new VUMeterEffect);
      effects.push_back(new WaverlyEffect);
    #endif

    #ifdef STARBASE_USERMOD_LIVE
      liveEffect = new LiveEffect;
    #endif

    //load projections
    #ifdef STARLIGHT_LIVE_MAPPING
      projections.push_back(new LiveMappingProjection);
    #else
      projections.push_back(new NoneProjection); //0
    #endif
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
    #ifdef STARBASE_USERMOD_MPU6050
      projections.push_back(new AccelerationProjection);
    #endif
    projections.push_back(new CheckerboardProjection); //12
    projections.push_back(new RotateProjection);
    projections.push_back(new RippleYZ);
  }; //constructor

  void LedModEffects::setup() {
    SysModule::setup();

    const Variable parentVar = ui->initAppMod(Variable(), name, 1201);

    Variable tableVar = ui->initTable(parentVar, "layers", nullptr, false, [this](EventArguments) { switch (eventType) {
      case onUI:
        variable.setComment("List of effects");
        return true;
      case onAdd:
        if (rowNr >= fix->layers.size()) {
          ppf("layers creating new LedsLayer instance %d\n", rowNr);
          LedsLayer *leds = new LedsLayer();
          fix->layers.push_back(leds);
        }
        return true;
      case onDelete:
        ppf("layers onDelete %s[%d]\n", variable.id(), rowNr);
        //tbd: fade to black
        if (rowNr < fix->layers.size()) {
          LedsLayer *leds = fix->layers[rowNr];
          fix->layers.erase(fix->layers.begin() + rowNr); //remove from vector
          delete leds; //remove leds itself
        }
        return true;
      default: return false;
    }});

    Variable currentVar = ui->initSelect(tableVar, "effect", (uint8_t)0, false, [this](EventArguments) { switch (eventType) {
      // case onSetValue:
      //   for (size_t rowNr = 0; rowNr < fix->layers.size(); rowNr++)
      //     variable.setValue(fix->layers[rowNr]->effectNr, rowNr);
      //   return true;
      case onUI: {
        variable.setComment("Effect to show");
        JsonArray options = variable.setOptions();
        for (Effect *effect:effects) {
          StarString buf;
          buf += effect->name();
          buf += effect->dim()==_1D?" ┊":effect->dim()==_2D?" ▦":" 🧊";
          buf += " ";
          buf += effect->tags();
          options.add(buf.getString()); //copy!
        }
        // add live effects from FS
        files->dirToJson(options, true, "E_"); //only files containing E_(ffects), alphabetically
        return true; }
      case onChange:

        if (sys->safeMode) return true; //do not process effect in safeMode do this if the effect crashes at boot, then change effect to working effect and reboot

        print->printJson("layers.effect.onChange", variable.var);

        if (rowNr == UINT8_MAX) rowNr = 0; // in case effect without a rowNr

        //create a new leds instance if a new row is created
        if (rowNr >= fix->layers.size()) {
          ppf("layers effect[%d] onChange #:%d v:%s\n", rowNr, fix->layers.size(), variable.valueString().c_str());
          ppf("effect creating new LedsLayer instance %d\n", rowNr);
          LedsLayer *leds = new LedsLayer();
          fix->layers.push_back(leds);
        }

        if (rowNr < fix->layers.size()) {
          LedsLayer *leds = fix->layers[rowNr];

          // leds->doMap = true; //stop the effects loop already here

          #ifdef STARBASE_USERMOD_LIVE
            //kill Live Script if moving to other effect
            if (leds->effect && leds->liveEffectID != UINT8_MAX && strnstr(leds->effect->name(), "Live Effect", 11) != nullptr) {
              ESP_LOGD("", "effect.onChange kill effect %d", leds->liveEffectID);
              liveM->killAndDelete(leds->liveEffectID);
              leds->liveEffectID = UINT8_MAX;
            }
          #endif

          uint16_t effectNr = variable.getValue(rowNr);

          if (effectNr < effects.size()) {
            leds->effect = effects[effectNr];
          }
          else 
            leds->effect = liveEffect;

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

        }
        return true;
      default: return false;
    }});
    currentVar.var["dash"] = true;

    //projection, default projection is 'default'
    currentVar = ui->initSelect(tableVar, "projection", 1, false, [this](EventArguments) { switch (eventType) {
      // case onSetValue:
      //   for (size_t rowNr = 0; rowNr < fix->layers.size(); rowNr++)
      //     variable.setValue(fix->layers[rowNr]->projectionNr, rowNr);
      //   return true;
      case onUI: {
        variable.setComment("How to project effect");

        JsonArray options = variable.setOptions();
        for (Projection *projection:projections) {
          char buf[32] = "";
          strlcat(buf, projection->name(), sizeof(buf));
          // strlcat(buf, projection->dim()==_1D?" ┊":projection->dim()==_2D?" ▦":" 🧊");
          strlcat(buf, " ", sizeof(buf));
          strlcat(buf, projection->tags(), sizeof(buf));
          options.add(JsonString(buf)); //copy!
        }
        return true; }
      case onChange:

        if (rowNr == UINT8_MAX) rowNr = 0; // in case effect without a rowNr

        if (rowNr < fix->layers.size()) {
          LedsLayer *leds = fix->layers[rowNr];

          // leds->doMap = true; //stop the effects loop already here

          uint8_t proValue = variable.getValue(rowNr);

          if (proValue < projections.size()) {
            if (proValue == 0) //none
              leds->projection = nullptr; //not projections[0] so test on if (leds->projection) can be used
            else
              leds->projection = projections[proValue];

            ppf("initProjection leds[%d] projection:%s a:%d\n", rowNr, leds->projection?leds->projection->name():"None", leds->projectionData.bytesAllocated);

            leds->projectionData.clear(); //delete effectData memory so it can be rebuild
            leds->projectionData.read<uint8_t>(); leds->projectionData.begin(); //allocate minimum amount for projectionData (chunk of 32 bytes) to avoid control defaults to be removed

            variable.preDetails(); //set all positive var N orders to negative
            mdl->setValueRowNr = rowNr;
            if (leds->projection) leds->projection->setup(*leds, variable); //not if None projection
            variable.postDetails(rowNr);

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
    currentVar.var["dash"] = true;

    ui->initCoord3D(tableVar, "start", {0,0,0}, 0, STARLIGHT_MAXLEDS, false, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        //is this needed?
        for (size_t rowNr = 0; rowNr < fix->layers.size(); rowNr++) {
          ppf("ledsStart[%d] onSetValue %d,%d,%d\n", rowNr, fix->layers[rowNr]->start.x, fix->layers[rowNr]->start.y, fix->layers[rowNr]->start.z);
          variable.setValue(fix->layers[rowNr]->start, rowNr);
        }
        return true;
      case onUI:
        variable.setComment("In pixels");
        return true;
      case onChange:
        if (rowNr < fix->layers.size()) {
          fix->layers[rowNr]->start = variable.getValue(rowNr).as<Coord3D>().minimum(fix->fixSize - Coord3D{1,1,1});

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

    ui->initCoord3D(tableVar, "middle", {0,0,0}, 0, STARLIGHT_MAXLEDS, false, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        //is this needed?
        for (size_t rowNr = 0; rowNr < fix->layers.size(); rowNr++) {
          ppf("ledsMid[%d] onSetValue %d,%d,%d\n", rowNr, fix->layers[rowNr]->middle.x, fix->layers[rowNr]->middle.y, fix->layers[rowNr]->middle.z);
          variable.setValue(fix->layers[rowNr]->middle, rowNr);
        }
        return true;
      case onUI:
        variable.setComment("In pixels");
        return true;
      case onChange:
        if (rowNr < fix->layers.size()) {
          fix->layers[rowNr]->middle = variable.getValue(rowNr).as<Coord3D>().minimum(fix->fixSize - Coord3D{1,1,1});

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

    ui->initCoord3D(tableVar, "end", {8,8,0}, 0, STARLIGHT_MAXLEDS, false, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        //is this needed?
        for (size_t rowNr = 0; rowNr < fix->layers.size(); rowNr++) {
          ppf("ledsEnd[%d] onSetValue %d,%d,%d\n", rowNr, fix->layers[rowNr]->end.x, fix->layers[rowNr]->end.y, fix->layers[rowNr]->end.z);
          variable.setValue(fix->layers[rowNr]->end, rowNr);
        }
        return true;
      case onUI:
        variable.setComment("In pixels");
        return true;
      case onChange:
        if (rowNr < fix->layers.size()) {
          fix->layers[rowNr]->end = variable.getValue(rowNr).as<Coord3D>().minimum(fix->fixSize - Coord3D{1,1,1});

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

    ui->initText(tableVar, "size", nullptr, 32, true, [this](EventArguments) { switch (eventType) {
      case onSetValue: {
        // for (std::vector<LedsLayer *>::iterator leds=fix->layers.begin(); leds!=fix->layers.end(); ++leds) {
        uint8_t rowNr = 0;
        for (LedsLayer *leds:fix->layers) {
          StarString message;
          message.format("%d x %d x %d", leds->size.x, leds->size.y, leds->size.z);
          ppf("onSetValue ledsSize[%d] = %s\n", rowNr, message.getString());
          variable.setValue(JsonString(message.getString()), rowNr); //rowNr
          rowNr++;
        }
        return true; }
      default: return false;
    }});

    // ui->initSelect(parentVar, "layout", 0, false, [](EventArguments) { switch (eventType) {
    //   case onUI: {
    //     variable.setComment("WIP");
    //     JsonArray options = variable.setOptions();
    //     options.add("□"); //0
    //     options.add("="); //1
    //     options.add("||"); //2
    //     options.add("+"); //3
    //     return true;
    //   }
    //   default: return false;
    // }}); //effect Layout

    ui->initSlider(parentVar, "Blending", &fix->globalBlend);

    addPresets(parentVar.var);

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
          for (JsonObject childVar: Variable("E131", "patches").children())
            Variable(childVar).triggerEvent(onSetValue); //set the value (WIP)

      // }
      // else
      //   ppf("e131 not enabled\n");
    #endif

    //for use in loop
    varSystem = mdl->findVar("m", "System");
  }

  //this loop is run as often as possible so coding should also be as efficient as possible (no findVar etc)
  void LedModEffects::loop() {
    // SysModule::loop();

    random16_set_seed(sys->now);

    //set new frame
    if (sys->now - frameMillis >= 1000.0/fix->fps - 1 && fix->mappingStatus == 0 && fix->ledsP[0].b != 100) { //floorf to make it no wait to go beyond 1000 fps ;-), ledsP[0].b: fixChange

      //reset pixelsToBlend if multiple leds effects
      // ppf(" %d-%d", fix->pixelsToBlend.size(), fix->nrOfLeds);
      if (fix->layers.size() > 1) //if more then one effect
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
          //using cached virtual class methods! (so no need for if projectionNr optimizations!)
          if (leds->projection) {
            leds->projectionData.begin();
            (leds->projection->*leds->loopCached)(*leds);
          }
          mdl->getValueRowNr = UINT8_MAX;

          if (fix->showTicker && rowNr == fix->layers.size() - 1) { //last effect, add sysinfo
            StarString text;
            if (leds->size.x > 48) {
              #ifdef STARLIGHT_VIRTUAL_DRIVER
                if (strlen(fix->tickerTape))
                  text.format("%d @ %.3d %s", fix->fixSize.x * fix->fixSize.y, fix->realFps, fix->tickerTape);
                else
                  text.format("%d @ %.3d %.1fMHz %dDB", fix->fixSize.x * fix->fixSize.y, fix->realFps, fix->clockFreq / 10.0, __NB_DMA_BUFFER);
              #else
                text.format("%d @ %.3d %s", fix->fixSize.x * fix->fixSize.y, fix->realFps, fix->tickerTape);
              #endif
            } else
              text.format("%.3d %s", fix->realFps, fix->tickerTape);
            leds->drawText(text.getString(), 0, 0, 1);
          }

          // if (leds->projectionNr == p_TiltPanRoll || leds->projectionNr == p_Preset1)
          //   leds->fadeToBlackBy(50);

          //loop over mapped pixels and set pixelsToBlend to true
          if (fix->layers.size() > 1) { //if more then one effect
            for (const std::vector<uint16_t>& mappingTableIndex: leds->mappingTableIndexes) {
              for (const uint16_t indexP: mappingTableIndex)
                fix->pixelsToBlend[indexP] = true;
            }
            for (const PhysMap &physMap: leds->mappingTable) {
              if (physMap.mapType == m_onePixel)
                fix->pixelsToBlend[physMap.indexP] = true;
            }
          }
        }
      }

      if (fix->ledsP[0].b == 100) fix->ledsP[0].b--; //if fixChange pattern created then change that (as this is not a fixChange)

      frameCounter++;
    }
    else {
      newFrame = false;
    }

    // JsonObject varSystem = mdl->findVar("m", "System");
    if (!varSystem["canvasData"].isNull()) { //tbd: pubsub system
      const char * canvasData = varSystem["canvasData"]; //0 - 494 - 140,150,0
      ppf("LedModEffects loop canvasData %s\n", canvasData);

      if (!fix->layers.empty()) {
        uint8_t rowNr = 0;
        //if more then one effect
        fix->layers[rowNr]->fadeToBlackBy();

        char * token = strtok((char *)canvasData, ":");
        const bool isStart = strncmp(token, "start", 6) == 0;
        const bool isEnd = strncmp(token, "end", 4) == 0;

        Coord3D midCoord{}; //placeHolder for mid

        Coord3D *newCoord = isStart? &fix->layers[rowNr]->start: isEnd? &fix->layers[rowNr]->end : &midCoord;

        if (newCoord) {
          token = strtok(nullptr, ",");
          if (token != nullptr) newCoord->x = strtol(token, nullptr, 10) / fix->ledFactor; else newCoord->x = 0; //should never happen
          token = strtok(nullptr, ",");
          if (token != nullptr) newCoord->y = strtol(token, nullptr, 10) / fix->ledFactor; else newCoord->y = 0;
          token = strtok(nullptr, ",");
          if (token != nullptr) newCoord->z = strtol(token, nullptr, 10) / fix->ledFactor; else newCoord->z = 0;

          mdl->setValue("layers", isStart?"start":isEnd?"end":"middle", *newCoord, 0); //assuming row 0 for the moment

          fix->layers[rowNr]->triggerMapping();
        }

        varSystem.remove("canvasData"); //convasdata has been processed
      }
    }

  } //loop

  void LedModEffects::initEffect(LedsLayer &leds, uint8_t rowNr) {
      ESP_LOGD("", "leds[%d] effect:%s a:%d (%d,%d,%d)", rowNr, leds.effect->name(), leds.effectData.bytesAllocated, leds.size.x, leds.size.y, leds.size.z);

      leds.effectData.clear(); //delete effectData memory so it can be rebuild
      if (strnstr(leds.effect->name(), "Live Effect", 11) == nullptr) //not needed for Live script, will hang instead
        leds.effect->loop(leds); leds.effectData.begin(); //do a loop to set effectData right to avoid control defaults to be removed

      Variable variable = Variable("layers", "effect");
      variable.preDetails();
      mdl->setValueRowNr = rowNr;
      leds.effect->setup(leds, variable); //if changed then run setup once (like call==0 in WLED) and set all defaults in effectData
      variable.postDetails(rowNr);
      mdl->setValueRowNr = UINT8_MAX;

      leds.effectData.alertIfChanged = true; //find out when it is changing, eg when projections change, in that case controls are lost...solution needed for that...


  }
