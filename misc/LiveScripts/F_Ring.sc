//F_Ring.sc

define PI 3.141592654
define radius 200
define ledCount 100

void main()
{
  addPixelsPre(radius * 2 / 10, radius * 2 / 10, 1, ledCount, 5, 0);
  for (int i=0; i<ledCount; i++) {
    int x = radius + sin(i/ledCount * 2 * PI) * radius;
    int y = radius + cos(i/ledCount * 2 * PI) * radius;
    printf("x:%d y:%d\n", x, y);
    addPixel(x,y,0);
  }
  addPixelsPost();
}