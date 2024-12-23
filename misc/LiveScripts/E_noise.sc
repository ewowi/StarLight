//E_noise.sc

void setup()
{
}

void loop() {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      uint8_t pixelHue8 = inoise8(x * intensityControl, y * intensityControl, now / (16 - speedControl/16));
      // leds.setPixelColor(leds.XY(x, y), ColorFromPalette(leds.palette, pixelHue8));
      sCFP(y*width+x, pixelHue8, 255);
    }
  }
}