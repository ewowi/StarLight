//F_Cube888.sc

define width 8
define height 8
define depth 8

void main()
{
  for (int z=0; z<depth;z++)
    for (int y=0; y<height; y++)
      for (int x=0; x<width;x++)
        addPixel(x,y,z);
}