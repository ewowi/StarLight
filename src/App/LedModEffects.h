/*
   @title     StarLight
   @file      LedModEffects.h
   @date      20240819
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModule.h"

#include "LedFixture.h"
#include "LedEffects.h"
#include "LedProjections.h"

#ifdef STARLIGHT_CLOCKLESS_LED_DRIVER
  #if CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2
    #include "I2SClockLessLedDriveresp32s3.h"
  #else
    #include "I2SClocklessLedDriver.h"
  #endif
#endif


// #define FASTLED_RGBW

//https://www.partsnotincluded.com/fastled-rgbw-neopixels-sk6812/
inline unsigned16 getRGBWsize(unsigned16 nleds){
	unsigned16 nbytes = nleds * 4;
	if(nbytes % 3 > 0) return nbytes / 3 + 1;
	else return nbytes / 3;
}

//https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino
//https://blog.ja-ke.tech/2019/06/02/neopixel-performance.html

class LedModEffects:public SysModule {

public:
  bool newFrame = false; //for other modules (DDP)
  unsigned long frameCounter = 0;

  unsigned16 fps = 60;
  unsigned long lastMappingMillis = 0;

  std::vector<Effect *> effects;

  Fixture fixture = Fixture();

  bool fShow = true;

  #ifdef STARLIGHT_CLOCKLESS_LED_DRIVER
    #if CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2
      I2SClocklessLedDriveresp32S3 driver;
    #else
      I2SClocklessLedDriver driver;
    #endif
  #endif

  LedModEffects() :SysModule("Effects") {

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
      effects.push_back(new WaverlyEffect);
    #endif
    //3D
    effects.push_back(new RipplesEffect);
    effects.push_back(new RubiksCubeEffect);
    effects.push_back(new SphereMoveEffect);
    effects.push_back(new PixelMapEffect);
    effects.push_back(new MarioTestEffect);

    #ifdef STARBASE_USERMOD_LIVE
      effects.push_back(new LiveScriptEffect);
    #endif

    //load projections
    fixture.projections.push_back(new NoneProjection);
    fixture.projections.push_back(new DefaultProjection);
    fixture.projections.push_back(new PinwheelProjection);
    fixture.projections.push_back(new MultiplyProjection);
    fixture.projections.push_back(new TiltPanRollProjection);
    fixture.projections.push_back(new DistanceFromPointProjection);
    fixture.projections.push_back(new Preset1Projection);
    fixture.projections.push_back(new RandomProjection);
    fixture.projections.push_back(new ReverseProjection);
    fixture.projections.push_back(new MirrorProjection);
    fixture.projections.push_back(new GroupingProjection);
    fixture.projections.push_back(new SpacingProjection);
    fixture.projections.push_back(new TransposeProjection);
    // fixture.projections.push_back(new KaleidoscopeProjection);
    fixture.projections.push_back(new ScrollingProjection);
    fixture.projections.push_back(new AccelerationProjection);
    fixture.projections.push_back(new CheckerboardProjection);
    fixture.projections.push_back(new RotateProjection);

    #ifdef STARLIGHT_CLOCKLESS_LED_DRIVER
      #if !(CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2)
        driver.total_leds = 0;
      #endif
    #endif
  };

  void setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name, 1201);

    JsonObject currentVar;

    JsonObject tableVar = ui->initTable(parentVar, "layers", nullptr, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setComment(var, "List of effects");
        return true;
      case onAdd: {
        rowNr = fixture.layers.size();
        // ppf("layers onAdd %s[%d]\n", Variable(var).id(), rowNr);

        web->getResponseObject()["onAdd"]["rowNr"] = rowNr; //also done in callVarFun??

        if (rowNr >= fixture.layers.size()) {
          ppf("layers creating new LedsLayer instance %d\n", rowNr);
          LedsLayer *leds = new LedsLayer(fixture);
          fixture.layers.push_back(leds);
        }
        return true; }
      case onDelete: {
        // ppf("layers onDelete %s[%d]\n", Variable(var).id(), rowNr);
        //tbd: fade to black
        if (rowNr <fixture.layers.size()) {
          LedsLayer *leds = fixture.layers[rowNr];
          fixture.layers.erase(fixture.layers.begin() + rowNr); //remove from vector
          delete leds; //remove leds itself

        }
        return true; }
      default: return false;
    }});

    currentVar = ui->initSelect(tableVar, "effect", 0, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onSetValue:
        for (forUnsigned8 rowNr = 0; rowNr < fixture.layers.size(); rowNr++)
          mdl->setValue(var, fixture.layers[rowNr]->fx, rowNr);
        return true;
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

        if (rowNr == UINT8_MAX) rowNr = 0; // in case fx without a rowNr

        //create a new leds instance if a new row is created
        if (rowNr >= fixture.layers.size()) {
          ppf("layers fx[%d] onChange %d %s\n", rowNr, fixture.layers.size(), Variable(mdl->findVar("layers", "effect")).valueString().c_str());
          ppf("fx creating new LedsLayer instance %d\n", rowNr);
          LedsLayer *leds = new LedsLayer(fixture);
          fixture.layers.push_back(leds);
        }

        if (rowNr < fixture.layers.size()) {
          LedsLayer *leds = fixture.layers[rowNr];

          // leds->doMap = true; //stop the effects loop already here

          #ifdef STARBASE_USERMOD_LIVE
            //kill live script of moving to other effect
            if (leds->fx < effects.size()) {
              Effect* effect = effects[leds->fx];
              if (strncmp(effect->name(), "Live Script", 12) == 0) {
                liveM->kill();
              }
            }
          #endif

          leds->fx = mdl->getValue(var, rowNr);

          if (leds->fx < effects.size()) {
            ppf("setEffect fx[%d]: %d\n", rowNr, leds->fx);

            Effect* effect = effects[leds->fx];

            if (effect->dim() != leds->effectDimension) {
              leds->effectDimension = effect->dim();
              leds->triggerMapping();
            }

            ppf("initEffect leds[%d] fx:%d a:%d\n", rowNr, leds->fx, leds->effectData.bytesAllocated);

            leds->effectData.clear(); //delete effectData memory so it can be rebuild
            effect->loop(*leds); leds->effectData.begin(); //do a loop to set effectData right

            Variable(var).preDetails();
            mdl->setValueRowNr = rowNr;
            effect->controls(*leds, var); //set all defaults in effectData
            Variable(var).postDetails(rowNr);
            mdl->setValueRowNr = UINT8_MAX;

            leds->effectData.alertIfChanged = true; //find out when it is changing, eg when projections change, in that case controls are lost...solution needed for that...

            effect->setup(*leds); //if changed then run setup once (like call==0 in WLED)

          } // fx < size

        }
        return true;
      default: return false;
    }});
    currentVar["dash"] = true;

    //projection, default projection is 'default'
    currentVar = ui->initSelect(tableVar, "projection", 1, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onSetValue:
        for (forUnsigned8 rowNr = 0; rowNr < fixture.layers.size(); rowNr++)
          mdl->setValue(var, fixture.layers[rowNr]->projectionNr, rowNr);
        return true;
      case onUI: {
        ui->setComment(var, "How to project fx");

        JsonArray options = ui->setOptions(var);
        for (Projection *projection:fixture.projections) {
          char buf[32] = "";
          strlcat(buf, projection->name(), sizeof(buf));
          // strlcat(buf, projection->dim()==_1D?" ┊":projection->dim()==_2D?" ▦":" 🧊");
          strlcat(buf, " ", sizeof(buf));
          strlcat(buf, projection->tags(), sizeof(buf));
          options.add(JsonString(buf, JsonString::Copied)); //copy!
        }
        return true; }
      case onChange:

        if (rowNr == UINT8_MAX) rowNr = 0; // in case fx without a rowNr

        if (rowNr < fixture.layers.size()) {
          LedsLayer *leds = fixture.layers[rowNr];

          // leds->doMap = true; //stop the effects loop already here

          stackUnsigned8 proValue = mdl->getValue(var, rowNr);
          leds->projectionNr = proValue;
          
          if (proValue < fixture.projections.size()) {
            Projection* projection = fixture.projections[proValue];

            //setting cached virtual class methods! (By chatGPT so no source and don't understand how it works - scary!)
            //   (don't know how it works as it is not refering to derived classes, just to the base class but later it calls the derived class method)
            leds->setupCached = &Projection::setup;
            leds->adjustXYZCached = &Projection::adjustXYZ;

            //initProjection

            ppf("initProjection leds[%d] fx:%d a:%d\n", rowNr, leds->fx, leds->projectionData.bytesAllocated);

            leds->projectionData.clear(); //delete effectData memory so it can be rebuild

            Variable(var).preDetails(); //set all positive var N orders to negative
            mdl->setValueRowNr = rowNr;
            projection->controls(*leds, var);
            Variable(var).postDetails(rowNr);
            mdl->setValueRowNr = UINT8_MAX;

            leds->projectionData.alertIfChanged = true; //find out when it is changing, eg when projections change, in that case controls are lost...solution needed for that...
          }
          // ppf("onChange pro[%d] <- %d (%d)\n", rowNr, proValue, fixture.layers.size());

          leds->triggerMapping(); //set also fixture.doMap
        }
        return true;
      default: return false;
    }});
    currentVar["dash"] = true;

    ui->initCoord3D(tableVar, "start", {0,0,0}, 0, NUM_LEDS_Max, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onSetValue:
        for (forUnsigned8 rowNr = 0; rowNr < fixture.layers.size(); rowNr++) {
          ppf("ledsStart[%d] onSetValue %d,%d,%d\n", rowNr, fixture.layers[rowNr]->startPos.x, fixture.layers[rowNr]->startPos.y, fixture.layers[rowNr]->startPos.z);
          mdl->setValue(var, fixture.layers[rowNr]->startPos, rowNr);
        }
        return true;
      case onUI:
        ui->setComment(var, "In pixels");
        return true;
      case onChange:
        if (rowNr < fixture.layers.size()) {
          fixture.layers[rowNr]->startPos = mdl->getValue(var, rowNr).as<Coord3D>();

          ppf("ledsStart[%d] onChange %d,%d,%d\n", rowNr, fixture.layers[rowNr]->startPos.x, fixture.layers[rowNr]->startPos.y, fixture.layers[rowNr]->startPos.z);

          fixture.layers[rowNr]->fadeToBlackBy();
          fixture.layers[rowNr]->triggerMapping();
        }
        else {
          ppf("ledsStart[%d] onChange rownr not in range > %d\n", rowNr, fixture.layers.size());
        }
        return true;
      default: return false;
    }});

    ui->initCoord3D(tableVar, "middle", {0,0,0}, 0, NUM_LEDS_Max, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onSetValue:
        for (forUnsigned8 rowNr = 0; rowNr < fixture.layers.size(); rowNr++) {
          ppf("ledsMid[%d] onSetValue %d,%d,%d\n", rowNr, fixture.layers[rowNr]->midPos.x, fixture.layers[rowNr]->midPos.y, fixture.layers[rowNr]->midPos.z);
          mdl->setValue(var, fixture.layers[rowNr]->midPos, rowNr);
        }
        return true;
      case onUI:
        ui->setComment(var, "In pixels");
        return true;
      case onChange:
        if (rowNr < fixture.layers.size()) {
          fixture.layers[rowNr]->midPos = mdl->getValue(var, rowNr).as<Coord3D>();

          ppf("ledsMid[%d] onChange %d,%d,%d\n", rowNr, fixture.layers[rowNr]->midPos.x, fixture.layers[rowNr]->midPos.y, fixture.layers[rowNr]->midPos.z);

          fixture.layers[rowNr]->fadeToBlackBy();
          fixture.layers[rowNr]->triggerMapping();
        }
        else {
          ppf("ledsMid[%d] onChange rownr not in range > %d\n", rowNr, fixture.layers.size());
        }
        return true;
      default: return false;
    }});

    ui->initCoord3D(tableVar, "end", {8,8,0}, 0, NUM_LEDS_Max, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onSetValue:
        for (forUnsigned8 rowNr = 0; rowNr < fixture.layers.size(); rowNr++) {
          ppf("ledsEnd[%d] onSetValue %d,%d,%d\n", rowNr, fixture.layers[rowNr]->endPos.x, fixture.layers[rowNr]->endPos.y, fixture.layers[rowNr]->endPos.z);
          mdl->setValue(var, fixture.layers[rowNr]->endPos, rowNr);
        }
        return true;
      case onUI:
        ui->setComment(var, "In pixels");
        return true;
      case onChange:
        if (rowNr < fixture.layers.size()) {
          fixture.layers[rowNr]->endPos = mdl->getValue(var, rowNr).as<Coord3D>();

          ppf("ledsEnd[%d] onChange %d,%d,%d\n", rowNr, fixture.layers[rowNr]->endPos.x, fixture.layers[rowNr]->endPos.y, fixture.layers[rowNr]->endPos.z);

          fixture.layers[rowNr]->fadeToBlackBy();
          fixture.layers[rowNr]->triggerMapping();
        }
        else {
          ppf("ledsEnd[%d] onChange rownr not in range > %d\n", rowNr, fixture.layers.size());
        }
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "size", nullptr, 32, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onSetValue: {
        // for (std::vector<LedsLayer *>::iterator leds=fixture.layers.begin(); leds!=fixture.layers.end(); ++leds) {
        stackUnsigned8 rowNr = 0;
        for (LedsLayer *leds:fixture.layers) {
          char message[32];
          print->fFormat(message, sizeof(message)-1, "%d x %d x %d -> %d", leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds);
          ppf("onSetValue ledsSize[%d] = %s\n", rowNr, message);
          mdl->setValue(var, JsonString(message, JsonString::Copied), rowNr); //rowNr
          rowNr++;
        }
        return true; }
      default: return false;
    }});

    // ui->initSelect(parentVar, "layout", 0, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
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
    // }}); //fxLayout

    ui->initSlider(parentVar, "Blending", &fixture.globalBlend);

    #ifdef STARBASE_USERMOD_E131
      // if (e131mod->isEnabled) {
          e131mod->patchChannel(0, "Fixture", "bri", 255); //should be 256??
          e131mod->patchChannel(1, "layers", "effect", effects.size());
          e131mod->patchChannel(2, "effect", "pal", 8); //tbd: calculate nr of palettes (from select)
          // //add these temporary to test remote changing of this values do not crash the system
          // e131mod->patchChannel(3, "projection", Projections::count);
          // e131mod->patchChannel(4, "fixture", 5); //assuming 5!!!

          // ui->dashVarChanged = true;
          // //rebuild the table
          for (JsonObject childVar: Variable(mdl->findVar("E131", "e131Tbl")).children())
            ui->callVarFun(childVar, UINT8_MAX, onSetValue); //set the value (WIP)

      // }
      // else
      //   ppf("e131 not enabled\n");
    #endif

    #ifdef STARLIGHT_CLOCKLESS_LED_DRIVER
      fixture.setMaxPowerBrightness = 30;
    #else
      FastLED.setMaxPowerInMilliWatts(10000); // 5v, 2000mA
    #endif

    //for use in loop
    varSystem = mdl->findVar("m", "System");
    viewRot = mdl->getValue("pview", "viewRot");
  }

  //this loop is run as often as possible so coding should also be as efficient as possible (no findVars etc)
  void loop() {
    // SysModule::loop();

    random16_set_seed(sys->now);

    //set new frame
    if (sys->now - frameMillis >= 1000.0/fps) {

      //reset pixelsToBlend if multiple leds effects
      // ppf(" %d-%d", fixture.pixelsToBlend.size(), fixture.nrOfLeds);
      if (fixture.layers.size()) //if more then one effect
        for (uint16_t indexP=0; indexP < fixture.pixelsToBlend.size(); indexP++)
          fixture.pixelsToBlend[indexP] = false;

      frameMillis = sys->now;

      newFrame = true;

      //for each programmed effect
      //  run the next frame of the effect
      stackUnsigned8 rowNr = 0;
      for (LedsLayer *leds: fixture.layers) {
        if (leds->fx < effects.size()) { // don't run effect while remapping or non existing effect (default UINT16_MAX)
          // ppf(" %d %d,%d,%d - %d,%d,%d (%d,%d,%d)", leds->fx, leds->startPos.x, leds->startPos.y, leds->startPos.z, leds->endPos.x, leds->endPos.y, leds->endPos.z, leds->size.x, leds->size.y, leds->size.z );
          mdl->getValueRowNr = rowNr++;

          leds->effectData.begin(); //sets the effectData pointer back to 0 so loop effect can go through it
          effects[leds->fx]->loop(*leds);

          mdl->getValueRowNr = UINT8_MAX;
          // if (leds->projectionNr == p_TiltPanRoll || leds->projectionNr == p_Preset1)
          //   leds->fadeToBlackBy(50);

          //loop over mapped pixels and set pixelsToBlend to true
          if (fixture.layers.size()) { //if more then one effect
            for (std::vector<uint16_t> mappingTableIndex: leds->mappingTableIndexes) {
              for (uint16_t indexP: mappingTableIndex)
                fixture.pixelsToBlend[indexP] = true;
            }
            for (PhysMap physMap: leds->mappingTable) {
              if (physMap.mapType == m_onePixel)
                fixture.pixelsToBlend[physMap.indexP] = true;
            }
          }
        }
      }

      #ifdef STARLIGHT_USERMOD_AUDIOSYNC

        if (viewRot == 4) {
          fixture.head.x = audioSync->fftResults[3];
          fixture.head.y = audioSync->fftResults[8];
          fixture.head.z = audioSync->fftResults[13];
        }

      #endif

      if (fShow) {
        #ifdef STARLIGHT_CLOCKLESS_LED_DRIVER
          #if CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2
            if (driver.ledsbuff != NULL)
              driver.show();
          #else
            if (driver.total_leds > 0)
              driver.showPixels(WAIT);
          #endif
        #else
          FastLED.show();
        #endif
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
      if (fixture.layers.size()) { //if more then one effect
        fixture.layers[rowNr]->fadeToBlackBy();

        char * token = strtok((char *)canvasData, ":");
        bool isStart = strncmp(token, "start", 6) == 0;
        bool isEnd = strncmp(token, "end", 4) == 0;

        Coord3D midCoord; //placeHolder for mid

        Coord3D *newCoord = isStart? &fixture.layers[rowNr]->startPos: isEnd? &fixture.layers[rowNr]->endPos : &midCoord;

        if (newCoord) {
          token = strtok(NULL, ",");
          if (token != NULL) newCoord->x = atoi(token) / 10; else newCoord->x = 0; //should never happen
          token = strtok(NULL, ",");
          if (token != NULL) newCoord->y = atoi(token) / 10; else newCoord->y = 0;
          token = strtok(NULL, ",");
          if (token != NULL) newCoord->z = atoi(token) / 10; else newCoord->z = 0;

          mdl->setValue("layers", isStart?"start":isEnd?"end":"middle", *newCoord, 0); //assuming row 0 for the moment

          fixture.layers[rowNr]->triggerMapping();
        }

        varSystem.remove("canvasData"); //convasdata has been processed
      }
    }

    //run projectAndMap
    if (sys->now - lastMappingMillis >= 1000 && fixture.doMap) { //not more then once per second (for E131)
      lastMappingMillis = sys->now;
      fixture.projectAndMap();

      //https://github.com/FastLED/FastLED/wiki/Multiple-Controller-Examples

      //connect allocated Pins to gpio

      if (fixture.doAllocPins) {
        unsigned pinNr = 0;

        #ifdef STARLIGHT_CLOCKLESS_LED_DRIVER
          int pinAssignment[16]; //max 16 pins
          int lengths[16]; //max 16 pins
          int nb_pins=0;
        #endif
        for (PinObject &pinObject: pinsM->pinObjects) {

          if (pinsM->isOwner(pinNr, "Leds")) { //if pin owned by leds, (assigned in projectAndMap)
            //dirty trick to decode nrOfLedsPerPin
            char details[32];
            strlcpy(details, pinObject.details, sizeof(details)); //copy as strtok messes with the string
            char * after = strtok((char *)details, "-");
            if (after != NULL ) {
              char * before;
              before = after;
              after = strtok(NULL, " ");

              stackUnsigned16 startLed = atoi(before);
              stackUnsigned16 nrOfLeds = atoi(after) - atoi(before) + 1;
              ppf("addLeds new %d: %d-%d\n", pinNr, startLed, nrOfLeds-1);

              #ifdef STARLIGHT_CLOCKLESS_LED_DRIVER
                pinAssignment[nb_pins] = pinNr;
                lengths[nb_pins] = nrOfLeds;
                nb_pins++;
              #else
              //commented pins: error: static assertion failed: Invalid pin specified
              switch (pinNr) {
                #if CONFIG_IDF_TARGET_ESP32
                  case 0: FastLED.addLeds<STARLIGHT_CHIPSET, 0>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 1: FastLED.addLeds<STARLIGHT_CHIPSET, 1>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 2: FastLED.addLeds<STARLIGHT_CHIPSET, 2>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 3: FastLED.addLeds<STARLIGHT_CHIPSET, 3>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 4: FastLED.addLeds<STARLIGHT_CHIPSET, 4>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 5: FastLED.addLeds<STARLIGHT_CHIPSET, 5>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 6: FastLED.addLeds<STARLIGHT_CHIPSET, 6>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 7: FastLED.addLeds<STARLIGHT_CHIPSET, 7>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 8: FastLED.addLeds<STARLIGHT_CHIPSET, 8>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 9: FastLED.addLeds<STARLIGHT_CHIPSET, 9>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 10: FastLED.addLeds<STARLIGHT_CHIPSET, 10>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 11: FastLED.addLeds<STARLIGHT_CHIPSET, 11>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 12: FastLED.addLeds<STARLIGHT_CHIPSET, 12>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 13: FastLED.addLeds<STARLIGHT_CHIPSET, 13>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 14: FastLED.addLeds<STARLIGHT_CHIPSET, 14>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 15: FastLED.addLeds<STARLIGHT_CHIPSET, 15>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #if !defined(BOARD_HAS_PSRAM) && !defined(ARDUINO_ESP32_PICO)
                  // 16+17 = reserved for PSRAM, or reserved for FLASH on pico-D4
                  case 16: FastLED.addLeds<STARLIGHT_CHIPSET, 16>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 17: FastLED.addLeds<STARLIGHT_CHIPSET, 17>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #endif
                  case 18: FastLED.addLeds<STARLIGHT_CHIPSET, 18>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 19: FastLED.addLeds<STARLIGHT_CHIPSET, 19>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 20: FastLED.addLeds<STARLIGHT_CHIPSET, 20>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 21: FastLED.addLeds<STARLIGHT_CHIPSET, 21>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 22: FastLED.addLeds<STARLIGHT_CHIPSET, 22>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 23: FastLED.addLeds<STARLIGHT_CHIPSET, 23>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 24: FastLED.addLeds<STARLIGHT_CHIPSET, 24>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 25: FastLED.addLeds<STARLIGHT_CHIPSET, 25>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 26: FastLED.addLeds<STARLIGHT_CHIPSET, 26>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 27: FastLED.addLeds<STARLIGHT_CHIPSET, 27>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 28: FastLED.addLeds<STARLIGHT_CHIPSET, 28>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 29: FastLED.addLeds<STARLIGHT_CHIPSET, 29>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 30: FastLED.addLeds<STARLIGHT_CHIPSET, 30>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 31: FastLED.addLeds<STARLIGHT_CHIPSET, 31>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 32: FastLED.addLeds<STARLIGHT_CHIPSET, 32>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 33: FastLED.addLeds<STARLIGHT_CHIPSET, 33>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // 34-39 input-only
                  // case 34: FastLED.addLeds<STARLIGHT_CHIPSET, 34>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 35: FastLED.addLeds<STARLIGHT_CHIPSET, 35>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 36: FastLED.addLeds<STARLIGHT_CHIPSET, 36>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 37: FastLED.addLeds<STARLIGHT_CHIPSET, 37>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 38: FastLED.addLeds<STARLIGHT_CHIPSET, 38>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 39: FastLED.addLeds<STARLIGHT_CHIPSET, 39>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                #endif //CONFIG_IDF_TARGET_ESP32

                #if CONFIG_IDF_TARGET_ESP32S2
                  case 0: FastLED.addLeds<STARLIGHT_CHIPSET, 0>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 1: FastLED.addLeds<STARLIGHT_CHIPSET, 1>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 2: FastLED.addLeds<STARLIGHT_CHIPSET, 2>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 3: FastLED.addLeds<STARLIGHT_CHIPSET, 3>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 4: FastLED.addLeds<STARLIGHT_CHIPSET, 4>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 5: FastLED.addLeds<STARLIGHT_CHIPSET, 5>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 6: FastLED.addLeds<STARLIGHT_CHIPSET, 6>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 7: FastLED.addLeds<STARLIGHT_CHIPSET, 7>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 8: FastLED.addLeds<STARLIGHT_CHIPSET, 8>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 9: FastLED.addLeds<STARLIGHT_CHIPSET, 9>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 10: FastLED.addLeds<STARLIGHT_CHIPSET, 10>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 11: FastLED.addLeds<STARLIGHT_CHIPSET, 11>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 12: FastLED.addLeds<STARLIGHT_CHIPSET, 12>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 13: FastLED.addLeds<STARLIGHT_CHIPSET, 13>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 14: FastLED.addLeds<STARLIGHT_CHIPSET, 14>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 15: FastLED.addLeds<STARLIGHT_CHIPSET, 15>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 16: FastLED.addLeds<STARLIGHT_CHIPSET, 16>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 17: FastLED.addLeds<STARLIGHT_CHIPSET, 17>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 18: FastLED.addLeds<STARLIGHT_CHIPSET, 18>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #if !ARDUINO_USB_CDC_ON_BOOT
                  // 19 + 20 = USB HWCDC. reserved for USB port when ARDUINO_USB_CDC_ON_BOOT=1
                  case 19: FastLED.addLeds<STARLIGHT_CHIPSET, 19>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 20: FastLED.addLeds<STARLIGHT_CHIPSET, 20>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #endif
                  case 21: FastLED.addLeds<STARLIGHT_CHIPSET, 21>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // 22 to 32: not connected, or reserved for SPI FLASH
                  // case 22: FastLED.addLeds<STARLIGHT_CHIPSET, 22>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 23: FastLED.addLeds<STARLIGHT_CHIPSET, 23>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 24: FastLED.addLeds<STARLIGHT_CHIPSET, 24>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 25: FastLED.addLeds<STARLIGHT_CHIPSET, 25>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #if !defined(BOARD_HAS_PSRAM)
                  // 26-32 = reserved for PSRAM
                  case 26: FastLED.addLeds<STARLIGHT_CHIPSET, 26>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 27: FastLED.addLeds<STARLIGHT_CHIPSET, 27>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 28: FastLED.addLeds<STARLIGHT_CHIPSET, 28>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 29: FastLED.addLeds<STARLIGHT_CHIPSET, 29>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 30: FastLED.addLeds<STARLIGHT_CHIPSET, 30>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 31: FastLED.addLeds<STARLIGHT_CHIPSET, 31>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 32: FastLED.addLeds<STARLIGHT_CHIPSET, 32>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #endif
                  case 33: FastLED.addLeds<STARLIGHT_CHIPSET, 33>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 34: FastLED.addLeds<STARLIGHT_CHIPSET, 34>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 35: FastLED.addLeds<STARLIGHT_CHIPSET, 35>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 36: FastLED.addLeds<STARLIGHT_CHIPSET, 36>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 37: FastLED.addLeds<STARLIGHT_CHIPSET, 37>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 38: FastLED.addLeds<STARLIGHT_CHIPSET, 38>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 39: FastLED.addLeds<STARLIGHT_CHIPSET, 39>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 40: FastLED.addLeds<STARLIGHT_CHIPSET, 40>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 41: FastLED.addLeds<STARLIGHT_CHIPSET, 41>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 42: FastLED.addLeds<STARLIGHT_CHIPSET, 42>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 43: FastLED.addLeds<STARLIGHT_CHIPSET, 43>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 44: FastLED.addLeds<STARLIGHT_CHIPSET, 44>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 45: FastLED.addLeds<STARLIGHT_CHIPSET, 45>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // 46 input-only
                  // case 46: FastLED.addLeds<STARLIGHT_CHIPSET, 46>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                #endif //CONFIG_IDF_TARGET_ESP32S2

                #if CONFIG_IDF_TARGET_ESP32C3
                  case 0: FastLED.addLeds<STARLIGHT_CHIPSET, 0>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 1: FastLED.addLeds<STARLIGHT_CHIPSET, 1>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 2: FastLED.addLeds<STARLIGHT_CHIPSET, 2>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 3: FastLED.addLeds<STARLIGHT_CHIPSET, 3>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 4: FastLED.addLeds<STARLIGHT_CHIPSET, 4>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 5: FastLED.addLeds<STARLIGHT_CHIPSET, 5>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 6: FastLED.addLeds<STARLIGHT_CHIPSET, 6>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 7: FastLED.addLeds<STARLIGHT_CHIPSET, 7>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 8: FastLED.addLeds<STARLIGHT_CHIPSET, 8>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 9: FastLED.addLeds<STARLIGHT_CHIPSET, 9>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 10: FastLED.addLeds<STARLIGHT_CHIPSET, 10>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // 11-17 reserved for SPI FLASH
                  //case 11: FastLED.addLeds<STARLIGHT_CHIPSET, 11>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  //case 12: FastLED.addLeds<STARLIGHT_CHIPSET, 12>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  //case 13: FastLED.addLeds<STARLIGHT_CHIPSET, 13>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  //case 14: FastLED.addLeds<STARLIGHT_CHIPSET, 14>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  //case 15: FastLED.addLeds<STARLIGHT_CHIPSET, 15>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  //case 16: FastLED.addLeds<STARLIGHT_CHIPSET, 16>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  //case 17: FastLED.addLeds<STARLIGHT_CHIPSET, 17>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #if !ARDUINO_USB_CDC_ON_BOOT
                  // 18 + 19 = USB HWCDC. reserved for USB port when ARDUINO_USB_CDC_ON_BOOT=1
                  case 18: FastLED.addLeds<STARLIGHT_CHIPSET, 18>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 19: FastLED.addLeds<STARLIGHT_CHIPSET, 19>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #endif
                  // 20+21 = Serial RX+TX --> don't use for LEDS when serial-to-USB is needed
                  case 20: FastLED.addLeds<STARLIGHT_CHIPSET, 20>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 21: FastLED.addLeds<STARLIGHT_CHIPSET, 21>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                #endif //CONFIG_IDF_TARGET_ESP32S2

                #if CONFIG_IDF_TARGET_ESP32S3
                  case 0: FastLED.addLeds<STARLIGHT_CHIPSET, 0>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 1: FastLED.addLeds<STARLIGHT_CHIPSET, 1>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 2: FastLED.addLeds<STARLIGHT_CHIPSET, 2>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 3: FastLED.addLeds<STARLIGHT_CHIPSET, 3>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 4: FastLED.addLeds<STARLIGHT_CHIPSET, 4>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 5: FastLED.addLeds<STARLIGHT_CHIPSET, 5>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 6: FastLED.addLeds<STARLIGHT_CHIPSET, 6>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 7: FastLED.addLeds<STARLIGHT_CHIPSET, 7>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 8: FastLED.addLeds<STARLIGHT_CHIPSET, 8>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 9: FastLED.addLeds<STARLIGHT_CHIPSET, 9>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 10: FastLED.addLeds<STARLIGHT_CHIPSET, 10>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 11: FastLED.addLeds<STARLIGHT_CHIPSET, 11>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 12: FastLED.addLeds<STARLIGHT_CHIPSET, 12>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 13: FastLED.addLeds<STARLIGHT_CHIPSET, 13>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 14: FastLED.addLeds<STARLIGHT_CHIPSET, 14>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 15: FastLED.addLeds<STARLIGHT_CHIPSET, 15>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 16: FastLED.addLeds<STARLIGHT_CHIPSET, 16>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 17: FastLED.addLeds<STARLIGHT_CHIPSET, 17>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 18: FastLED.addLeds<STARLIGHT_CHIPSET, 18>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #if !ARDUINO_USB_CDC_ON_BOOT
                  // 19 + 20 = USB-JTAG. Not recommended for other uses.
                  case 19: FastLED.addLeds<STARLIGHT_CHIPSET, 19>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 20: FastLED.addLeds<STARLIGHT_CHIPSET, 20>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #endif
                  case 21: FastLED.addLeds<STARLIGHT_CHIPSET, 21>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // // 22 to 32: not connected, or SPI FLASH
                  // case 22: FastLED.addLeds<STARLIGHT_CHIPSET, 22>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 23: FastLED.addLeds<STARLIGHT_CHIPSET, 23>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 24: FastLED.addLeds<STARLIGHT_CHIPSET, 24>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 25: FastLED.addLeds<STARLIGHT_CHIPSET, 25>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 26: FastLED.addLeds<STARLIGHT_CHIPSET, 26>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 27: FastLED.addLeds<STARLIGHT_CHIPSET, 27>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 28: FastLED.addLeds<STARLIGHT_CHIPSET, 28>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 29: FastLED.addLeds<STARLIGHT_CHIPSET, 29>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 30: FastLED.addLeds<STARLIGHT_CHIPSET, 30>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 31: FastLED.addLeds<STARLIGHT_CHIPSET, 31>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // case 32: FastLED.addLeds<STARLIGHT_CHIPSET, 32>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #if !defined(BOARD_HAS_PSRAM)
                  // 33 to 37: reserved if using _octal_ SPI Flash or _octal_ PSRAM
                  case 33: FastLED.addLeds<STARLIGHT_CHIPSET, 33>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 34: FastLED.addLeds<STARLIGHT_CHIPSET, 34>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 35: FastLED.addLeds<STARLIGHT_CHIPSET, 35>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 36: FastLED.addLeds<STARLIGHT_CHIPSET, 36>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 37: FastLED.addLeds<STARLIGHT_CHIPSET, 37>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
              #endif
                  case 38: FastLED.addLeds<STARLIGHT_CHIPSET, 38>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 39: FastLED.addLeds<STARLIGHT_CHIPSET, 39>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 40: FastLED.addLeds<STARLIGHT_CHIPSET, 40>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 41: FastLED.addLeds<STARLIGHT_CHIPSET, 41>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 42: FastLED.addLeds<STARLIGHT_CHIPSET, 42>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  // 43+44 = Serial RX+TX --> don't use for LEDS when serial-to-USB is needed
                  case 43: FastLED.addLeds<STARLIGHT_CHIPSET, 43>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 44: FastLED.addLeds<STARLIGHT_CHIPSET, 44>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 45: FastLED.addLeds<STARLIGHT_CHIPSET, 45>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 46: FastLED.addLeds<STARLIGHT_CHIPSET, 46>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 47: FastLED.addLeds<STARLIGHT_CHIPSET, 47>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                  case 48: FastLED.addLeds<STARLIGHT_CHIPSET, 48>(fixture.ledsP, startLed, nrOfLeds).setCorrection(TypicalLEDStrip); break;
                #endif //CONFIG_IDF_TARGET_ESP32S3

                default: ppf("FastLedPin assignment: pin not supported %d\n", pinNr);
              } //switch pinNr
              #endif //STARLIGHT_CLOCKLESS_LED_DRIVER
            } //if led range in details (- in details e.g. 0-1023)
          } //if pin owned by leds
          pinNr++;
        } // for pins
        #ifdef STARLIGHT_CLOCKLESS_LED_DRIVER
          if (nb_pins>0) {
            #if CONFIG_IDF_TARGET_ESP32S3 | CONFIG_IDF_TARGET_ESP32S2
              driver.initled((uint8_t*) fixture.ledsP, pinAssignment, nb_pins, lengths[0]); //s3 doesn't support lengths so we pick the first
              //void initled( uint8_t * leds, int * pins, int numstrip, int NUM_LED_PER_STRIP)
            #else
              driver.initled((uint8_t*) fixture.ledsP, pinAssignment, lengths, nb_pins, ORDER_GRB);
              //void initled(uint8_t *leds, int *Pinsq, int *sizes, int num_strips, colorarrangment cArr)
            #endif
            mdl->callVarOnChange(fix->bri, UINT8_MAX, true); //set brightness (init is true so bri value not send via udp)
            // driver.setBrightness(fixture.setMaxPowerBrightness / 256); //not brighter then the set limit (WIP)
          }
        #endif
        fixture.doAllocPins = false;
      }
    }
  } //loop

  // void loop10s() {
    // ppf("caching %u %u\n", trigoCached, trigoUnCached); //not working for some reason!!
    // trigoCached = 0;
    // trigoUnCached = 0;
  // }

private:
  unsigned long frameMillis = 0;
  JsonObject varSystem = JsonObject();
  uint8_t viewRot = UINT8_MAX;

};

extern LedModEffects *eff;