//E_noise.sc

//compile error: line:56 incorrect value 32 should be between 0 and 15

void setup()
{
}

void loop() {
  for (int y = 0; y < width; y++) {
    for (int x = 0; x < height; x++) {
      uint8_t pixelHue8 = inoise8(x * custom1Control, y * custom1Control, millis() / (16 - custom2Control));
      // leds.setPixelColor(leds.XY(x, y), ColorFromPalette(leds.palette, pixelHue8));
      //leds.setPixelColorPal(leds.XY(x, y), pixelHue8);
      sCFP(y*panel_width+x, pixelHue8, 255);
    }
  }
}