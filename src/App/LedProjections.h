/*
   @title     StarLight
   @file      LedProjections.h
   @date      20241014
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

//should not contain variables/bytes to keep mem as small as possible!!

class NoneProjection: public Projection {
  const char * name() {return "None";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

  void controls(LedsLayer &leds, JsonObject parentVar) {
  }
}; //NoneProjection

class DefaultProjection: public Projection {
  const char * name() {return "Default";}
  const char * tags() {return "💫";}

  public:

  void setup(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted, Coord3D &mapped, uint16_t &indexV) {
    if (leds.size == Coord3D{0,0,0}) {
      adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
      leds.size = sizeAdjusted;
    }
    adjustMapped(leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    indexV = leds.XYZUnprojected(mapped);
  }

  void adjustSizeAndPixel(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // ppf ("Default Projection %dD -> %dD Effect  Size: %d,%d,%d Pixel: %d,%d,%d ->", leds.projectionDimension, leds.effectDimension, sizeAdjusted.x, sizeAdjusted.y, sizeAdjusted.z, pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z);
    switch (leds.effectDimension) {
      case _1D: // effectDimension 1DxD
          sizeAdjusted.x = sqrt(sq(max(sizeAdjusted.x - midPosAdjusted.x, midPosAdjusted.x)) + 
                                sq(max(sizeAdjusted.y - midPosAdjusted.y, midPosAdjusted.y)) + 
                                sq(max(sizeAdjusted.z - midPosAdjusted.z, midPosAdjusted.z))) + 1;
          sizeAdjusted.y = 1;
          sizeAdjusted.z = 1;
          break;
      case _2D: // effectDimension 2D
          switch (leds.projectionDimension) {
              case _1D: // 2D1D
                  sizeAdjusted.x = sqrt(sizeAdjusted.x * sizeAdjusted.y * sizeAdjusted.z); // only one is > 1, square root
                  sizeAdjusted.y = sizeAdjusted.x * sizeAdjusted.y * sizeAdjusted.z / sizeAdjusted.x;
                  sizeAdjusted.z = 1;
                  break;
              case _2D: // 2D2D
                  // find the 2 axes
                  if (sizeAdjusted.x > 1) {
                      if (sizeAdjusted.y <= 1) {
                          sizeAdjusted.y = sizeAdjusted.z;
                      }
                  } else {
                      sizeAdjusted.x = sizeAdjusted.y;
                      sizeAdjusted.y = sizeAdjusted.z;
                  }
                  sizeAdjusted.z = 1;
                  break;
              case _3D: // 2D3D
                  sizeAdjusted.x = sizeAdjusted.x + sizeAdjusted.y / 2;
                  sizeAdjusted.y = sizeAdjusted.y / 2 + sizeAdjusted.z;
                  sizeAdjusted.z = 1;
                  break;
          }
          break;
      case _3D: // effectDimension 3D
          switch (leds.projectionDimension) {
              case _1D:
                  sizeAdjusted.x = std::pow(sizeAdjusted.x * sizeAdjusted.y * sizeAdjusted.z, 1/3); // only one is > 1, cube root
                  break;
              case _2D:
                  break;
              case _3D:
                  break;
          }
          break;
    }
    // ppf (" Size: %d,%d,%d Pixel: %d,%d,%d\n", sizeAdjusted.x, sizeAdjusted.y, sizeAdjusted.z, pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z);
  }

  void adjustMapped(LedsLayer &leds, Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted, Coord3D midPosAdjusted) {
    switch (leds.effectDimension) {
      case _1D: // effectDimension 1DxD
          mapped.x = pixelAdjusted.distance(midPosAdjusted);
          mapped.y = 0;
          mapped.z = 0;
          break;
      case _2D: // effectDimension 2D
          switch (leds.projectionDimension) {
              case _1D: // 2D1D
                  mapped.x = (pixelAdjusted.x + pixelAdjusted.y + pixelAdjusted.z) % leds.size.x; // only one > 0
                  mapped.y = (pixelAdjusted.x + pixelAdjusted.y + pixelAdjusted.z) / leds.size.x; // all rows next to each other
                  mapped.z = 0;
                  break;
              case _2D: // 2D2D
                  if (sizeAdjusted.x > 1) {
                      mapped.x = pixelAdjusted.x;
                      if (sizeAdjusted.y > 1) {
                          mapped.y = pixelAdjusted.y;
                      } else {
                          mapped.y = pixelAdjusted.z;
                      }
                  } else {
                      mapped.x = pixelAdjusted.y;
                      mapped.y = pixelAdjusted.z;
                  }
                  mapped.z = 0;
                  break;
              case _3D: // 2D3D
                  mapped.x = pixelAdjusted.x + pixelAdjusted.y / 2;
                  mapped.y = pixelAdjusted.y / 2 + pixelAdjusted.z;
                  mapped.z = 0;
                  break;
          }
          break;
      case _3D: // effectDimension 3D
          mapped = pixelAdjusted;
          break;
    }
    // ppf("Default %dD Effect -> %dD   %d,%d,%d -> %d,%d,%d\n", leds.effectDimension, leds.projectionDimension, pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z, mapped.x, mapped.y, mapped.z);
  }

}; //DefaultProjection

class PinwheelProjection: public Projection {
  const char * name() {return "Pinwheel";}
  const char * tags() {return "💫";}

  public:

  void setup(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted, Coord3D &mapped, uint16_t &indexV) {
    if (leds.size == Coord3D{0,0,0}) {
      adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
      leds.size = sizeAdjusted;
    }
    adjustMapped(leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    indexV = leds.XYZUnprojected(mapped);
  }

  void adjustSizeAndPixel(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    if (leds.size != Coord3D{0,0,0}) return; // Adjust only on first call
    leds.projectionData.begin();
    const int petals = leds.projectionData.read<uint8_t>();
    if (leds.projectionDimension > _1D && leds.effectDimension > _1D) {
      sizeAdjusted.y = sqrt(sq(max(sizeAdjusted.x - midPosAdjusted.x, midPosAdjusted.x)) + 
                            sq(max(sizeAdjusted.y - midPosAdjusted.y, midPosAdjusted.y))) + 1; // Adjust y before x
      sizeAdjusted.x = petals;
      sizeAdjusted.z = 1;
    }
    else {
      sizeAdjusted.x = petals;
      sizeAdjusted.y = 1;
      sizeAdjusted.z = 1;
    }
  }

  void adjustMapped(LedsLayer &leds, Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted, Coord3D midPosAdjusted) {
    // factors of 360
    const int FACTORS[24] = {360, 180, 120, 90, 72, 60, 45, 40, 36, 30, 24, 20, 18, 15, 12, 10, 9, 8, 6, 5, 4, 3, 2};
    // UI Variables
    leds.projectionData.begin();
    const int petals   = leds.projectionData.read<uint8_t>();
    const int swirlVal = leds.projectionData.read<uint8_t>() - 30; // SwirlVal range -30 to 30
    const bool reverse = leds.projectionData.read<bool>();
    const int symmetry = FACTORS[leds.projectionData.read<uint8_t>()-1];
    const int zTwist   = leds.projectionData.read<uint8_t>();
         
    const int dx = pixelAdjusted.x - midPosAdjusted.x;
    const int dy = pixelAdjusted.y - midPosAdjusted.y;
    const int swirlFactor = swirlVal == 0 ? 0 : hypot(dy, dx) * abs(swirlVal); // Only calculate if swirlVal != 0
    int angle = degrees(atan2(dy, dx)) + 180;  // 0 - 360
    
    if (swirlVal < 0) angle = 360 - angle; // Reverse Swirl

    int value = angle + swirlFactor + (zTwist * pixelAdjusted.z);
    float petalWidth = symmetry / float(petals);
    value /= petalWidth;
    value %= petals;

    if (reverse) value = petals - value - 1; // Reverse Movement

    mapped.x = value;
    mapped.y = 0;
    if (leds.effectDimension > _1D && leds.projectionDimension > _1D) {
      mapped.y = int(sqrt(sq(dx) + sq(dy))); // Round produced blank pixel
    }
    mapped.z = 0;

    // if (pixelAdjusted.x == 0 && pixelAdjusted.y == 0 && pixelAdjusted.z == 0) ppf("Pinwheel  Center: (%d, %d) SwirlVal: %d Symmetry: %d Petals: %d zTwist: %d\n", midPosAdjusted.x, midPosAdjusted.y, swirlVal, symmetry, petals, zTwist);
    // ppf("pixelAdjusted %2d,%2d,%2d -> %2d,%2d,%2d Angle: %3d Petal: %2d\n", pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z, mapped.x, mapped.y, mapped.z, angle, value);
  }

  void controls(LedsLayer &leds, JsonObject parentVar) {
    uint8_t *petals   = leds.projectionData.write<uint8_t>(60); // Initalize petal first for adjustSizeAndPixel
    uint8_t *swirlVal = leds.projectionData.write<uint8_t>(30);
    bool    *reverse  = leds.projectionData.write<bool>(false);
    uint8_t *symmetry = leds.projectionData.write<uint8_t>(1);
    uint8_t *zTwist   = leds.projectionData.write<uint8_t>(0);

    ui->initSlider(parentVar, "Swirl", swirlVal, 0, 60, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
      if (rowNr < leds.fixture->layers.size())
        leds.fixture->layers[rowNr]->triggerMapping();
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "Reverse", reverse, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
      if (rowNr < leds.fixture->layers.size())
        leds.fixture->layers[rowNr]->triggerMapping();
        return true;
      default: return false;
    }});
    // Testing zTwist range 0 to 42 arbitrary values for testing. Hide if not 3D fixture. Select pinwheel while using 3D fixture.
    if (leds.projectionDimension == _3D) {
      ui->initSlider(parentVar, "Z Twist", zTwist, 0, 42, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
        case onChange:
        if (rowNr < leds.fixture->layers.size())
          leds.fixture->layers[rowNr]->triggerMapping();
          return true;
        default: return false;
      }});
    }
    // Rotation symmetry. Uses factors of 360.
    ui->initSlider(parentVar, "Rotational Symmetry", symmetry, 1, 23, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
      if (rowNr < leds.fixture->layers.size())
        leds.fixture->layers[rowNr]->triggerMapping();
        return true;
      default: return false;
    }});
    // Naming petals, arms, blades, rays? Controls virtual strip length.
    ui->initSlider(parentVar, "Petals", petals, 1, 60, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
      if (rowNr < leds.fixture->layers.size())
        leds.fixture->layers[rowNr]->triggerMapping();
        return true;
      default: return false;
    }});
  }
}; //PinwheelProjection

class MultiplyProjection: public Projection {
  const char * name() {return "Multiply";}
  const char * tags() {return "💫";}

  public:

  void setup(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted, Coord3D &mapped, uint16_t &indexV) {
    adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    DefaultProjection dp;
    dp.setup(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted, mapped, indexV);
  }

  void adjustSizeAndPixel(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // UI Variables
    leds.projectionData.begin();
    Coord3D proMulti = leds.projectionData.read<Coord3D>();
    bool    mirror   = leds.projectionData.read<bool>();

    proMulti = proMulti.maximum(Coord3D{1, 1, 1}); // {1, 1, 1} is the minimum value
    if (proMulti == Coord3D{1, 1, 1}) return;      // No need to adjust if proMulti is {1, 1, 1}
    
    sizeAdjusted = (sizeAdjusted + proMulti - Coord3D({1,1,1})) / proMulti; // Round up
    midPosAdjusted /= proMulti;

    if (mirror) {
      Coord3D mirrors = pixelAdjusted / sizeAdjusted; // Place the pixel in the right quadrant
      pixelAdjusted = pixelAdjusted % sizeAdjusted;
      if (mirrors.x %2 != 0) pixelAdjusted.x = sizeAdjusted.x - 1 - pixelAdjusted.x;
      if (mirrors.y %2 != 0) pixelAdjusted.y = sizeAdjusted.y - 1 - pixelAdjusted.y;
      if (mirrors.z %2 != 0) pixelAdjusted.z = sizeAdjusted.z - 1 - pixelAdjusted.z;
    }
    else pixelAdjusted = pixelAdjusted % sizeAdjusted;
  }

  void controls(LedsLayer &leds, JsonObject parentVar) {
    Coord3D *proMulti = leds.projectionData.write<Coord3D>({2,2,1});
    bool *mirror = leds.projectionData.write<bool>(false);
    ui->initCoord3D(parentVar, "proMulti", proMulti, 0, 10, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
        if (rowNr < leds.fixture->layers.size()) {
          leds.fixture->layers[rowNr]->triggerMapping();
        }
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "mirror", mirror, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
        if (rowNr < leds.fixture->layers.size()) {
          leds.fixture->layers[rowNr]->triggerMapping();
        }
        return true;
      default: return false;
    }});
  }
}; //MultiplyProjection

class TiltPanRollProjection: public Projection {
  const char * name() {return "TiltPanRoll";}
  const char * tags() {return "💫";}

  public:

  void setup(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted, Coord3D &mapped, uint16_t &indexV) {
    // adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted); // Uncomment to expand grid to fill corners
    DefaultProjection dp;
    dp.setup(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted, mapped, indexV);
  }

  void adjustSizeAndPixel(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    uint8_t size = max(sizeAdjusted.x, max(sizeAdjusted.y, sizeAdjusted.z));
    size = sqrt(size * size * 2) + 1;
    Coord3D offset = {(size - sizeAdjusted.x) / 2, (size - sizeAdjusted.y) / 2, 0};
    sizeAdjusted = Coord3D{size, size, 1};

    pixelAdjusted.x += offset.x;
    pixelAdjusted.y += offset.y;
    pixelAdjusted.z += offset.z;
  }

  void adjustXYZ(LedsLayer &leds, Coord3D &pixel) {
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

  void controls(LedsLayer &leds, JsonObject parentVar) {
    //tbd: implement variable by reference for rowNrs
    #ifdef STARBASE_USERMOD_MPU6050
      ui->initCheckBox(parentVar, "gyro", false, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
        case onChange:
          if (rowNr < leds.fixture->layers.size())
            leds.fixture->layers[rowNr]->proGyro = mdl->getValue(var, rowNr);
          return true;
        default: return false;
      }});
    #endif
    ui->initSlider(parentVar, "tilt", 128, 0, 254, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
        if (rowNr < leds.fixture->layers.size())
          leds.fixture->layers[rowNr]->proTiltSpeed = mdl->getValue(var, rowNr);
        return true;
      default: return false;
    }});
    ui->initSlider(parentVar, "pan", 128, 0, 254, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
        if (rowNr < leds.fixture->layers.size())
          leds.fixture->layers[rowNr]->proPanSpeed = mdl->getValue(var, rowNr);
        return true;
      default: return false;
    }});
    ui->initSlider(parentVar, "roll", 128, 0, 254, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onUI:
        ui->setComment(var, "Roll speed");
        return true;
      case onChange:
        if (rowNr < leds.fixture->layers.size())
          leds.fixture->layers[rowNr]->proRollSpeed = mdl->getValue(var, rowNr);
        return true;
      default: return false;
    }});
  }
}; //TiltPanRollProjection

class DistanceFromPointProjection: public Projection {
  const char * name() {return "Distance ⌛";}
  const char * tags() {return "💫";}

  public:

  void setup(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted, Coord3D &mapped, uint16_t &indexV) {
    DefaultProjection dp;
    dp.setup(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted, mapped, indexV);
    if (leds.projectionDimension == _2D && leds.effectDimension == _2D) postProcessing(leds, indexV);
  }

  void postProcessing(LedsLayer &leds, uint16_t &indexV) {
    //2D2D: inverse mapping
    Trigo trigo(leds.size.x-1); // 8 bits trigo with period leds.size.x-1 (currentl Float trigo as same performance)
    float minDistance = 10;
    // ppf("checking indexV %d\n", indexV);
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
        if (indexV == leds.XY(x2New, y2New)) { //(uint8_t)xNew + (uint8_t)yNew * size.x) {
          // ppf("  found one %d => %d=%d+%d*%d (%f+%f*%d) [%f]\n", indexV, x+y*size.x, x,y, size.x, xNew, yNew, size.x, distance);
          indexV = leds.XY(x, y);

          if (indexV%10 == 0) ppf("."); //show some progress as this projection is slow (Need S007 to optimize ;-)
                                      
          minDistance = 0.0f; // stop looking further
        }
      }
    }
    if (minDistance > 0.5f) indexV = UINT16_MAX; //do not show this pixel
  }
}; //DistanceFromPointProjection

class Preset1Projection: public Projection {
  const char * name() {return "Preset1";}
  const char * tags() {return "💫";}

  void setup(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted, Coord3D &mapped, uint16_t &indexV) {
    adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    DefaultProjection dp;
    dp.setup(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted, mapped, indexV);
  }

  void adjustSizeAndPixel(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    MultiplyProjection mp;
    mp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
  }

  void adjustXYZ(LedsLayer &leds, Coord3D &pixel) {
    TiltPanRollProjection tp;
    tp.adjustXYZ(leds, pixel);
  }

  void controls(LedsLayer &leds, JsonObject parentVar) {
    MultiplyProjection mp;
    mp.controls(leds, parentVar);
    TiltPanRollProjection tp;
    tp.controls(leds, parentVar);
  }
}; //Preset1Projection

class RandomProjection: public Projection {
  const char * name() {return "Random";}
  const char * tags() {return "💫";}

  void controls(LedsLayer &leds, JsonObject parentVar) {
  }
}; //RandomProjection

class ReverseProjection: public Projection {
  const char * name() {return "Reverse";}
  const char * tags() {return "💡";}

  public:

  void setup(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted, Coord3D &mapped, uint16_t &indexV) { 
    adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    DefaultProjection dp;
    dp.setup(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted, mapped, indexV);
  }

  void adjustSizeAndPixel(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // UI Variables
    leds.projectionData.begin();
    bool reverseX = leds.projectionData.read<bool>();
    bool reverseY = leds.projectionData.read<bool>();
    bool reverseZ = leds.projectionData.read<bool>();

    if (reverseX) pixelAdjusted.x = sizeAdjusted.x - pixelAdjusted.x - 1;
    if (reverseY) pixelAdjusted.y = sizeAdjusted.y - pixelAdjusted.y - 1;
    if (reverseZ) pixelAdjusted.z = sizeAdjusted.z - pixelAdjusted.z - 1;
  }

  void controls(LedsLayer &leds, JsonObject parentVar) {
    bool *reverseX = leds.projectionData.write<bool>(false);
    bool *reverseY = leds.projectionData.write<bool>(false);
    bool *reverseZ = leds.projectionData.write<bool>(false);

    ui->initCheckBox(parentVar, "reverse X", reverseX, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->layers[rowNr]->triggerMapping();
        return true;
      default: return false;
    }});
    if (leds.effectDimension >= _2D || leds.projectionDimension >= _2D) {
      ui->initCheckBox(parentVar, "reverse Y", reverseY, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
        case onChange:
          leds.fixture->layers[rowNr]->triggerMapping();
          return true;
        default: return false;
      }});
    }
    if (leds.effectDimension == _3D || leds.projectionDimension == _3D) {
      ui->initCheckBox(parentVar, "reverse Z", reverseZ, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
        case onChange:
          leds.fixture->layers[rowNr]->triggerMapping();
          return true;
        default: return false;
      }});
    }
  }
}; //ReverseProjection

class MirrorProjection: public Projection {
  const char * name() {return "Mirror";}
  const char * tags() {return "💡";}

  public:

  void setup(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted, Coord3D &mapped, uint16_t &indexV) {
    adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    DefaultProjection dp;
    dp.setup(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted, mapped, indexV);
  }

  void adjustSizeAndPixel(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // UI Variables
    leds.projectionData.begin();
    bool mirrorX = leds.projectionData.read<bool>();
    bool mirrorY = leds.projectionData.read<bool>();
    bool mirrorZ = leds.projectionData.read<bool>();

    if (mirrorX) {
      if (pixelAdjusted.x >= sizeAdjusted.x / 2) pixelAdjusted.x = sizeAdjusted.x - 1 - pixelAdjusted.x;
      sizeAdjusted.x = (sizeAdjusted.x + 1) / 2;
    }
    if (mirrorY) {
      if (pixelAdjusted.y >= sizeAdjusted.y / 2) pixelAdjusted.y = sizeAdjusted.y - 1 - pixelAdjusted.y;
      sizeAdjusted.y = (sizeAdjusted.y + 1) / 2;
    }
    if (mirrorZ) {
      if (pixelAdjusted.z >= sizeAdjusted.z / 2) pixelAdjusted.z = sizeAdjusted.z - 1 - pixelAdjusted.z;
      sizeAdjusted.z = (sizeAdjusted.z + 1) / 2;
    }
}

  void controls(LedsLayer &leds, JsonObject parentVar) {
    bool *mirrorX = leds.projectionData.write<bool>(false);
    bool *mirrorY = leds.projectionData.write<bool>(false);
    bool *mirrorZ = leds.projectionData.write<bool>(false);
    ui->initCheckBox(parentVar, "mirror X", mirrorX, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->layers[rowNr]->triggerMapping();
        return true;
      default: return false;
    }});
    if (leds.projectionDimension >= _2D) {
      ui->initCheckBox(parentVar, "mirror Y", mirrorY, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
        case onChange:
          leds.fixture->layers[rowNr]->triggerMapping();
          return true;
        default: return false;
      }});
    }
    if (leds.projectionDimension == _3D) {
      ui->initCheckBox(parentVar, "mirror Z", mirrorZ, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
        case onChange:
          leds.fixture->layers[rowNr]->triggerMapping();
          return true;
        default: return false;
      }});
    }
  }
}; //MirrorProjection

class GroupingProjection: public Projection {
  const char * name() {return "Grouping";}
  const char * tags() {return "💡";}

  public:

  void setup(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted, Coord3D &mapped, uint16_t &indexV) {
    adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    DefaultProjection dp;
    dp.setup(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted, mapped, indexV);
  }

  void adjustSizeAndPixel(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // UI Variables
    leds.projectionData.begin();
    Coord3D grouping = leds.projectionData.read<Coord3D>();
    grouping = grouping.maximum(Coord3D{1, 1, 1}); // {1, 1, 1} is the minimum value
    if (grouping == Coord3D{1, 1, 1}) return;

    midPosAdjusted /= grouping;
    pixelAdjusted /= grouping;

    sizeAdjusted = (sizeAdjusted + grouping - Coord3D{1,1,1}) / grouping; // round up
  }

  void controls(LedsLayer &leds, JsonObject parentVar) {
    Coord3D *grouping = leds.projectionData.write<Coord3D>({2,2,2});
    ui->initCoord3D(parentVar, "Grouping", grouping, 0, 100, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->layers[rowNr]->triggerMapping();
        return true;
      default: return false;
    }});
  }
}; //GroupingProjection

class SpacingProjection: public Projection {
  const char * name() {return "Spacing WIP";}
  const char * tags() {return "💡";}

  void setup(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted, Coord3D &mapped, uint16_t &indexV) {
    adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    DefaultProjection dp;
    dp.setup(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted, mapped, indexV);
  }

  void adjustSizeAndPixel(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // UI Variables
    leds.projectionData.begin();
    Coord3D spacing = leds.projectionData.read<Coord3D>();

    // ppf ("pixel: %d,%d,%d -> ", pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z);
    spacing = spacing.maximum(Coord3D{0, 0, 0}) + Coord3D{1,1,1}; // {1, 1, 1} is the minimum value

    if (pixelAdjusted % spacing == Coord3D{0,0,0}) pixelAdjusted /= spacing; 
    else pixelAdjusted = Coord3D{UINT16_MAX, UINT16_MAX, UINT16_MAX};

    // ppf ("%d,%d,%d\n", pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z);

    sizeAdjusted = (sizeAdjusted + spacing - Coord3D{1,1,1}) / spacing; // round up
    midPosAdjusted /= spacing;
  }

  void controls(LedsLayer &leds, JsonObject parentVar) {
    Coord3D *spacing = leds.projectionData.write<Coord3D>({1,1,1});
    ui->initCoord3D(parentVar, "Spacing", spacing, 0, 100, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->layers[rowNr]->triggerMapping();
        return true;
      default: return false;
    }});
  }
}; //SpacingProjection

class TransposeProjection: public Projection {
  const char * name() {return "Transpose";}
  const char * tags() {return "💡";}

  public:

  void setup(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted, Coord3D &mapped, uint16_t &indexV) {
    adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    DefaultProjection dp;
    dp.setup(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted, mapped, indexV);
  }

  void adjustSizeAndPixel(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // UI Variables
    leds.projectionData.begin();
    bool transposeXY = leds.projectionData.read<bool>();
    bool transposeXZ = leds.projectionData.read<bool>();
    bool transposeYZ = leds.projectionData.read<bool>();

    if (transposeXY) { int temp = pixelAdjusted.x; pixelAdjusted.x = pixelAdjusted.y; pixelAdjusted.y = temp; }
    if (transposeXZ) { int temp = pixelAdjusted.x; pixelAdjusted.x = pixelAdjusted.z; pixelAdjusted.z = temp; }
    if (transposeYZ) { int temp = pixelAdjusted.y; pixelAdjusted.y = pixelAdjusted.z; pixelAdjusted.z = temp; }
  }

  void controls(LedsLayer &leds, JsonObject parentVar) {
    bool *transposeXY = leds.projectionData.write<bool>(false);
    bool *transposeXZ = leds.projectionData.write<bool>(false);
    bool *transposeYZ = leds.projectionData.write<bool>(false);

    ui->initCheckBox(parentVar, "transpose XY", transposeXY, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->layers[rowNr]->triggerMapping();
        return true;
      default: return false;
    }});
    if (leds.effectDimension == _3D) {
      ui->initCheckBox(parentVar, "transpose XZ", transposeXZ, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
        case onChange:
          leds.fixture->layers[rowNr]->triggerMapping();
          return true;
        default: return false;
      }});
      ui->initCheckBox(parentVar, "transpose YZ", transposeYZ, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
        case onChange:
          leds.fixture->layers[rowNr]->triggerMapping();
          return true;
        default: return false;
      }});
    }
  }
}; //TransposeProjection

class KaleidoscopeProjection: public Projection {
  const char * name() {return "Kaleidoscope WIP";}
  const char * tags() {return "💫";}

  void controls(LedsLayer &leds, JsonObject parentVar) {
  }
}; //KaleidoscopeProjection

class ScrollingProjection: public Projection {
  const char * name() {return "Scrolling WIP";}
  const char * tags() {return "💫";}

  void setup(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted, Coord3D &mapped, uint16_t &indexV) {
    MirrorProjection mp;
    mp.setup(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted, mapped, indexV);
  }

  void adjustXYZ(LedsLayer &leds, Coord3D &pixel) {
    leds.projectionData.begin();
    bool mirrorX = leds.projectionData.read<bool>(); // Not used 
    bool mirrorY = leds.projectionData.read<bool>(); // Not used
    bool mirrorZ = leds.projectionData.read<bool>(); // Not used

    uint8_t xSpeed = leds.projectionData.read<uint8_t>();
    uint8_t ySpeed = leds.projectionData.read<uint8_t>();
    uint8_t zSpeed = leds.projectionData.read<uint8_t>();

    if (xSpeed) pixel.x = (pixel.x + (sys->now * xSpeed / 255 / 100)) % leds.size.x;
    if (ySpeed) pixel.y = (pixel.y + (sys->now * ySpeed / 255 / 100)) % leds.size.y;
    if (zSpeed) pixel.z = (pixel.z + (sys->now * zSpeed / 255 / 100)) % leds.size.z;
  }

  void controls(LedsLayer &leds, JsonObject parentVar) {
    MirrorProjection mp;
    mp.controls(leds, parentVar);

    uint8_t *xSpeed  = leds.projectionData.write<uint8_t>(128);
    uint8_t *ySpeed  = leds.projectionData.write<uint8_t>(0);
    uint8_t *zSpeed  = leds.projectionData.write<uint8_t>(0);

    ui->initSlider(parentVar, "X Speed", xSpeed, 0, 255, false);
    //ewowi: 2D/3D inits will be done automatically in the future, then the if's are not needed here
    if (leds.projectionDimension >= _2D) ui->initSlider(parentVar, "Y Speed", ySpeed, 0, 255, false);
    if (leds.projectionDimension == _3D) ui->initSlider(parentVar, "Z Speed", zSpeed, 0, 255, false);
  }

}; //ScrollingProjection

class AccelerationProjection: public Projection {
  const char * name() {return "Acceleration WIP";}
  const char * tags() {return "💫🧭";}

  public:

  void setup(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted, Coord3D &mapped, uint16_t &indexV) {
    DefaultProjection dp;
    dp.setup(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted, mapped, indexV);
  }

  void adjustXYZ(LedsLayer &leds, Coord3D &pixel) {
    leds.projectionData.begin();
    bool wrap = leds.projectionData.read<bool>();
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

  void controls(LedsLayer &leds, JsonObject parentVar) {
    bool *wrap = leds.projectionData.write<bool>(false);
    uint8_t *sensitivity = leds.projectionData.write<uint8_t>(0);
    uint8_t *deadzone = leds.projectionData.write<uint8_t>(10);

    ui->initCheckBox(parentVar, "Wrap", wrap);
    ui->initSlider(parentVar, "Sensitivity", sensitivity, 0, 100, false);
    ui->initSlider(parentVar, "Deadzone", deadzone, 0, 100, false);
  }
}; //Acceleration

class CheckerboardProjection: public Projection {
  const char * name() {return "Checkerboard";}
  const char * tags() {return "💫";}

  void setup(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted, Coord3D &mapped, uint16_t &indexV) {
    leds.projectionData.begin();
    Coord3D size = leds.projectionData.read<Coord3D>();
    bool invert = leds.projectionData.read<bool>();
    bool group = leds.projectionData.read<bool>();
    adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    if (group) {GroupingProjection gp; gp.setup(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted, mapped, indexV);}
    else       {DefaultProjection  dp; dp.setup(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted, mapped, indexV);}
  }

  void adjustSizeAndPixel(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // UI Variables
    leds.projectionData.begin();
    Coord3D size = leds.projectionData.read<Coord3D>();
    bool invert = leds.projectionData.read<bool>();

    // ppf ("pixel: %d,%d,%d -> ", pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z);
    size = size.maximum(Coord3D{1, 1, 1}); // {1, 1, 1} is the minimum value

    Coord3D check = pixelAdjusted / size;
    if ((check.x + check.y + check.z) % 2 == 0) {
      if (invert) pixelAdjusted = {UINT16_MAX, UINT16_MAX, UINT16_MAX};
    }
    else {
      if (!invert) pixelAdjusted = {UINT16_MAX, UINT16_MAX, UINT16_MAX};
    }
      
    // ppf ("%d,%d,%d", pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z);
    // ppf (" Check: %d,%d,%d  Even: %d\n", check.x, check.y, check.z, (check.x + check.y + check.z) % 2 == 0);
  }

  void controls(LedsLayer &leds, JsonObject parentVar) {
    Coord3D *size = leds.projectionData.write<Coord3D>({3,3,3});
    bool *invert = leds.projectionData.write<bool>(false);
    bool *group = leds.projectionData.write<bool>(false);
    ui->initCoord3D(parentVar, "Square Size", size, 0, 100, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->layers[rowNr]->triggerMapping();
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "Invert", invert, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->layers[rowNr]->triggerMapping();
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "Group", group, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->layers[rowNr]->triggerMapping();
        return true;
      default: return false;
    }});
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

  void setup(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted, Coord3D &mapped, uint16_t &indexV) {
    adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    DefaultProjection dp;
    dp.setup(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted, mapped, indexV);
  }

  void adjustSizeAndPixel(LedsLayer &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    leds.projectionData.begin();
    RotateData *data = leds.projectionData.readWrite<RotateData>();
    if (leds.size == Coord3D{0, 0, 0}) {
      data->expand = mdl->getValue("projection", "Expand");
    }
    if (data->expand) {
      uint8_t size = max(sizeAdjusted.x, max(sizeAdjusted.y, sizeAdjusted.z));
      size = sqrt(size * size * 2) + 1;
      Coord3D offset = {(size - sizeAdjusted.x) / 2, (size - sizeAdjusted.y) / 2, 0};
      sizeAdjusted = Coord3D{size, size, 1};
      pixelAdjusted.x += offset.x;
      pixelAdjusted.y += offset.y;
      pixelAdjusted.z += offset.z;
    }
    if (leds.size == Coord3D{0, 0, 0}) {
      data->midX = sizeAdjusted.x / 2;
      data->midY = sizeAdjusted.y / 2;
      return;
    }
  }

  void adjustXYZ(LedsLayer &leds, Coord3D &pixel) {
    leds.projectionData.begin();
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

  void controls(LedsLayer &leds, JsonObject parentVar) {
    RotateData *data = leds.projectionData.readWrite<RotateData>();

    ui->initSelect(parentVar, "Direction", (uint8_t)0, false, [data](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onUI: {
        JsonArray options = ui->setOptions(var);
        options.add("Clockwise");
        options.add("Counter-Clockwise");
        options.add("Alternate");
        return true; }
      case onChange: {
        uint8_t val = mdl->getValue(var, rowNr);
        if (val == 0) data->reverse = false;
        if (val == 1) data->reverse = true;
        if (val == 2) data->alternate = true; else data->alternate = false;
        return true; }
      default: return false;
    }});
    ui->initSlider(parentVar, "Rotate Speed", 128, 0, 254, false, [data](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
        data->speed = mdl->getValue(var, rowNr);
        data->interval = 1000 / (data->speed + 1);
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "Expand", false, false, [&leds](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->layers[rowNr]->triggerMapping();
        return true;
      default: return false;
    }});
  }
}; //RotateProjection

class TestProjection: public Projection {
  const char * name() {return "Test";}
  const char * tags() {return "💡";}

  void controls(LedsLayer &leds, JsonObject parentVar) {
  }
}; //TestProjection