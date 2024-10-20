//animwleSL.sc

// WIP, By Yves but time and triangle is now external, not working yet
// Unexpected    at line:115
// preScript has 27 lines, so should be 88, or a but around but looks okay there 

define maxIterations 15
define scale 0.5
uint32_t __deltamillis[1];

uint32_t __baseTime[1];

float cR; // = -0.94299;
float cI;     // = 0.3162;

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
//  float jj=triangle(time(0.2));
//     dp(2.4*(jj-0.5));
//§§float g=2.4;
 //§ t1 = g*(triangle(time(0.2)) - 0.5);
//dp(t1);
  t1 = (triangle(time(0.2)) - 0.5)*2.4 ;
//dp(t1);
  t2 = time(0.05);

  cX = cR + t1;
  cY = cI + (t1 / 2.5);
  //  dp(cY);
// dp(cX);
//  dp(t1);
//  display(11111);
 // dp(t2);
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
    if ((int)(x2 + y2) >= 4)
    {
      break; 
    }
    fX = x2 - y2 + cX;
    fY = 2 * x * y + cY;
    x = fX;
    y = fY;
  }
 

  if (iter < maxIterations)
  {
    sPC(panel_width * y1 + x1, hsv(((t2 + (iter / maxIterations)) * 255), 255, 255));
  }
  else
  {
    sPC(panel_width * y1 + x1, hsv(0,0,0));
  } 
}

void setup()
{
  //clear();
  cR = -0.94299;
  cI = 0.3162;
  int h = 1 ;
}

void loop()
{
  beforeRender();
  for (int i = 0; i < width; i++)
  {
    for (int j = 0; j < height; j++)
    {
      render2D(i, j);
    }
  }
}