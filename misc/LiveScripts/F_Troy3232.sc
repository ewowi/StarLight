//F_Troy3232.sc

define width 32
define height 32

void main()
{
  for (int x=width-1; x>=0; x--) {
    for (int y=0; y<height; y++)
      addPixel(x, (x%2)? height - 1 - y: y, 0);
    
    if (x==16) addPin(12); //right panel
    if (x==0)  addPin(4); //left panel
  }
}