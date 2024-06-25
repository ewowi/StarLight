/*
   @title     StarLight
   @file      LedModEffects.h
   @date      20240228
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

  unsigned16 fps = 60;
  unsigned long lastMappingMillis = 0;

  std::vector<Effect *> effects;

  Fixture fixture = Fixture();

  bool fShow = true;

  LedModEffects() :SysModule("Effects") {

    //load effects

    //1D Basis
    effects.push_back(new SolidEffect);
    // 1D FastLed
    effects.push_back(new RainbowEffect);
    effects.push_back(new RainbowWithGlitterEffect);
    effects.push_back(new FlowEffect);
    effects.push_back(new SinelonEffect);
    effects.push_back(new ConfettiEffect);
    effects.push_back(new BPMEffect);
    effects.push_back(new JuggleEffect);
    //1D StarLight
    effects.push_back(new RunningEffect);
    effects.push_back(new RingRandomFlow);
    // 1D WLED
    effects.push_back(new BouncingBalls);
    effects.push_back(new RainEffect);
    effects.push_back(new DripEffect);
    effects.push_back(new HeartBeatEffect);

    #ifdef STARLIGHT_USERMOD_WLEDAUDIO
      //1D Volume
      effects.push_back(new FreqMatrix);
      effects.push_back(new PopCorn);
      effects.push_back(new NoiseMeter);
      //1D frequency
      effects.push_back(new AudioRings);
      effects.push_back(new DJLight);
    #endif

    //2D StarLight
    effects.push_back(new Lines);
    //2D WLED
    effects.push_back(new BlackHole);
    effects.push_back(new DNA);
    effects.push_back(new DistortionWaves);
    effects.push_back(new Octopus);
    effects.push_back(new Lissajous);
    effects.push_back(new Frizzles);
    effects.push_back(new ScrollingText);
    effects.push_back(new Noise2D);
    effects.push_back(new GameOfLife);
    effects.push_back(new RubiksCube);
    effects.push_back(new ParticleTest);
    #ifdef STARLIGHT_USERMOD_WLEDAUDIO
      //2D WLED
      effects.push_back(new Waverly);
      effects.push_back(new GEQEffect);
      effects.push_back(new FunkyPlank);
    #endif
    //3D
    effects.push_back(new RipplesEffect);
    effects.push_back(new SphereMoveEffect);

    //load projections
    fixture.projections.push_back(new NoneProjection);
    fixture.projections.push_back(new DefaultProjection);
    fixture.projections.push_back(new MultiplyProjection);
    fixture.projections.push_back(new TiltPanRollProjection);
    fixture.projections.push_back(new DistanceFromPointProjection);
    fixture.projections.push_back(new Preset1Projection);
    fixture.projections.push_back(new RandomProjection);
    fixture.projections.push_back(new ReverseProjection);
    fixture.projections.push_back(new MirrorProjection);
    fixture.projections.push_back(new GroupingProjection);
    fixture.projections.push_back(new SpacingProjection);
    fixture.projections.push_back(new KaleidoscopeProjection);
    fixture.projections.push_back(new PinwheelProjection);
  };

  void setup() {
    SysModule::setup();

    parentVar = ui->initAppMod(parentVar, name, 1201);

    JsonObject currentVar;

    JsonObject tableVar = ui->initTable(parentVar, "fxTbl", nullptr, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Effects");
        ui->setComment(var, "List of effects");
        return true;
      case onAddRow: {
        rowNr = fixture.listOfLeds.size();
        // ppf("chFun addRow %s[%d]\n", mdl->varID(var), rowNr);

        web->getResponseObject()["addRow"]["rowNr"] = rowNr;

        if (rowNr >= fixture.listOfLeds.size())
          fixture.listOfLeds.push_back(new Leds(fixture));
        return true; }
      case onDeleteRow: {
        // ppf("chFun delrow %s[%d]\n", mdl->varID(var), rowNr);
        //tbd: fade to black
        if (rowNr <fixture.listOfLeds.size()) {
          Leds *leds = fixture.listOfLeds[rowNr];
          fixture.listOfLeds.erase(fixture.listOfLeds.begin() + rowNr); //remove from vector
          delete leds; //remove leds itself
        }
        return true; }
      default: return false;
    }});

    currentVar = ui->initSelect(tableVar, "fx", 0, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onSetValue:
        for (forUnsigned8 rowNr = 0; rowNr < fixture.listOfLeds.size(); rowNr++)
          mdl->setValue(var, fixture.listOfLeds[rowNr]->fx, rowNr);
        return true;
      case onUI: {
        ui->setLabel(var, "Effect");
        ui->setComment(var, "Effect to show");
        JsonArray options = ui->setOptions(var);
        for (Effect *effect:effects) {
          char buf[32] = "";
          strcat(buf, effect->name());
          strcat(buf, effect->dim()==_1D?" ┊":effect->dim()==_2D?" ▦":" 🧊");
          strcat(buf, " ");
          strcat(buf, effect->tags());
          options.add(JsonString(buf, JsonString::Copied)); //copy!
        }
        return true; }
      case onChange:

        if (rowNr == UINT8_MAX) rowNr = 0; // in case fx without a rowNr

        //create a new leds instance if a new row is created
        if (rowNr >= fixture.listOfLeds.size()) {
          ppf("listOfLeds fx[%d] onChange %d %s\n", rowNr, fixture.listOfLeds.size(), mdl->findVar("fx")["value"].as<String>().c_str());
          fixture.listOfLeds.push_back(new Leds(fixture));
        }

        if (rowNr < fixture.listOfLeds.size()) {
          Leds *leds = fixture.listOfLeds[rowNr];

          leds->fx = mdl->getValue(var, rowNr);

          ppf("setEffect fx[%d]: %d\n", rowNr, leds->fx);

          if (leds->fx < effects.size()) {

            leds->sharedData.reset(); //make sure all values are 0 and reset for a fresh start of the effect

            Effect* effect = effects[leds->fx];

            // effect->loop(leds); //do a loop to set sharedData right
            // leds->sharedData.loop();
            mdl->varPreDetails(var, rowNr);
            effect->controls(*leds, var);
            mdl->varPostDetails(var, rowNr);

            effect->setup(*leds); //if changed then run setup once (like call==0 in WLED)

            ppf("control ");
            print->printVar(var);
            ppf("\n");

            if (effect->dim() != leds->effectDimension) {
              leds->effectDimension = effect->dim();
              leds->doMap = true;
              leds->fixture->doMap = true;
            }
          } // fx < size

        }
        return true;
      default: return false;
    }});
    currentVar["dash"] = true;

    //projection, default projection is 'default'
    currentVar = ui->initSelect(tableVar, "pro", 1, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onSetValue:
        for (forUnsigned8 rowNr = 0; rowNr < fixture.listOfLeds.size(); rowNr++)
          mdl->setValue(var, fixture.listOfLeds[rowNr]->projectionNr, rowNr);
        return true;
      case onUI: {
        ui->setLabel(var, "Projection");
        ui->setComment(var, "How to project fx");

        JsonArray options = ui->setOptions(var);
        for (Projection *projection:fixture.projections) {
          char buf[32] = "";
          strcat(buf, projection->name());
          // strcat(buf, projection->dim()==_1D?" ┊":projection->dim()==_2D?" ▦":" 🧊");
          strcat(buf, " ");
          strcat(buf, projection->tags());
          options.add(JsonString(buf, JsonString::Copied)); //copy!
        }
        return true; }
      case onChange:

        if (rowNr == UINT8_MAX) rowNr = 0; // in case fx without a rowNr

        if (rowNr < fixture.listOfLeds.size()) {
          Leds *leds = fixture.listOfLeds[rowNr];
          leds->doMap = true;

          stackUnsigned8 proValue = mdl->getValue(var, rowNr);
          leds->projectionNr = proValue;
          
          if (proValue < fixture.projections.size()) {
            Projection* projection = fixture.projections[proValue];

            mdl->varPreDetails(var, rowNr); //set all positive var N orders to negative
            projection->controls(*leds, var);
            mdl->varPostDetails(var, rowNr);
          }
          // ppf("chFun pro[%d] <- %d (%d)\n", rowNr, proValue, fixture.listOfLeds.size());

          fixture.doMap = true;
        }
        return true;
      default: return false;
    }});
    currentVar["dash"] = true;

    ui->initCoord3D(tableVar, "fxStart", {0,0,0}, 0, NUM_LEDS_Max, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onSetValue:
        for (forUnsigned8 rowNr = 0; rowNr < fixture.listOfLeds.size(); rowNr++) {
          ppf("fxStart[%d] onSetValue %d,%d,%d\n", rowNr, fixture.listOfLeds[rowNr]->startPos.x, fixture.listOfLeds[rowNr]->startPos.y, fixture.listOfLeds[rowNr]->startPos.z);
          mdl->setValue(var, fixture.listOfLeds[rowNr]->startPos, rowNr);
        }
        return true;
      case onUI:
        ui->setLabel(var, "Start");
        ui->setComment(var, "In pixels");
        return true;
      case onChange:
        if (rowNr < fixture.listOfLeds.size()) {
          fixture.listOfLeds[rowNr]->startPos = mdl->getValue(var, rowNr).as<Coord3D>();

          ppf("fxStart[%d] chFun %d,%d,%d\n", rowNr, fixture.listOfLeds[rowNr]->startPos.x, fixture.listOfLeds[rowNr]->startPos.y, fixture.listOfLeds[rowNr]->startPos.z);

          fixture.listOfLeds[rowNr]->fadeToBlackBy();
          fixture.listOfLeds[rowNr]->doMap = true;
          fixture.doMap = true;
        }
        else {
          ppf("fxStart[%d] chfun rownr not in range > %d\n", rowNr, fixture.listOfLeds.size());
        }
        return true;
      default: return false;
    }});

    ui->initCoord3D(tableVar, "fxEnd", {8,8,0}, 0, NUM_LEDS_Max, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onSetValue:
        for (forUnsigned8 rowNr = 0; rowNr < fixture.listOfLeds.size(); rowNr++) {
          ppf("fxEnd[%d] onSetValue %d,%d,%d\n", rowNr, fixture.listOfLeds[rowNr]->endPos.x, fixture.listOfLeds[rowNr]->endPos.y, fixture.listOfLeds[rowNr]->endPos.z);
          mdl->setValue(var, fixture.listOfLeds[rowNr]->endPos, rowNr);
        }
        return true;
      case onUI:
        ui->setLabel(var, "End");
        ui->setComment(var, "In pixels");
        return true;
      case onChange:
        if (rowNr < fixture.listOfLeds.size()) {
          fixture.listOfLeds[rowNr]->endPos = mdl->getValue(var, rowNr).as<Coord3D>();

          ppf("fxEnd[%d] chFun %d,%d,%d\n", rowNr, fixture.listOfLeds[rowNr]->endPos.x, fixture.listOfLeds[rowNr]->endPos.y, fixture.listOfLeds[rowNr]->endPos.z);

          fixture.listOfLeds[rowNr]->fadeToBlackBy();
          fixture.listOfLeds[rowNr]->doMap = true;
          fixture.doMap = true;
        }
        else {
          ppf("fxEnd[%d] chfun rownr not in range > %d\n", rowNr, fixture.listOfLeds.size());
        }
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "fxSize", nullptr, 32, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onSetValue: {
        // for (std::vector<Leds *>::iterator leds=fixture.listOfLeds.begin(); leds!=fixture.listOfLeds.end(); ++leds) {
        stackUnsigned8 rowNr = 0;
        for (Leds *leds:fixture.listOfLeds) {
          char message[32];
          print->fFormat(message, sizeof(message)-1, "%d x %d x %d -> %d", leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds);
          ppf("onSetValue fxSize[%d] = %s\n", rowNr, message);
          mdl->setValue(var, JsonString(message, JsonString::Copied), rowNr); //rowNr
          rowNr++;
        }
        return true; }
      case onUI:
        ui->setLabel(var, "Size");
        return true;
      default: return false;
    }});

    // ui->initSelect(parentVar, "fxLayout", 0, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    //   case onUI: {
    //     ui->setLabel(var, "Layout");
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
          e131mod->patchChannel(0, "bri", 255); //should be 256??
          e131mod->patchChannel(1, "fx", effects.size());
          e131mod->patchChannel(2, "pal", 8); //tbd: calculate nr of palettes (from select)
          // //add these temporary to test remote changing of this values do not crash the system
          // e131mod->patchChannel(3, "pro", Projections::count);
          // e131mod->patchChannel(4, "fixture", 5); //assuming 5!!!

          // ui->dashVarChanged = true;
          // //rebuild the table
          for (JsonObject childVar: mdl->varChildren("e131Tbl"))
            ui->callVarFun(childVar);

      // }
      // else
      //   ppf("Leds e131 not enabled\n");
    #endif

    FastLED.setMaxPowerInVoltsAndMilliamps(5,2000); // 5v, 2000mA
  }

  void loop() {
    // SysModule::loop();

    random16_set_seed(sys->now);

    //set new frame
    if (sys->now - frameMillis >= 1000.0/fps) {
      frameMillis = sys->now;

      newFrame = true;

      //for each programmed effect
      //  run the next frame of the effect
      stackUnsigned8 rowNr = 0;
      for (Leds *leds: fixture.listOfLeds) {
        if (!leds->doMap) { // don't run effect while remapping
          // ppf(" %d %d,%d,%d - %d,%d,%d (%d,%d,%d)", leds->fx, leds->startPos.x, leds->startPos.y, leds->startPos.z, leds->endPos.x, leds->endPos.y, leds->endPos.z, leds->size.x, leds->size.y, leds->size.z );
          mdl->getValueRowNr = rowNr++;

          leds->sharedData.loop(); //sets the sharedData pointer back to 0 so loop effect can go through it
          effects[leds->fx]->loop(*leds);

          mdl->getValueRowNr = UINT8_MAX;
          // if (leds->projectionNr == p_TiltPanRoll || leds->projectionNr == p_Preset1)
          //   leds->fadeToBlackBy(50);
        }
      }

      #ifdef STARLIGHT_USERMOD_WLEDAUDIO

        if (mdl->getValue("viewRot")  == 4) {
          fixture.head.x = wledAudioMod->fftResults[3];
          fixture.head.y = wledAudioMod->fftResults[8];
          fixture.head.z = wledAudioMod->fftResults[13];
        }

      #endif

      if (fShow)
        FastLED.show();

      frameCounter++;
    }
    else {
      newFrame = false;
    }

    JsonObject var = mdl->findVar("System");
    if (!var["canvasData"].isNull()) {
      const char * canvasData = var["canvasData"]; //0 - 494 - 140,150,0
      ppf("LedModEffects loop canvasData %s\n", canvasData);

      uint8_t rowNr = 0; //currently only leds[0] supported
      if (fixture.listOfLeds.size()) {
        fixture.listOfLeds[rowNr]->fadeToBlackBy();

        char * token = strtok((char *)canvasData, ":");
        bool isStart = strcmp(token, "start") == 0;
        bool isEnd = strcmp(token, "end") == 0;

        Coord3D midCoord; //placeHolder for mid

        Coord3D *newCoord = isStart? &fixture.listOfLeds[rowNr]->startPos: isEnd? &fixture.listOfLeds[rowNr]->endPos : &midCoord;

        if (newCoord) {
          token = strtok(NULL, ",");
          if (token != NULL) newCoord->x = atoi(token) / 10; else newCoord->x = 0; //should never happen
          token = strtok(NULL, ",");
          if (token != NULL) newCoord->y = atoi(token) / 10; else newCoord->y = 0;
          token = strtok(NULL, ",");
          if (token != NULL) newCoord->z = atoi(token) / 10; else newCoord->z = 0;

          mdl->setValue(isStart?"fxStart":isEnd?"fxEnd":"proCenter", *newCoord, 0); //assuming row 0 for the moment

          fixture.listOfLeds[rowNr]->doMap = true; //recalc projection
          fixture.doMap = true;
        }

        var.remove("canvasData"); //convasdata has been processed
      }
    }

    //update projection
    if (sys->now - lastMappingMillis >= 1000 && fixture.doMap) { //not more then once per second (for E131)
      lastMappingMillis = sys->now;
      fixture.projectAndMap();

      //https://github.com/FastLED/FastLED/wiki/Multiple-Controller-Examples

      //connect allocated Pins to gpio

      if (fixture.doAllocPins) {
        unsigned pinNr = 0;

        for (PinObject &pinObject: pins->pinObjects) {

          if (pins->isOwner(pinNr, "Leds")) { //if pin owned by leds, (assigned in projectAndMap)
            //dirty trick to decode nrOfLedsPerPin
            char details[32];
            strcpy(details, pinObject.details); //copy as strtok messes with the string
            char * after = strtok((char *)details, "-");
            if (after != NULL ) {
              char * before;
              before = after;
              after = strtok(NULL, " ");

              stackUnsigned16 startLed = atoi(before);
              stackUnsigned16 nrOfLeds = atoi(after) - atoi(before) + 1;
              ppf("FastLED.addLeds new %d: %d-%d\n", pinNr, startLed, nrOfLeds-1);

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
            } //if led range in details (- in details e.g. 0-1023)
          } //if pin owned by leds
          pinNr++;
        } // for pins
        fixture.doAllocPins = false;
      }
    }
  } //loop

  void loop1s() {
    mdl->setUIValueV("realFps", "%lu /s", frameCounter);
    frameCounter = 0;
  }

  void loop10s() {
    // ppf("caching %u %u\n", trigoCached, trigoUnCached); //not working for some reason!!
    // trigoCached = 0;
    // trigoUnCached = 0;
  }

private:
  unsigned long frameMillis = 0;
  unsigned long frameCounter = 0;

};

extern LedModEffects *eff;