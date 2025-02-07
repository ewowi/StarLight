//F_panel4020.sc

#define width 40
#define height 20
#define depth 1

void main()
{
  for (int z=0; z<depth;z++)
    for (int y=0; y<height; y++)
      for (int x=0; x<width;x++)
        addPixel(x,y,z);
}