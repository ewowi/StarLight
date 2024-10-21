//F_panel2040.sc

define width 20
define height 40
define depth 1

void main()
{
  addPixelsPre(width,height,depth,width * height * depth, 5, 0);
  for (int z=0; z<depth;z++) {
    for (int y=0; y<height; y++) {
      for (int x=0; x<width;x++) {
        addPixel(x*10,y*10,z*10);
      }
    }
  }
  addPixelsPost();
}