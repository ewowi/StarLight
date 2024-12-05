//E_animwle.sc

// WIP, By Yves but time and triangle is now external, not working yet

define maxIterations 15
define scale 0.5

uint32_t __deltamillis[1];

uint32_t __baseTime[1];

float cR; //= -0.94299; expecting external, __ASM__  or variable type  at line:39 position:10 10 
float cI = 0.3162;

float cX;
float cY;
float fX;
float fY;

// timers used to animate movement and color
float t1;
float t2;
int iter;
// UI

void beforeRender()
{

  t1 = (triangle(time(0.2)) - 0.5)*2.4 ;

  t2 = time(0.05);

  cX = cR + t1;
  cY = cI + (t1 / 2.5);
}

void render2D(int x1, int y1)
{
  float x = (x1/width-0.5)/scale;
  float y = (y1/height-0.5)/scale; 
  int iter;
  for (iter = 0; iter < maxIterations; iter++)
  {
    float x2 = x ^ 2;
    float y2 = y ^ 2;

    if (x2 + y2 >= 4) break; 
    
    fX = x2 - y2 + cX;
    fY = 2 * x * y + cY;
    x = fX;
    y = fY;
  }
 
  if (iter < maxIterations)
    sPC(width * y1 + x1, hsv((t2 + iter / maxIterations) * 255, 255, 255));
  else
    sPC(width * y1 + x1, hsv(0,0,0));
}

void setup()
{
   cR = -0.94299; //workaround see above
}

void loop()
{
  beforeRender();
  for (int i = 0; i < width; i++)
    for (int j = 0; j < height; j++)
      render2D(i, j);
}