/*
   @title     StarLeds
   @file      LedProjections.h
   @date      20240228
   @repo      https://github.com/MoonModules/StarLeds
   @Authors   https://github.com/MoonModules/StarLeds/commits/main
   @Copyright © 2024 Github StarLeds Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

//should not contain variables/bytes to keep mem as small as possible!!
class Projection {
public:
  virtual const char * name() {return "noname";}
  virtual const char * tags() {return "";}
  virtual uint8_t dim() {return _1D;};

  virtual void step1(Fixture &fixture) {}

  virtual void controls(Leds &leds, JsonObject parentVar) {}

};

class DefaultProjection: public Projection {
  const char * name() {return "Default";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💡";}

  void step1(Leds &leds) {
  }
  
  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //DefaultProjection

class MultiplyProjection: public Projection {
  const char * name() {return "Multiply";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💡";}

  void adjustSizeAndPixel(Coord3D &sizeAdjusted, Coord3D &pixelAdjusted) {
    Coord3D proMulti;
    // proMulti = mdl->getValue("proMulti", rowNr);
    // //promultu can be 0,0,0 but /= protects from /div0
    // sizeAdjusted /= proMulti; sizeAdjusted = sizeAdjusted.maximum(Coord3D{1,1,1}); //size min 1,1,1
    // proCenter /= proMulti;
    // mirrors = pixelAdjusted / sizeAdjusted; //place the pixel in the right quadrant
    // pixelAdjusted = pixelAdjusted%sizeAdjusted; // pixel % size
  }

  public: //to use in Preset1Projection
  void controls(Leds &leds, JsonObject parentVar) {
    ui->initCoord3D(parentVar, "proMulti", Coord3D{3,3,1}, 0, 10, false, [leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "MultiplyX");
        return true;
      case f_ChangeFun:
        ui->initCheckBox(var, "mirror", false, false, [leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
          case f_ChangeFun:
            if (rowNr < leds.fixture->listOfLeds.size()) {
              leds.fixture->listOfLeds[rowNr]->doMap = true;
              leds.fixture->doMap = true;
            }
            return true;
          default: return false;
        }});
        if (rowNr < leds.fixture->listOfLeds.size()) {
          leds.fixture->listOfLeds[rowNr]->doMap = true;
          leds.fixture->doMap = true;
        }
        return true;
      default: return false;
    }});
  }
}; //MultiplyProjection

class TiltPanRollProjection: public Projection {
  const char * name() {return "TiltPanRoll";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💡";}

  public: //to use in Preset1Projection
  void controls(Leds &leds, JsonObject parentVar) {
    ui->initSlider(parentVar, "proTilt", 128, 0, 254, false, [leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ChangeFun:
        if (rowNr < leds.fixture->listOfLeds.size())
          leds.fixture->listOfLeds[rowNr]->proTiltSpeed = mdl->getValue(var, rowNr);
        return true;
      default: return false;
    }});
    ui->initSlider(parentVar, "proPan", 128, 0, 254, false, [leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ChangeFun:
        if (rowNr < leds.fixture->listOfLeds.size())
          leds.fixture->listOfLeds[rowNr]->proPanSpeed = mdl->getValue(var, rowNr);
        return true;
      default: return false;
    }});
    ui->initSlider(parentVar, "proRoll", 128, 0, 254, false, [leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "Roll speed");
        return true;
      case f_ChangeFun:
        if (rowNr < leds.fixture->listOfLeds.size())
          leds.fixture->listOfLeds[rowNr]->proRollSpeed = mdl->getValue(var, rowNr);
        return true;
      default: return false;
    }});
  }
}; //TiltPanRollProjection

class DistanceFromPointProjection: public Projection {
  const char * name() {return "Distance ⌛";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💡";}

  public: //to use in Preset1Projection
  void controls(Leds &leds, JsonObject parentVar) {
    ui->initCoord3D(parentVar, "proCenter", Coord3D{8,8,8}, 0, NUM_LEDS_Max, false, [leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "Center");
        return true;
      case f_ChangeFun:
        //initiate projectAndMap
        ppf("proCenter %d %d\n", rowNr, leds.fixture->listOfLeds.size());
        if (rowNr < leds.fixture->listOfLeds.size()) {
          leds.fixture->listOfLeds[rowNr]->doMap = true; //Guru Meditation Error: Core  1 panic'ed (StoreProhibited). Exception was unhandled.
          leds.fixture->doMap = true;
        }
        // ui->setLabel(var, "Size");
        return true;
      default: return false;
    }});
  }
}; //DistanceFromPointProjection

class Preset1Projection: public Projection {
  const char * name() {return "Preset1";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💡";}

  void controls(Leds &leds, JsonObject parentVar) {
    DistanceFromPointProjection dp;
    dp.controls(leds, parentVar);
    MultiplyProjection mp;
    mp.controls(leds, parentVar);
    TiltPanRollProjection tp;
    tp.controls(leds, parentVar);
  }
}; //Preset1Projection

class NoneProjection: public Projection {
  const char * name() {return "None";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💡";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //NoneProjection

class RandomProjection: public Projection {
  const char * name() {return "Random";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💡";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //RandomProjection

class ReverseProjection: public Projection {
  const char * name() {return "Reverse WIP";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💡";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //ReverseProjection

class MirrorProjection: public Projection {
  const char * name() {return "Mirror WIP";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💡";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //MirrorProjection

class KaleidoscopeProjection: public Projection {
  const char * name() {return "Kaleidoscope WIP";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💡";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //KaleidoscopeProjection

class TestProjection: public Projection {
  const char * name() {return "Test";}
  uint8_t dim() {return _1D;}
  const char * tags() {return "💡";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //TestProjection