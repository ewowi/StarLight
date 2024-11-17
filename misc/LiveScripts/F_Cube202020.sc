//F_Cube202020.sc

define width 20
define height 20
define depth 20

void main()
{
  setLedSize(2); //smaller leds (default 5)
  for (int z=0; z<depth;z++) {
    for (int y=0; y<height; y++) {
      for (int x=0; x<width;x++) {
        addPixel(x,y,z);
      }
    }
  }
}