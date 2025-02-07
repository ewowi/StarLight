//F_Cube202020.sc

#define width 20
#define height 20
#define depth 10

//int pins[16] = {22,21,19,18,5,4,2,15,13,12,14,27,26,25,33,32}; //optionally: 0,1,3,11,23 PHYSICAL_DRIVER on esp32-wrover (PSRAM)
int pins[10] = {22,21,14,18,5,4,2,15,13,12}; //optionally: 0,1,3,11,23 PHYSICAL_DRIVER on esp32-wrover (PSRAM)
//int pins[6] = {9,10,12,8,18,17}; //for esp32-S3

void main()
{
  ledSize = 2; //smaller leds (default 5)
  colorOrder = 1; //RGB (not for FastLED yet: see pio.ini)

  for (int z=0; z<depth;z++) {

    for (int x=0; x<width;x++)
      for (int y=0; y<height;y++)
        addPixel(x, y, z);

    addPin(pins[z]); //every curtain on one pin
  }
}