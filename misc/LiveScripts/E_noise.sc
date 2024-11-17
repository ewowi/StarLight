//E_noise.sc

//compile error: line:56 incorrect value 32 should be between 0 and 15

void setup()
{
}

void loop() {
  for (int y = 0; y < width; y++) {
    for (int x = 0; x < height; x++) {
      uint8_t pixelHue8 = inoise8(x * slider1, y * slider1, millis() / (16 - slider2));
      // leds.setPixelColor(leds.XY(x, y), ColorFromPalette(leds.palette, pixelHue8));
      //leds.setPixelColorPal(leds.XY(x, y), pixelHue8);
      sCFP(y*panel_width+x, pixelHue8, 255);
    }
  }
}