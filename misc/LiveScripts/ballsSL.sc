
import rand
define rmax 8
define rmin 8

define max_nb_balls 20
int nb_balls;

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
      int r2 =r * r;
      float r4=r^4;
      int starty = yc - r;
      int _xc=xc;
      int _yc=yc;
      for (int i =startx; i <=_xc; i++)
      {
         for (int j = starty; j <= _yc; j++)
         {
            int v;

            int distance = (i - xc)^2+(j-yc)^2;
   
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
      
      xc = xc + vx;
      yc = yc + vy;
      if (xc >= width - r - 1)
      {
         xc =width - r - 1;
         vx = -vx;
      }
      if (xc < r + 1)
      {
         xc=r + 1;
         vx = -vx;

      }
      if (yc >= height - r - 1)
      {
         yc=height - r - 1;
         vy = -vy;
      }
      if (yc < r + 1)
      {
         yc = r + 1;
         vy = -vy;
      }

      drawBall();
   }
}


ball Balls[max_nb_balls];
ball tmpball;

void setup()
{
   for(int i=0;i<nb_balls;i++)
   {
      tmpball.vx = rand(300)/255+0.7;
      tmpball.vy = rand(280)/255+0.5;
      tmpball.r = (rmax-rmin)*(rand(280)/180) +rmin;
      tmpball.xc = width/2*(rand(280)/255+0.3);
      tmpball.yc = height/2*(rand(280)/255+0.3);
      
      tmpball.color = rand(255);
      Balls[i]=tmpball;
   }  
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

void loop()
{
   uint32_t h = 1;
  
   while (h > 0)
   {

     for(int i=0;i<width;i++)
     {
        for(int j=0;j<height;j++)
        {
            sPC(i+panel_width*j, hsv(i+h+j,255,180));
        }
     }
       updateParams();
      for (int i = 0; i < nb_balls; i++)
      {
         Balls[i].updateBall();
      }

     // sync();
      h++;
   }
}