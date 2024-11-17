//E_balls.sc

import rand
define max_nb_balls 25
define rmax 6
define rmin 4

struct ball {
   float vx;
   float vy;
   float xc;
   float yc;
   float r;
   int color;
   void drawBall( )
{
   int startx = xc - r;
   float r2 =r * r;
   float r4=r^4;
   int starty = yc - r;
   int _xc=xc;
   int _yc=yc;
   for (int i =startx; i <=_xc; i++)
   {
      for (int j = starty; j <= _yc; j++)
      {
         int v;

          float distance = (i - xc)^2+(j-yc)^2;
  
         if (distance <= r2)
         {
            v = 255 * (1 - distance^2 / (r4));
            CRGB cc=hsv(color,255,v);
            
               sPC(i + j * panel_width,cc);
               sPC((int)(2 * xc - i) + j * panel_width,cc);
               sPC((int)(2 * xc - i) + (int)(2 * yc - j) * panel_width,  cc);
               sPC(i + (int)(2 * yc - j) * panel_width, cc);
         }
      }
   }
 }

 void updateBall()
{

//drawBall();
//return;   
   xc = xc + vx;
   yc = yc + vy;
   if (xc >= width - r - 1)
   {
      xc =width - r - 1.1;
       vx = -vx;
   }
   if (xc < r + 1)
   {
      xc=r + 1.1;
      vx = -vx;

   }
   if (yc >= height - r - 1)
   {
      yc=height - r - 1.1;
      vy = -vy;
   }
   if (yc < r + 1)
   {
      yc = r + 1.1;
      vy = -vy;
    }

   drawBall();
}

}


ball Balls[max_nb_balls];
ball tmpball;

uint32_t h;

void setup()
{
   h = 1;

   for(int i=0;i<max_nb_balls;i++)
   {
      tmpball.vx = rand(300)/255+0.5;
      //dp(tmpball.vx);
      tmpball.vy = rand(280)/255+0.3;
      tmpball.r = (rmax-rmin)*(rand(280)/180) +rmin;
      tmpball.xc = width/2*(rand(280)/255+0.3)+15;
      tmpball.yc = height/2*(rand(280)/255+0.3)+15;
      
      tmpball.color = rand(255);
      Balls[i]=tmpball;
   }  
}

void loop()
{

   for(int i=0;i<width;i++)
   {
      for(int j=0;j<height;j++)
      {
         sPC(i+panel_width*j, hsv(i+h+j,255,180));
      }
   }
   for (int i = 0; i < slider1/10; i++)
   {
      Balls[i].updateBall();
   }

   sync();
   h++;
}