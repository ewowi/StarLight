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

class NoneProjection: public Projection {
  const char * name() {return "None";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //NoneProjection


class DefaultProjection: public Projection {
  const char * name() {return "Default";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

}; //DefaultProjection

class MultiplyProjection: public Projection {
  const char * name() {return "Multiply";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

  public: //to use in Preset1Projection

  void adjustSizeAndPixel(Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &proCenter) {
    Coord3D proMulti = mdl->getValue("proMulti"); //, rowNr
    //promulti can be 0,0,0 but /= protects from /div0
    sizeAdjusted /= proMulti; sizeAdjusted = sizeAdjusted.maximum(Coord3D{1,1,1}); //size min 1,1,1
    proCenter /= proMulti;
    pixelAdjusted = pixelAdjusted%sizeAdjusted; // pixel % size
    // ppf("Multiply %d,%d,%d\n", leds->size.x, leds->size.y, leds->size.z);
  }

  void adjustMapped(Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted) {
    // if mirrored find the indexV of the mirrored pixel
    bool mirror = mdl->getValue("mirror");

    if (mirror) {
      Coord3D mirrors = pixelAdjusted / sizeAdjusted; //place the pixel in the right quadrant
      if (mirrors.x %2 != 0) mapped.x = sizeAdjusted.x - 1 - mapped.x;
      if (mirrors.y %2 != 0) mapped.y = sizeAdjusted.y - 1 - mapped.y;
      if (mirrors.z %2 != 0) mapped.z = sizeAdjusted.z - 1 - mapped.z;
    }
  }

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initCoord3D(parentVar, "proMulti", {2,2,1}, 0, 10, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "MultiplyX");
        return true;
      case onChange:
        ui->initCheckBox(var, "mirror", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
          case onChange:
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
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

  public: //to use in Preset1Projection

  void adjustXYZ(Leds &leds, Coord3D &pixel) {
    #ifdef STARBASE_USERMOD_MPU6050
      if (leds.proGyro) {
        pixel = trigoTiltPanRoll.tilt(pixel, leds.size/2, mpu6050->gyro.x);
        pixel = trigoTiltPanRoll.pan(pixel, leds.size/2, mpu6050->gyro.y);
        pixel = trigoTiltPanRoll.roll(pixel, leds.size/2, mpu6050->gyro.z);
      }
      else 
    #endif
    {
      if (leds.proTiltSpeed) pixel = trigoTiltPanRoll.tilt(pixel, leds.size/2, sys->now * 5 / (255 - leds.proTiltSpeed));
      if (leds.proPanSpeed) pixel = trigoTiltPanRoll.pan(pixel, leds.size/2, sys->now * 5 / (255 - leds.proPanSpeed));
      if (leds.proRollSpeed) pixel = trigoTiltPanRoll.roll(pixel, leds.size/2, sys->now * 5 / (255 - leds.proRollSpeed));
      if (leds.fixture->fixSize.z == 1) pixel.z = 0; // 3d effects will be flattened on 2D fixtures
    }
  }

  void controls(Leds &leds, JsonObject parentVar) {
    //tbd: implement variable by reference for rowNrs
    #ifdef STARBASE_USERMOD_MPU6050
      ui->initCheckBox(parentVar, "proGyro", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case onChange:
          if (rowNr < leds.fixture->listOfLeds.size())
            leds.fixture->listOfLeds[rowNr]->proGyro = mdl->getValue(var, rowNr);
          return true;
        default: return false;
      }});
    #endif
    ui->initSlider(parentVar, "proTilt", 128, 0, 254, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        if (rowNr < leds.fixture->listOfLeds.size())
          leds.fixture->listOfLeds[rowNr]->proTiltSpeed = mdl->getValue(var, rowNr);
        return true;
      default: return false;
    }});
    ui->initSlider(parentVar, "proPan", 128, 0, 254, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        if (rowNr < leds.fixture->listOfLeds.size())
          leds.fixture->listOfLeds[rowNr]->proPanSpeed = mdl->getValue(var, rowNr);
        return true;
      default: return false;
    }});
    ui->initSlider(parentVar, "proRoll", 128, 0, 254, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Roll speed");
        return true;
      case onChange:
        if (rowNr < leds.fixture->listOfLeds.size())
          leds.fixture->listOfLeds[rowNr]->proRollSpeed = mdl->getValue(var, rowNr);
        return true;
      default: return false;
    }});
  }
}; //TiltPanRollProjection

class DistanceFromPointProjection: public Projection {
  const char * name() {return "Distance ⌛";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

  public: //to use in Preset1Projection

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initCoord3D(parentVar, "proCenter", {8,8,8}, 0, NUM_LEDS_Max, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Center");
        return true;
      case onChange:
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
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

  void adjustSizeAndPixel(Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &proCenter) {
    MultiplyProjection mp;
    mp.adjustSizeAndPixel(sizeAdjusted, pixelAdjusted, proCenter);
  }

  void adjustMapped(Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted) {
    MultiplyProjection mp;
    mp.adjustMapped(mapped, sizeAdjusted, pixelAdjusted);
  }

  void adjustXYZ(Leds &leds, Coord3D &pixel) {
    TiltPanRollProjection tp;
    tp.adjustXYZ(leds, pixel);
  }

  void controls(Leds &leds, JsonObject parentVar) {
    DistanceFromPointProjection dp;
    dp.controls(leds, parentVar);
    MultiplyProjection mp;
    mp.controls(leds, parentVar);
    TiltPanRollProjection tp;
    tp.controls(leds, parentVar);
  }
}; //Preset1Projection

class RandomProjection: public Projection {
  const char * name() {return "Random";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //RandomProjection

class ReverseProjection: public Projection {
  const char * name() {return "Reverse WIP";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💡";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //ReverseProjection

class MirrorProjection: public Projection {
  const char * name() {return "Mirror WIP";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💡";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //MirrorProjection

class GroupingProjection: public Projection {
  const char * name() {return "Grouping WIP";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💡";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //GroupingProjection

class SpacingProjection: public Projection {
  const char * name() {return "Spacing WIP";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💡";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //SpacingProjection

class KaleidoscopeProjection: public Projection {
  const char * name() {return "Kaleidoscope WIP";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //KaleidoscopeProjection

// Pinwheel WIP
// Currently 1D to 2D/3D May be possible to make 2D to 2D/3D
class PinwheelProjection: public Projection {
  const char * name() {return "Pinwheel WIP";}
  const char * tags() {return "💡";}

  void adjustSizeAndPixel(Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &proCenter) {
    sizeAdjusted.x = mdl->getValue("Petals");
    sizeAdjusted.y = 1;
    sizeAdjusted.z = 1;
  }

  void adjustMapped(Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted) {
    // get mdl variables
    Coord3D center = mdl->getValue("Center");
    int swirlVal   = mdl->getValue("swirlVal");
    bool reverse   = mdl->getValue("reverse");
    int angleRange = max(1, int(mdl->getValue("angleRange")));
    int petals     = max(1, sizeAdjusted.x); // sizeAdjusted.x == mdl->getValue("Petals");

    if (swirlVal < 0) { // swap x and y instead of negative swirlVal use transpose later
      swirlVal = abs(swirlVal);
      int temp = pixelAdjusted.x;
      pixelAdjusted.x = pixelAdjusted.y;
      pixelAdjusted.y = temp;
    }

    // float swirlFactor = sqrt(sq(pixelAdjusted.x - center.x) + sq(pixelAdjusted.y - center.y)) * swirlVal;
    float swirlFactor = hypot(pixelAdjusted.x - center.x, pixelAdjusted.y - center.y) * swirlVal;
    float radians = atan2(pixelAdjusted.y - center.y, pixelAdjusted.x - center.x);
    float angle = degrees(radians);

    int value  = round(angle) + 180 + swirlFactor; // Keeping values variables separate for debugging
    int value2 = value % angleRange;
    int value3 = value2;
    if (reverse) value3 = angleRange - value2 - 1; // Reverse

    int value4 = round(value3 / (angleRange / float(petals)));
    value4 = value4 % petals;

    mapped.x = value4;
    mapped.y = 1;
    mapped.z = 1;

    // if (pixelAdjusted.x == 0 && pixelAdjusted.y == 0 && pixelAdjusted.z == 0) ppf("Pinwheel  Center: (%d, %d) SwirlVal: %f angleRange: %d\n", center.x, center.y, swirlVal, angleRange);
    // if (pixelAdjusted.x == 0) { //print first column
    //   ppf(" pixelAdjusted: %d,%d,%d sizeAdjusted: %d,%d,%d", pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z, sizeAdjusted.x, sizeAdjusted.y, sizeAdjusted.z);
    //   ppf(" mapped %d,%d,%d", mapped.x, mapped.y, mapped.z);
    //   ppf(" angle %f angleRange %d value %d value2 %d value3 %d value4 %d\n", angle, angleRange, value, value2, value3, value4);
    // }
  }
  void controls(Leds &leds, JsonObject parentVar) {
    // Bad idea to use fixture->fixSize? Segments default won't be centered.
    // leds.endPos works for "segmenting" use min(fixSize, endPos)?
    // min(leds.fixture->fixSize/2, leds.endPos/2) syntax doesn't work CRGB error?
    ui->initCoord3D(parentVar, "Center", {min(leds.fixture->fixSize.x/2, leds.endPos.x/2), min(leds.fixture->fixSize.y/2, leds.endPos.y/2), min(leds.fixture->fixSize.z/2, leds.endPos.z/2)}, -10, 100, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Pinwheel Center"); // Does nothing?
        return true;
      case onChange:
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    // Slider range supposed to be 0 - 255, but -15 to 15 seems to work fine?
    ui->initSlider(parentVar, "swirlVal", 0, -15, 15, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Swirl"); // Does nothing?
        return true;
      case onChange:
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    // Angle range 0 - angleRange. For testing purposes
    ui->initNumber(parentVar, "angleRange", 360, 1, 720, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    // Naming petals, arms, blades, rays? Controls virtual strip length.
    ui->initNumber(parentVar, "Petals", 360, 1, 360, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});

    // use reverse class when implemented
    ui->initCheckBox(parentVar, "reverse", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    // UI add multiply / mirror / tilt pan roll

  }
}; //PinwheelProjection

class TestProjection: public Projection {
  const char * name() {return "Test";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💡";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //TestProjection