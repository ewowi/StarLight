//F_Cube202020.sc

define width 20
define height 20
define depth 6

int pins[6] = {32,33,25,26,27,14}; //STARLIGHT_CLOCKLESS_LED_DRIVER on esp32-wrover (PSRAM)
//int pins[6] = {9,10,12,8,18,17}; //for esp32-S3

void main()
{
  ledSize = 2; //smaller leds (default 5)
  colorOrder = 1; //RGB (not for FastLED yet: see pio.ini)

  for (int z=0; z<depth;z++) {

    for (int x=0; x<width;x++)
      for (int y=0; y<height;y++)
        addPixel(x, y, z);

    addPin(pins[z]); //every 8 panels one pin on the esp32
  }
}