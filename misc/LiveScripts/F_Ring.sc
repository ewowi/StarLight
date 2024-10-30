//F_Ring.sc

define PI 3.141592654
define radius 200
define ledCount 100

void main()
{
  setFactor(10); //coordinates in mm
  for (int i=0; i<ledCount; i++) {
    int x = radius + sin(i/ledCount * 2 * PI) * radius;
    int y = radius + cos(i/ledCount * 2 * PI) * radius;
    //printf("x:%d y:%d\n", x, y);
    addPixel(x,y,0);
  }
}