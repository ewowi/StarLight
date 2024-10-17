//F_test.sc

define width 8
define height 8
define depth 4
define nrOfPixels 256

void main()
{
  addPixelsPre(width,height,depth,nrOfPixels);
  for (int z=0; z<depth;z++) {
    for (int y=0; y<height; y++) {
      for (int x=0; x<width;x++) {
        addPixel(x*10,y*10,z*10);
      }
    }
  }
  addPixelsPost();
}