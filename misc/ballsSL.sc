import rand
define max_nb_balls 20
define rmax 4
define rmin 3
int nb_balls;
float vx[max_nb_balls];
float vy[max_nb_balls];
float xc[max_nb_balls];
float yc[max_nb_balls];
float r[max_nb_balls];
int color[max_nb_balls];

//CRGB cc;

void drawBall(float xc, float yc, float r, int c)
{
   int startx = (float)(xc - r);
   int r2 =(float) (r * r);
   float r4=r*r*r*r;
   // int endx=(float)(xc+r);
   int starty = (float)(yc - r);
   // int endy=(float)(yc+r);
   int _xc=xc;
   int _yc=yc;
  // CRGB *_leds=leds;
   for (int i = startx; i <= _xc; i++)
   {
      for (int j = starty; j <= _yc; j++)
      {
         int v;

         int distance = (float)((i - xc) * (i - xc) + (j - yc) * (j - yc));
  
         if (distance <= r2)
         {
            v = (float)(255 * (1 - distance * distance / (r4)));
            CRGB cc=hsv(c,255,v);
            sPC(i + j * panel_width, cc);
           // int h=(int)(2 * xc - i) + j * panel_width;
            sPC((int)(2 * xc - i) + j * panel_width,cc);
          // h=(int)(2 * xc - i) + (int)(2 * yc - j) * panel_widthsdfsdf;
            sPC((int)(2 * xc - i) + (int)(2 * yc - j) * panel_width,  cc);
           // h=i + (int)(2 * yc - j) * panel_width;
            sPC(i + (int)(2 * yc - j) * panel_width, cc);
         }
      }
   }
}

void updateBall(int index)
{
 float _r =r[index];
   float _xc = xc[index];
   float _yc = yc[index];
   float _vx = vx[index];
   float _vy = vy[index];

   _xc = _xc + _vx;
   _yc = _yc + _vy;
   if ((int)(_xc) >= (int)(width - _r - 1))
   {
      _xc =width - _r - 1;
      _vx = -_vx;
   }
   if ((int)(_xc) < (int)(_r + 1))
   {
      _xc=_r + 1;
      _vx = -_vx;
   }
   if ((int)(_yc) >= (int)(height - _r - 1))
   {
      _yc=height - _r - 1;
      _vy = -_vy;
   }
   if ((int)(_yc) < (int)(_r + 1))
   {
      _yc = _r + 1;
      _vy = -_vy;
   }

   xc[index] = _xc;
   yc[index] = _yc;
   vx[index] = _vx;
   vy[index] = _vy;
   int _color = color[index];
   drawBall(_xc, _yc, _r, _color);
}

void updateParams()
{
   nb_balls=slider1;
	if(nb_balls>max_nb_balls)
	{
		nb_balls=max_nb_balls;
	}
	if(nb_balls<=0)
	{
		nb_balls=1;
	}
}

int h;

void setup()
{
   for(int i=0;i<max_nb_balls;i++)
   {
      vx[i] = rand(280)/255+0.7;
      vy[i] = rand(280)/255+0.5;
      r[i] = (rmax-rmin)*(rand(280)/180) +rmin;
      xc[i] = width/2*(rand(280)/255+0.3)+15;
      yc[i] = height/2*(rand(280)/255+0.3)+15;
      
      color[i] = rand(255);
   }  

   h=0;
}

void loop()
{

    for(int i=0;i<width;i++)
    {
      for(int j=0;j<height;j++)
      {
         CRGB gg = hsv(i+h+j,255,180);
        sPC(i+panel_width*j, gg);
      }
   }

   updateParams();
    for (int i = 0; i < nb_balls; i++)
    {
      updateBall(i);
      // drawBall(1,1,1,CRGB(255,255,255));
    }

    h++;
}