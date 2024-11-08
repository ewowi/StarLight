/*
   @title     StarLight
   @file      LedProjections.h
   @date      20241105
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

//Projections should not contain variables/bytes to keep mem as small as possible!!

#include "LedModFixture.h"

class NoneProjection: public Projection {
  const char * name() {return "None";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

  void setup(LedsLayer &leds, JsonObject parentVar) {
  }
}; //NoneProjection

#ifdef STARLIGHT_LIVE_MAPPING
  class LiveMappingProjection: public Projection {
    const char * name() {return "Live Fixture Mapping";}
    //uint8_t dim() {return _1D;} // every projection should work for all D
    const char * tags() {return "💫";}

    void setup(LedsLayer &leds, JsonObject parentVar) {
    }
  }; //LiveMappingProjection
#endif

class DefaultProjection: public Projection {
  const char * name() {return "Default";}
  const char * tags() {return "💫";}

  public:

  void addPixelsPre(LedsLayer &leds) {
      ppf ("Default Projection %dD -> %dD Effect  Size: %d,%d,%d ->", leds.projectionDimension, leds.effectDimension, leds.size.x, leds.size.y, leds.size.z);
    switch (leds.effectDimension) {
      case _1D: // effectDimension 1DxD
          leds.size.x = sqrt(sq(max(leds.size.x - leds.middle.x, leds.middle.x)) + 
                                sq(max(leds.size.y - leds.middle.y, leds.middle.y)) + 
                                sq(max(leds.size.z - leds.middle.z, leds.middle.z))) + 1;
          leds.size.y = 1;
          leds.size.z = 1;
          break;
      case _2D: // effectDimension 2D
          switch (leds.projectionDimension) {
              case _1D: // 2D1D
                  leds.size.x = sqrt(leds.size.x * leds.size.y * leds.size.z); // only one is > 1, square root
                  leds.size.y = leds.size.x * leds.size.y * leds.size.z / leds.size.x;
                  leds.size.z = 1;
                  break;
              case _2D: // 2D2D
                  // find the 2 axes
                  if (leds.size.x > 1) {
                      if (leds.size.y <= 1) {
                          leds.size.y = leds.size.z;
                      }
                  } else {
                      leds.size.x = leds.size.y;
                      leds.size.y = leds.size.z;
                  }
                  leds.size.z = 1;
                  break;
              case _3D: // 2D3D
                  leds.size.x = leds.size.x + leds.size.y / 2;
                  leds.size.y = leds.size.y / 2 + leds.size.z;
                  leds.size.z = 1;
                  break;
          }
          break;
      case _3D: // effectDimension 3D
          switch (leds.projectionDimension) {
              case _1D:
                  leds.size.x = std::pow(leds.size.x * leds.size.y * leds.size.z, 1/3); // only one is > 1, cube root
                  break;
              case _2D:
                  break;
              case _3D:
                  break;
          }
          break;
    }
  }

  void addPixel(LedsLayer &leds, Coord3D &pixel) {
    Coord3D mapped;
    switch (leds.effectDimension) {
      case _1D: // effectDimension 1DxD
          mapped.x = pixel.distance(leds.middle);
          mapped.y = 0;
          mapped.z = 0;
          break;
      case _2D: // effectDimension 2D
          switch (leds.projectionDimension) {
              case _1D: // 2D1D
                  mapped.x = (pixel.x + pixel.y + pixel.z) % leds.size.x; // only one > 0
                  mapped.y = (pixel.x + pixel.y + pixel.z) / leds.size.x; // all rows next to each other
                  mapped.z = 0;
                  break;
              case _2D: // 2D2D
                  if (leds.size.x > 1) {
                      mapped.x = pixel.x;
                      if (leds.size.y > 1) {
                          mapped.y = pixel.y;
                      } else {
                          mapped.y = pixel.z;
                      }
                  } else {
                      mapped.x = pixel.y;
                      mapped.y = pixel.z;
                  }
                  mapped.z = 0;
                  break;
              case _3D: // 2D3D
                  mapped.x = pixel.x + pixel.y / 2;
                  mapped.y = pixel.y / 2 + pixel.z;
                  mapped.z = 0;
                  break;
          }
          break;
      case _3D: // effectDimension 3D
          mapped = pixel;
          break;
    }

    pixel = mapped;
  }

}; //DefaultProjection

class PinwheelProjection: public Projection {
  const char * name() {return "Pinwheel";}
  const char * tags() {return "💫";}

  public:

  void setup(LedsLayer &leds, JsonObject parentVar) {
    uint8_t *petals   = leds.projectionData.write<uint8_t>(60); // Initalize petal first for addPixel
    uint8_t *swirlVal = leds.projectionData.write<uint8_t>(30);
    bool3State    *reverse  = leds.projectionData.write<bool3State>(false);
    uint8_t *symmetry = leds.projectionData.write<uint8_t>(1);
    uint8_t *zTwist   = leds.projectionData.write<uint8_t>(0);

    ui->initSlider(parentVar, "swirl", swirlVal, 0, 60, false, [&leds](EventArguments) { switch (eventType) {
      case onChange:
        leds.triggerMapping();
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "reverse", reverse, false, [&leds](EventArguments) { switch (eventType) {
      case onChange:
        leds.triggerMapping();
        return true;
      default: return false;
    }});
    // Testing zTwist range 0 to 42 arbitrary values for testing. Hide if not 3D fixture. Select pinwheel while using 3D fixture.
    if (leds.projectionDimension == _3D) {
      ui->initSlider(parentVar, "zTwist", zTwist, 0, 42, false, [&leds](EventArguments) { switch (eventType) {
        case onChange:
          leds.triggerMapping();
          return true;
        default: return false;
      }});
    }
    // Rotation symmetry. Uses factors of 360.
    ui->initSlider(parentVar, "rotationalSymmetry", symmetry, 1, 23, false, [&leds](EventArguments) { switch (eventType) {
      case onChange:
        leds.triggerMapping();
        return true;
      default: return false;
    }});
    // Naming petals, arms, blades, rays? Controls virtual strip length.
    ui->initSlider(parentVar, "petals", petals, 1, 60, false, [&leds](EventArguments) { switch (eventType) {
      case onChange:
        leds.triggerMapping();
        return true;
      default: return false;
    }});
  }

  void addPixelsPre(LedsLayer &leds) {
    const int petals = leds.projectionData.read<uint8_t>();
    if (leds.projectionDimension > _1D && leds.effectDimension > _1D) {
      leds.size.y = sqrt(sq(max(leds.size.x - leds.middle.x, leds.middle.x)) + 
                            sq(max(leds.size.y - leds.middle.y, leds.middle.y))) + 1; // Adjust y before x
      leds.size.x = petals;
      leds.size.z = 1;
    }
    else {
      leds.size.x = petals;
      leds.size.y = 1;
      leds.size.z = 1;
    }
  }

  void addPixel(LedsLayer &leds, Coord3D &pixel) {
    // Coord3D mapped;
    // factors of 360
    const int FACTORS[24] = {360, 180, 120, 90, 72, 60, 45, 40, 36, 30, 24, 20, 18, 15, 12, 10, 9, 8, 6, 5, 4, 3, 2};
    // UI Variables
    const int petals   = leds.projectionData.read<uint8_t>();
    const int swirlVal = leds.projectionData.read<uint8_t>() - 30; // SwirlVal range -30 to 30
    const bool3State reverse = leds.projectionData.read<bool3State>();
    const int symmetry = FACTORS[leds.projectionData.read<uint8_t>()-1];
    const int zTwist   = leds.projectionData.read<uint8_t>();
         
    const int dx = pixel.x - leds.middle.x;
    const int dy = pixel.y - leds.middle.y;
    const int swirlFactor = swirlVal == 0 ? 0 : hypot(dy, dx) * abs(swirlVal); // Only calculate if swirlVal != 0
    int angle = degrees(atan2(dy, dx)) + 180;  // 0 - 360
    
    if (swirlVal < 0) angle = 360 - angle; // Reverse Swirl

    int value = angle + swirlFactor + (zTwist * pixel.z);
    float petalWidth = symmetry / float(petals);
    value /= petalWidth;
    value %= petals;

    if (reverse) value = petals - value - 1; // Reverse Movement

    pixel.x = value;
    pixel.y = 0;
    if (leds.effectDimension > _1D && leds.projectionDimension > _1D) {
      pixel.y = int(sqrt(sq(dx) + sq(dy))); // Round produced blank pixel
    }
    pixel.z = 0;

    // if (pixel.x == 0 && pixel.y == 0 && pixel.z == 0) ppf("Pinwheel  Center: (%d, %d) SwirlVal: %d Symmetry: %d Petals: %d zTwist: %d\n", leds.middle.x, leds.middle.y, swirlVal, symmetry, petals, zTwist);
    // ppf("pixel %2d,%2d,%2d -> %2d,%2d,%2d Angle: %3d Petal: %2d\n", pixel.x, pixel.y, pixel.z, mapped.x, mapped.y, mapped.z, angle, value);
  }
}; //PinwheelProjection

class MultiplyProjection: public Projection {
  const char * name() {return "Multiply";}
  const char * tags() {return "💫";}

  public:

  void setup(LedsLayer &leds, JsonObject parentVar) {
    Coord3D *proMulti = leds.projectionData.write<Coord3D>({2,2,1});
    bool3State *mirror = leds.projectionData.write<bool3State>(false);
    ui->initCoord3D(parentVar, "proMulti", proMulti, 0, 10, false, [&leds](EventArguments) { switch (eventType) {
      case onChange:
        leds.triggerMapping();
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "mirror", mirror, false, [&leds](EventArguments) { switch (eventType) {
      case onChange:
        leds.triggerMapping();
        return true;
      default: return false;
    }});
  }

  void addPixelsPre(LedsLayer &leds) {
    Coord3D proMulti = leds.projectionData.read<Coord3D>();
    bool3State    mirror   = leds.projectionData.read<bool3State>();
    Coord3D *originalSize = leds.projectionData.readWrite<Coord3D>();

    proMulti = proMulti.maximum(Coord3D{1, 1, 1}); // {1, 1, 1} is the minimum value
    if (proMulti == Coord3D{1, 1, 1}) return;      // No need to adjust if proMulti is {1, 1, 1}
    
    leds.size = (leds.size + proMulti - Coord3D({1,1,1})) / proMulti; // Round up
    leds.middle /= proMulti;

    *originalSize = leds.size;

    DefaultProjection dp;
    dp.addPixelsPre(leds);
  }

  void addPixel(LedsLayer &leds, Coord3D &pixel) {
    // UI Variables
    Coord3D proMulti = leds.projectionData.read<Coord3D>();
    bool3State    mirror   = leds.projectionData.read<bool3State>();
    Coord3D originalSize = leds.projectionData.read<Coord3D>();

    if (mirror) {
      Coord3D mirrors = pixel / originalSize; // Place the pixel in the right quadrant
      pixel = pixel % originalSize;
      if (mirrors.x %2 != 0) pixel.x = originalSize.x - 1 - pixel.x;
      if (mirrors.y %2 != 0) pixel.y = originalSize.y - 1 - pixel.y;
      if (mirrors.z %2 != 0) pixel.z = originalSize.z - 1 - pixel.z;
    }
    else pixel = pixel % originalSize;

    DefaultProjection dp;
    dp.addPixel(leds, pixel);
  }

}; //MultiplyProjection

class TiltPanRollProjection: public Projection {
  const char * name() {return "TiltPanRoll";}
  const char * tags() {return "💫";}

  public:

  void setup(LedsLayer &leds, JsonObject parentVar) {
    //tbd: implement variable by reference for rowNrs
    #ifdef STARBASE_USERMOD_MPU6050
      ui->initCheckBox(parentVar, "gyro", false, false, [&leds](EventArguments) { switch (eventType) {
        case onChange:
          leds.proGyro = variable.getValue(rowNr);
          return true;
        default: return false;
      }});
    #endif
    ui->initSlider(parentVar, "tilt", 128, 0, 254, false, [&leds](EventArguments) { switch (eventType) {
      case onChange:
        leds.proTiltSpeed = variable.getValue(rowNr);
        return true;
      default: return false;
    }});
    ui->initSlider(parentVar, "pan", 128, 0, 254, false, [&leds](EventArguments) { switch (eventType) {
      case onChange:
        leds.proPanSpeed = variable.getValue(rowNr);
        return true;
      default: return false;
    }});
    ui->initSlider(parentVar, "roll", 128, 0, 254, false, [&leds](EventArguments) { switch (eventType) {
      case onUI:
        variable.setComment("Roll speed");
        return true;
      case onChange:
        leds.proRollSpeed = variable.getValue(rowNr);
        return true;
      default: return false;
    }});
  }

  void addPixel(LedsLayer &leds, Coord3D &pixel) {
    // adjustSizeAndPixel(leds, pixel); // Uncomment to expand grid to fill corners
    DefaultProjection dp;
    dp.addPixel(leds, pixel);
  }

  void adjustSizeAndPixel(LedsLayer &leds, Coord3D &pixel) {
    uint8_t size = max(leds.size.x, max(leds.size.y, leds.size.z));
    size = sqrt(size * size * 2) + 1;
    Coord3D offset = {(size - leds.size.x) / 2, (size - leds.size.y) / 2, 0};
    leds.size = Coord3D{size, size, 1};

    pixel.x += offset.x;
    pixel.y += offset.y;
    pixel.z += offset.z;
  }

  void XYZ(LedsLayer &leds, Coord3D &pixel) {
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
      if (fix->fixSize.z == 1) pixel.z = 0; // 3d effects will be flattened on 2D fixtures
    }
  }
}; //TiltPanRollProjection

class DistanceFromPointProjection: public Projection {
  const char * name() {return "Distance ⌛";}
  const char * tags() {return "💫";}

  public:

  void addPixel(LedsLayer &leds, Coord3D &pixel) {
    DefaultProjection dp;
    dp.addPixel(leds, pixel);
    if (leds.projectionDimension == _2D && leds.effectDimension == _2D) postProcessing(leds, pixel);
  }

  void postProcessing(LedsLayer &leds, Coord3D &pixel) {
    //2D2D: inverse mapping
    Trigo trigo(leds.size.x-1); // 8 bits trigo with period leds.size.x-1 (currentl Float trigo as same performance)
    float minDistance = 10;
    for (uint16_t x=0; x<leds.size.x && minDistance > 0.5f; x++) {
      // float xFactor = x * TWO_PI / (float)(leds.size.x-1); //between 0 .. 2PI

      float xNew = trigo.sin(leds.size.x, x);
      float yNew = trigo.cos(leds.size.y, x);

      for (uint16_t y=0; y<leds.size.y && minDistance > 0.5f; y++) {

        // float yFactor = (leds.size.y-1.0f-y) / (leds.size.y-1.0f); // between 1 .. 0
        float yFactor = 1 - y / (leds.size.y-1.0f); // between 1 .. 0

        float x2New = round((yFactor * xNew + leds.size.x) / 2.0f); // 0 .. size.x
        float y2New = round((yFactor * yNew + leds.size.y) / 2.0f); //  0 .. size.y

        // ppf(" %d,%d->%f,%f->%f,%f", x, y, sinf(x * TWO_PI / (float)(size.x-1)), cosf(x * TWO_PI / (float)(size.x-1)), xNew, yNew);

        //this should work (better) but needs more testing
        // float distance = abs(indexV - xNew - yNew * size.x);
        // if (distance < minDistance) {
        //   minDistance = distance;
        //   indexV = x+y*size.x;
        // }

        // if the new XY i
        if (pixel == Coord3D({(int)x2New, (int)y2New, 0})) {
          // ppf("  found one %d => %d=%d+%d*%d (%f+%f*%d) [%f]\n", x+y*size.x, x,y, size.x, xNew, yNew, size.x, distance);
          pixel.x = x;
          pixel.y = y;
          pixel.z = 0;

          if (pixel.x%10 == 0) ppf("."); //show some progress as this projection is slow (Need S007 to optimize ;-)
                                      
          minDistance = 0.0f; // stop looking further
        }
      }
    }
    if (minDistance > 0.5f) {pixel.x = UINT16_MAX; return;} //do not show this pixel
  }
}; //DistanceFromPointProjection

class Preset1Projection: public Projection {
  const char * name() {return "Preset1";}
  const char * tags() {return "💫";}

  void setup(LedsLayer &leds, JsonObject parentVar) {
    MultiplyProjection mp;
    mp.setup(leds, parentVar);
    TiltPanRollProjection tp;
    tp.setup(leds, parentVar);
  }

  void addPixelsPre(LedsLayer &leds) {
    MultiplyProjection mp;
    mp.addPixelsPre(leds);

    DefaultProjection dp;
    dp.addPixelsPre(leds);

    TiltPanRollProjection tp;
    tp.addPixelsPre(leds);
  }

  void addPixel(LedsLayer &leds, Coord3D &pixel) {
    MultiplyProjection mp;
    mp.addPixel(leds, pixel);

    DefaultProjection dp;
    dp.addPixel(leds, pixel);
  }

  void XYZ(LedsLayer &leds, Coord3D &pixel) {
    TiltPanRollProjection tp;
    tp.XYZ(leds, pixel);
  }
}; //Preset1Projection

class RandomProjection: public Projection {
  const char * name() {return "Random";}
  const char * tags() {return "💫";}

  void setup(LedsLayer &leds, JsonObject parentVar) {
  }

  void XYZ(LedsLayer &leds, Coord3D &pixel) {
    pixel = {random(leds.size.x), random(leds.size.y), random(leds.size.z)};
  }
}; //RandomProjection

class MirrorReverseTransposeProjection: public Projection {
  const char * name() {return "Mirror Reverse Transpose";}
  const char * tags() {return "💡";}

  struct MRTData {
    union {
      struct {
        bool mirrorX  : 1;
        bool mirrorY  : 1;
        bool mirrorZ  : 1;
        bool reverseX : 1;
        bool reverseY : 1;
        bool reverseZ : 1;
        bool transposeXY : 1;
        bool transposeXZ : 1;
        bool transposeYZ : 1;
      };
      uint8_t flags;
    };
    Coord3D originalSize;
  };

  public:

  void setup(LedsLayer &leds, JsonObject parentVar) {
      MRTData *data = leds.projectionData.readWrite<MRTData>();
      ui->initCheckBox(parentVar, "Mirror X", false, false, [&leds, data](EventArguments) { switch (eventType) {
        case onChange:
          data->mirrorX = variable.getValue(rowNr);
          leds.triggerMapping();
          return true;
        default: return false;
      }});
      ui->initCheckBox(parentVar, "Mirror Y", false, false, [&leds, data](EventArguments) { switch (eventType) {
        case onChange:
          data->mirrorY = variable.getValue(rowNr);
          leds.triggerMapping();
          return true;
        default: return false;
      }});
      if (leds.projectionDimension == _3D) {
      ui->initCheckBox(parentVar, "Mirror Z", false, false, [&leds, data](EventArguments) { switch (eventType) {
        case onChange:
          data->mirrorZ = variable.getValue(rowNr);
          leds.triggerMapping();
          return true;
        default: return false;
      }});
      }
      ui->initCheckBox(parentVar, "Reverse X", false, false, [&leds, data](EventArguments) { switch (eventType) {
        case onChange:
          data->reverseX = variable.getValue(rowNr);
          leds.triggerMapping();
          return true;
        default: return false;
      }});
      ui->initCheckBox(parentVar, "Reverse Y", false, false, [&leds, data](EventArguments) { switch (eventType) {
        case onChange:
          data->reverseY = variable.getValue(rowNr);
          leds.triggerMapping();
          return true;
        default: return false;
      }});
      if (leds.projectionDimension == _3D) {
      ui->initCheckBox(parentVar, "Reverse Z", false, false, [&leds, data](EventArguments) { switch (eventType) {
        case onChange:
          data->reverseZ = variable.getValue(rowNr);
          leds.triggerMapping();
          return true;
        default: return false;
      }});
      }
      ui->initCheckBox(parentVar, "Transpose XY", false, false, [&leds, data](EventArguments) { switch (eventType) {
        case onChange:
          data->transposeXY = variable.getValue(rowNr);
          leds.triggerMapping();
          return true;
        default: return false;
      }});
      if (leds.projectionDimension == _3D) {
      ui->initCheckBox(parentVar, "Transpose XZ", false, false, [&leds, data](EventArguments) { switch (eventType) {
        case onChange:
          data->transposeXZ = variable.getValue(rowNr);
          leds.triggerMapping();
          return true;
        default: return false;
      }});
      ui->initCheckBox(parentVar, "Transpose YZ", false, false, [&leds, data](EventArguments) { switch (eventType) {
        case onChange:
          data->transposeYZ = variable.getValue(rowNr);
          leds.triggerMapping();
          return true;
        default: return false;
      }});
      }
    }

  void addPixelsPre(LedsLayer &leds) {
    // UI Variables
    MRTData *data = leds.projectionData.readWrite<MRTData>();
    ppf("MRT data: mX %d, mY %d, mZ %d, rX %d, rY %d, rZ %d, tXY %d, tXZ %d, tYZ %d\n", data->mirrorX, data->mirrorY, data->mirrorZ, data->reverseX, data->reverseY, data->reverseZ, data->transposeXY, data->transposeXZ, data->transposeYZ);
    ppf("MRT ledsize %d,%d,%d\n", leds.size.x, leds.size.y, leds.size.z);

    if (data->mirrorX) leds.size.x = (leds.size.x + 1) / 2;
    if (data->mirrorY) leds.size.y = (leds.size.y + 1) / 2;
    if (data->mirrorZ) leds.size.z = (leds.size.z + 1) / 2;

    data->originalSize = leds.size;

    if (data->transposeXY) { int temp = leds.size.x; leds.size.x = leds.size.y; leds.size.y = temp; }
    if (data->transposeXZ) { int temp = leds.size.x; leds.size.x = leds.size.z; leds.size.z = temp; }
    if (data->transposeYZ) { int temp = leds.size.y; leds.size.y = leds.size.z; leds.size.z = temp; }
  
    ppf("MRT ledsize %d,%d,%d\n", leds.size.x, leds.size.y, leds.size.z);
    ppf("MRT data: mX %d, mY %d, mZ %d, rX %d, rY %d, rZ %d, tXY %d, tXZ %d, tYZ %d\n", data->mirrorX, data->mirrorY, data->mirrorZ, data->reverseX, data->reverseY, data->reverseZ, data->transposeXY, data->transposeXZ, data->transposeYZ);

    DefaultProjection dp;
    dp.addPixelsPre(leds);
  }

  void addPixel(LedsLayer &leds, Coord3D &pixel) { 
    MRTData data = leds.projectionData.read<MRTData>();

    // Mirror
    if (data.mirrorX && pixel.x >= data.originalSize.x) pixel.x = data.originalSize.x * 2 - 1 - pixel.x;
    if (data.mirrorY && pixel.y >= data.originalSize.y) pixel.y = data.originalSize.y * 2 - 1 - pixel.y;
    if (data.mirrorZ && pixel.z >= data.originalSize.z) pixel.z = data.originalSize.z * 2 - 1 - pixel.z;
    
    // Reverse
    if (data.reverseX) pixel.x = data.originalSize.x - pixel.x - 1;
    if (data.reverseY) pixel.y = data.originalSize.y - pixel.y - 1;
    if (data.reverseZ) pixel.z = data.originalSize.z - pixel.z - 1;

    // Transpose
    if (data.transposeXY) { int temp = pixel.x; pixel.x = pixel.y; pixel.y = temp; }
    if (data.transposeXZ) { int temp = pixel.x; pixel.x = pixel.z; pixel.z = temp; }
    if (data.transposeYZ) { int temp = pixel.y; pixel.y = pixel.z; pixel.z = temp; }

    DefaultProjection dp;
    dp.addPixel(leds, pixel);
  }
}; //MirrorReverseTransposeProjection

class MirrorProjection: public Projection {
  const char * name() {return "Mirror";}
  const char * tags() {return "💡";}

  public:

  void setup(LedsLayer &leds, JsonObject parentVar) {
    bool3State *mirrorX = leds.projectionData.write<bool3State>(false);
    bool3State *mirrorY = leds.projectionData.write<bool3State>(false);
    bool3State *mirrorZ = leds.projectionData.write<bool3State>(false);
    ui->initCheckBox(parentVar, "mirrorX", mirrorX, false, [&leds](EventArguments) { switch (eventType) {
      case onChange:
        leds.triggerMapping();
        return true;
      default: return false;
    }});
    if (leds.projectionDimension >= _2D) {
      ui->initCheckBox(parentVar, "mirrorY", mirrorY, false, [&leds](EventArguments) { switch (eventType) {
        case onChange:
          leds.triggerMapping();
          return true;
        default: return false;
      }});
    }
    if (leds.projectionDimension == _3D) {
      ui->initCheckBox(parentVar, "mirrorZ", mirrorZ, false, [&leds](EventArguments) { switch (eventType) {
        case onChange:
          leds.triggerMapping();
          return true;
        default: return false;
      }});
    }
  }

  void addPixelsPre(LedsLayer &leds) {
    // UI Variables
    bool3State mirrorX = leds.projectionData.read<bool3State>();
    bool3State mirrorY = leds.projectionData.read<bool3State>();
    bool3State mirrorZ = leds.projectionData.read<bool3State>();
    Coord3D *originalSize = leds.projectionData.readWrite<Coord3D>();

    if (mirrorX) leds.size.x = (leds.size.x + 1) / 2;
    if (mirrorY) leds.size.y = (leds.size.y + 1) / 2;
    if (mirrorZ) leds.size.z = (leds.size.z + 1) / 2;
    *originalSize = leds.size;

    DefaultProjection dp;
    dp.addPixelsPre(leds);
  }

  void addPixel(LedsLayer &leds, Coord3D &pixel) {
    // UI Variables
    bool3State mirrorX = leds.projectionData.read<bool3State>();
    bool3State mirrorY = leds.projectionData.read<bool3State>();
    bool3State mirrorZ = leds.projectionData.read<bool3State>();
    Coord3D originalSize = leds.projectionData.read<Coord3D>();

    if (mirrorX && pixel.x >= originalSize.x) pixel.x = originalSize.x * 2 - 1 - pixel.x;
    if (mirrorY && pixel.y >= originalSize.y) pixel.y = originalSize.y * 2 - 1 - pixel.y;
    if (mirrorZ && pixel.z >= originalSize.z) pixel.z = originalSize.z * 2 - 1 - pixel.z;
    
    DefaultProjection dp;
    dp.addPixel(leds, pixel);
  }

}; //MirrorProjection

// class ReverseProjection: public Projection {
//   const char * name() {return "Reverse";}
//   const char * tags() {return "💡";}
// 
//   public:
// 
//   void setup(LedsLayer &leds, JsonObject parentVar) {
//     bool3State *reverseX = leds.projectionData.write<bool3State>(false);
//     bool3State *reverseY = leds.projectionData.write<bool3State>(false);
//     bool3State *reverseZ = leds.projectionData.write<bool3State>(false);
// 
//     ui->initCheckBox(parentVar, "reverseX", reverseX, false, [&leds](EventArguments) { switch (eventType) {
//       case onChange:
//         leds.triggerMapping();
//         return true;
//       default: return false;
//     }});
//     if (leds.effectDimension >= _2D || leds.projectionDimension >= _2D) {
//       ui->initCheckBox(parentVar, "reverseY", reverseY, false, [&leds](EventArguments) { switch (eventType) {
//         case onChange:
//           leds.triggerMapping();
//           return true;
//         default: return false;
//       }});
//     }
//     if (leds.effectDimension == _3D || leds.projectionDimension == _3D) {
//       ui->initCheckBox(parentVar, "reverseZ", reverseZ, false, [&leds](EventArguments) { switch (eventType) {
//         case onChange:
//           leds.triggerMapping();
//           return true;
//         default: return false;
//       }});
//     }
//   }
// 
//   void addPixel(LedsLayer &leds, Coord3D &pixel) { 
//     bool3State reverseX = leds.projectionData.read<bool3State>();
//     bool3State reverseY = leds.projectionData.read<bool3State>();
//     bool3State reverseZ = leds.projectionData.read<bool3State>();
// 
//     if (reverseX) pixel.x = leds.size.x - pixel.x - 1;
//     if (reverseY) pixel.y = leds.size.y - pixel.y - 1;
//     if (reverseZ) pixel.z = leds.size.z - pixel.z - 1;
// 
//     DefaultProjection dp;
//     dp.addPixel(leds, pixel);
//   }
// 
// }; //ReverseProjection

// class TransposeProjection: public Projection {
//   const char * name() {return "Transpose";}
//   const char * tags() {return "💡";}
// 
//   public:
// 
//   void setup(LedsLayer &leds, JsonObject parentVar) {
//     bool3State *transposeXY = leds.projectionData.write<bool3State>(false);
//     bool3State *transposeXZ = leds.projectionData.write<bool3State>(false);
//     bool3State *transposeYZ = leds.projectionData.write<bool3State>(false);
// 
//     ui->initCheckBox(parentVar, "transpose XY", transposeXY, false, [&leds](EventArguments) { switch (eventType) {
//       case onChange:
//         leds.triggerMapping();
//         return true;
//       default: return false;
//     }});
//     if (leds.effectDimension == _3D) {
//       ui->initCheckBox(parentVar, "transpose XZ", transposeXZ, false, [&leds](EventArguments) { switch (eventType) {
//         case onChange:
//           leds.triggerMapping();
//           return true;
//         default: return false;
//       }});
//       ui->initCheckBox(parentVar, "transpose YZ", transposeYZ, false, [&leds](EventArguments) { switch (eventType) {
//         case onChange:
//           leds.triggerMapping();
//           return true;
//         default: return false;
//       }});
//     }
//   }
// 
//   void addPixelsPre(LedsLayer &leds) {
//     // UI Variables
//     bool3State transposeXY = leds.projectionData.read<bool3State>();
//     bool3State transposeXZ = leds.projectionData.read<bool3State>();
//     bool3State transposeYZ = leds.projectionData.read<bool3State>();
// 
//     if (transposeXY) { int temp = leds.size.x; leds.size.x = leds.size.y; leds.size.y = temp; }
//     if (transposeXZ) { int temp = leds.size.x; leds.size.x = leds.size.z; leds.size.z = temp; }
//     if (transposeYZ) { int temp = leds.size.y; leds.size.y = leds.size.z; leds.size.z = temp; }
// 
//     DefaultProjection dp;
//     dp.addPixelsPre(leds);
//   }
// 
//   void addPixel(LedsLayer &leds, Coord3D &pixel) {
//     // UI Variables
//     bool3State transposeXY = leds.projectionData.read<bool3State>();
//     bool3State transposeXZ = leds.projectionData.read<bool3State>();
//     bool3State transposeYZ = leds.projectionData.read<bool3State>();
// 
//     if (transposeXY) { int temp = pixel.x; pixel.x = pixel.y; pixel.y = temp; }
//     if (transposeXZ) { int temp = pixel.x; pixel.x = pixel.z; pixel.z = temp; }
//     if (transposeYZ) { int temp = pixel.y; pixel.y = pixel.z; pixel.z = temp; }
// 
//     DefaultProjection dp;
//     dp.addPixel(leds, pixel);
//   }
// 
// }; //TransposeProjection

class GroupingSpacingProjection: public Projection {
  const char * name() {return "Grouping & Spacing";}
  const char * tags() {return "💡";}

  public:

  void setup(LedsLayer &leds, JsonObject parentVar) {
    Coord3D *grouping = leds.projectionData.write<Coord3D>({2,2,2});
    Coord3D *spacing  = leds.projectionData.write<Coord3D>({0,0,0});
    ui->initCoord3D(parentVar, "grouping", grouping, 0, 100, false, [&leds](EventArguments) { switch (eventType) {
      case onChange:
        leds.triggerMapping();
        return true;
      default: return false;
    }});
    ui->initCoord3D(parentVar, "spacing", spacing, 0, 100, false, [&leds](EventArguments) { switch (eventType) {
      case onChange:
        leds.triggerMapping();
        return true;
      default: return false;
    }});
  }

  void addPixelsPre(LedsLayer &leds) {
    Coord3D grouping = leds.projectionData.read<Coord3D>().maximum(Coord3D{1, 1, 1}); // {1, 1, 1} is the minimum value
    Coord3D spacing  = leds.projectionData.read<Coord3D>().maximum(Coord3D{0, 0, 0});
    Coord3D GS = grouping + spacing;

    leds.middle /= GS;
    leds.size = (leds.size + (GS - Coord3D{1,1,1})) / GS; // round up

    DefaultProjection dp;
    dp.addPixelsPre(leds);
  }

  void addPixel(LedsLayer &leds, Coord3D &pixel) {
    Coord3D grouping = leds.projectionData.read<Coord3D>().maximum(Coord3D{1, 1, 1}); // {1, 1, 1} is the minimum value
    Coord3D spacing  = leds.projectionData.read<Coord3D>().maximum(Coord3D{0, 0, 0});
    Coord3D GS = grouping + spacing;
    Coord3D modPixel = pixel % GS;

    if (modPixel.x < grouping.x && modPixel.y < grouping.y && modPixel.z < grouping.z) 
      pixel /= GS;
    else {
      pixel.x = UINT16_MAX; return;
    }

    DefaultProjection dp;
    dp.addPixel(leds, pixel);
  }
}; //GroupingSpacingProjection

class ScrollingProjection: public Projection {
  const char * name() {return "Scrolling";}
  const char * tags() {return "💫";}

  void setup(LedsLayer &leds, JsonObject parentVar) {
    MirrorProjection mp;
    mp.setup(leds, parentVar);

    uint8_t *xSpeed  = leds.projectionData.write<uint8_t>(128);
    uint8_t *ySpeed  = leds.projectionData.write<uint8_t>(0);
    uint8_t *zSpeed  = leds.projectionData.write<uint8_t>(0);

    ui->initSlider(parentVar, "xSpeed", xSpeed, 0, 255, false);
    //ewowi: 2D/3D inits will be done automatically in the future, then the if's are not needed here
    if (leds.projectionDimension >= _2D) ui->initSlider(parentVar, "ySpeed", ySpeed, 0, 255, false);
    if (leds.projectionDimension == _3D) ui->initSlider(parentVar, "zSpeed", zSpeed, 0, 255, false);
  }

  void addPixelsPre(LedsLayer &leds) {
    MirrorProjection mp;
    mp.addPixelsPre(leds);
  }

  void addPixel(LedsLayer &leds, Coord3D &pixel) {
    MirrorProjection mp;
    mp.addPixel(leds, pixel);
  }

  void XYZ(LedsLayer &leds, Coord3D &pixel) {
    bool3State mirrorX = leds.projectionData.read<bool3State>(); // Not used 
    bool3State mirrorY = leds.projectionData.read<bool3State>(); // Not used
    bool3State mirrorZ = leds.projectionData.read<bool3State>(); // Not used

    uint8_t xSpeed = leds.projectionData.read<uint8_t>();
    uint8_t ySpeed = leds.projectionData.read<uint8_t>();
    uint8_t zSpeed = leds.projectionData.read<uint8_t>();

    if (xSpeed) pixel.x = (pixel.x + (sys->now * xSpeed / 255 / 100)) % leds.size.x;
    if (ySpeed) pixel.y = (pixel.y + (sys->now * ySpeed / 255 / 100)) % leds.size.y;
    if (zSpeed) pixel.z = (pixel.z + (sys->now * zSpeed / 255 / 100)) % leds.size.z;
  }
}; //ScrollingProjection

#ifdef STARBASE_USERMOD_MPU6050

class AccelerationProjection: public Projection {
  const char * name() {return "Acceleration";}
  const char * tags() {return "💫🧭";}

  public:

  void setup(LedsLayer &leds, JsonObject parentVar) {
    bool3State *wrap = leds.projectionData.write<bool3State>(false);
    uint8_t *sensitivity = leds.projectionData.write<uint8_t>(0);
    uint8_t *deadzone = leds.projectionData.write<uint8_t>(10);

    ui->initCheckBox(parentVar, "wrap", wrap);
    ui->initSlider(parentVar, "sensitivity", sensitivity, 0, 100, false);
    ui->initSlider(parentVar, "deadzone", deadzone, 0, 100, false);
  }

  void addPixel(LedsLayer &leds, Coord3D &pixel) {
    DefaultProjection dp;
    dp.addPixel(leds, pixel);
  }

  void XYZ(LedsLayer &leds, Coord3D &pixel) {
    bool3State wrap = leds.projectionData.read<bool3State>();
    float sensitivity = float(leds.projectionData.read<uint8_t>()) / 20.0 + 1; // 0 - 100 slider -> 1.0 - 6.0 multiplier 
    uint16_t deadzone = map(leds.projectionData.read<uint8_t>(), 0, 255, 0 , 1000); // 0 - 1000

    int accelX = mpu6050->accell.x; 
    int accelY = mpu6050->accell.y;

    if (abs(accelX) < deadzone) accelX = 0;
    if (abs(accelY) < deadzone) accelY = 0;

    int xMove = map(accelX, -32768, 32767, -leds.size.x, leds.size.x) * sensitivity;
    int yMove = map(accelY, -32768, 32767, -leds.size.y, leds.size.y) * sensitivity;

    // if (pixel.x == 0 && pixel.y == 0) ppf("Accel: %d %d xMove: %d yMove: %d\n", accelX, accelY, xMove, yMove);
  
    pixel.x += xMove;
    pixel.y += yMove;
    if (wrap) {
      pixel.x %= leds.size.x;
      pixel.y %= leds.size.y;
    }
  }
}; //Acceleration

#endif

class CheckerboardProjection: public Projection {
  const char * name() {return "Checkerboard";}
  const char * tags() {return "💫";}

  void setup(LedsLayer &leds, JsonObject parentVar) {
    Coord3D *size = leds.projectionData.write<Coord3D>({3,3,3});
    bool3State *invert = leds.projectionData.write<bool3State>(false);
    bool3State *group = leds.projectionData.write<bool3State>(false);
    ui->initCoord3D(parentVar, "squareSize", size, 0, 100, false, [&leds](EventArguments) { switch (eventType) {
      case onChange:
        leds.triggerMapping();
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "invert", invert, false, [&leds](EventArguments) { switch (eventType) {
      case onChange:
        leds.triggerMapping();
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "group", group, false, [&leds](EventArguments) { switch (eventType) {
      case onChange:
        leds.triggerMapping();
        return true;
      default: return false;
    }});
  }

  void addPixelsPre(LedsLayer &leds) {
    Coord3D size = leds.projectionData.read<Coord3D>();
    bool3State invert  = leds.projectionData.read<bool3State>();
    bool3State group   = leds.projectionData.read<bool3State>();

    if (group) { leds.middle /= size; leds.size = (leds.size + (size - Coord3D{1,1,1})) / size; }

    DefaultProjection dp;
    dp.addPixelsPre(leds);
  }

  void addPixel(LedsLayer &leds, Coord3D &pixel) {
    Coord3D size = leds.projectionData.read<Coord3D>().maximum(Coord3D{1, 1, 1});
    bool3State invert = leds.projectionData.read<bool3State>();
    bool3State group = leds.projectionData.read<bool3State>();

    Coord3D check = pixel / size;
    if ((check.x + check.y + check.z) % 2 == 0) {
      if (invert) pixel.x = UINT16_MAX; return;
    }
    else {
      if (!invert) pixel.x = UINT16_MAX; return;
    }

    if (group) pixel /= size;

    DefaultProjection dp; 
    dp.addPixel(leds, pixel);
  }
}; //CheckerboardProjection

class RotateProjection: public Projection {
  const char * name() {return "Rotate";}
  const char * tags() {return "💫";}

  struct RotateData { // 16 bytes
    union {
      struct {
        bool flip : 1;
        bool reverse : 1;
        bool alternate : 1;
        bool expand : 1; 
      };
      uint8_t flags;
    };
    uint8_t  speed;
    uint8_t  midX;
    uint8_t  midY;
    uint16_t angle;
    uint16_t interval; // ms between updates
    int16_t  shearX;
    int16_t  shearY;
    unsigned long lastUpdate; // last sys->now update
  };

  public:

  void setup(LedsLayer &leds, JsonObject parentVar) {
    RotateData *data = leds.projectionData.readWrite<RotateData>();

    ui->initSelect(parentVar, "direction", (uint8_t)0, false, [data](EventArguments) { switch (eventType) {
      case onUI: {
        JsonArray options = variable.setOptions();
        options.add("Clockwise");
        options.add("Counter-Clockwise");
        options.add("Alternate");
        return true; }
      case onChange: {
        uint8_t val = variable.getValue(rowNr);
        if (val == 0) data->reverse = false;
        if (val == 1) data->reverse = true;
        if (val == 2) data->alternate = true; else data->alternate = false;
        return true; }
      default: return false;
    }});
    ui->initSlider(parentVar, "rotateSpeed", 128, 0, 254, false, [data](EventArguments) { switch (eventType) {
      case onChange:
        data->speed = variable.getValue(rowNr);
        data->interval = 1000 / (data->speed + 1);
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "expand", false, false, [&leds](EventArguments) { switch (eventType) {
      case onChange:
        leds.triggerMapping();
        return true;
      default: return false;
    }});
  }

  void addPixelsPre(LedsLayer &leds) {
    RotateData *data = leds.projectionData.readWrite<RotateData>();
    data->expand = mdl->getValue("projection", "Expand");

    if (data->expand) {
      uint8_t size = max(leds.size.x, max(leds.size.y, leds.size.z));
      size = sqrt(size * size * 2) + 1;
      Coord3D offset = {(size - leds.size.x) / 2, (size - leds.size.y) / 2, 0};

      leds.size = Coord3D{size, size, 1};
    }

    data->midX = leds.size.x / 2;
    data->midY = leds.size.y / 2;
  }

  void addPixel(LedsLayer &leds, Coord3D &pixel) {
    RotateData *data = leds.projectionData.readWrite<RotateData>();

    if (data->expand) {
      int size = max(leds.size.x, max(leds.size.y, leds.size.z));
      size = sqrt(size * size * 2) + 1;
      Coord3D offset = {(size - leds.size.x) / 2, (size - leds.size.y) / 2, 0};

      pixel.x += offset.x;
      pixel.y += offset.y;
      pixel.z += offset.z;
    }

    DefaultProjection dp;
    dp.addPixel(leds, pixel);
  }

  void XYZ(LedsLayer &leds, Coord3D &pixel) {
    RotateData *data = leds.projectionData.readWrite<RotateData>();

    constexpr int Fixed_Scale = 1 << 10;

    if ((sys->now - data->lastUpdate > data->interval) && data->speed) { // Only update if the angle has changed
      data->lastUpdate = sys->now;
      // Increment the angle
      data->angle = data->reverse ? (data->angle <= 0 ? 359 : data->angle - 1) : (data->angle >= 359 ? 0 : data->angle + 1);
      
      if (data->alternate && (data->angle == 0)) data->reverse = !data->reverse;

      data->flip = (data->angle > 90 && data->angle < 270);

      int newAngle = data->angle; // Flip newAngle if needed. Don't change angle in data
      if (data->flip) {newAngle += 180; newAngle %= 360;}

      // Calculate shearX and shearY
      float angleRadians = radians(newAngle);
      data->shearX = -tan(angleRadians / 2) * Fixed_Scale;
      data->shearY =  sin(angleRadians)     * Fixed_Scale;
    }

    int maxX = leds.size.x;
    int maxY = leds.size.y;

    if (data->flip) {
      // Reverse x and y values
      pixel.x = maxX - pixel.x;
      pixel.y = maxY - pixel.y;
    }

    // Translate pixel to origin
    int dx = pixel.x - data->midX;
    int dy = pixel.y - data->midY;

    // Apply the 3 shear transformations
    int x1 = dx + data->shearX * dy / Fixed_Scale;
    int y1 = dy + data->shearY * x1 / Fixed_Scale;
    int x2 = x1 + data->shearX * y1 / Fixed_Scale;

    // Translate pixel back and assign
    pixel.x = x2 + data->midX;
    pixel.y = y1 + data->midY;
    pixel.z = 0;

    // Clamp the pixel to the bounds
    if      (pixel.x < 0)     pixel.x = 0;
    else if (pixel.x >= maxX) pixel.x = maxX - 1;
    if      (pixel.y < 0)     pixel.y = 0;
    else if (pixel.y >= maxY) pixel.y = maxY - 1;
  }
}; //RotateProjection