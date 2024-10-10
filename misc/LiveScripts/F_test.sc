//F_test.sc

void setup()
{
  addPixelsPre();
  for (int y=0; y<16; y++) {
    for (int x=0; x<16;x++) {
      addPixel(x*10,y*10,0);
    }
  }
  addPixelsPost();
}

void loop() {

}
