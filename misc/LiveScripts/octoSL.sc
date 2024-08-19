//octoSL.sc

//import memset
//save_reg
define PI 3.1415926535
define speed 1
int nb_branches;
uint8_t C_X ;
uint8_t C_Y;
uint8_t mapp;
uint8_t rMapRadius[NUM_LEDS];
uint8_t rMapAngle[NUM_LEDS];
uint32_t t; //=speed

void setup()
{
  t = 0;
  C_X = width / 2;
  C_Y = height / 2;
  mapp = 255 / width;
  for (int x = -C_X; x < C_X + (width % 2); x++) {
    for (int y = -C_Y; y < C_Y + (height % 2); y++) {

      float h=128*(atan2(y, x)/PI);
      rMapAngle[(x + C_X) *height+y + C_Y]= (int)(h);
      h=hypot(x,y)*mapp;
      rMapRadius[(x + C_X)*height +y + C_Y] = (int)(h); //thanks Sutaburosu
    }
  }
}


void loop() {

  nb_branches = slider1 / 25;

  // memset(leds, 0, width * height * 3);
  for (uint8_t x = 0; x < width; x++) {
    for (uint8_t y = 0; y < height; y++) {
      uint8_t angle = rMapAngle[x*height+y];
      uint8_t radius = rMapRadius[x*height+y];

      // leds[y*panel_width+x] = hsv(t + radius, 255, sin8(t*4+sin8(t * 4 - radius)+angle*nb_branches));
      // int h=sin8(t*4+sin8(t * 4 - radius)+angle*nb_branches);
      // leds[y*panel_width+x] = hsv(t + radius, 255, h);

      CRGB gg = hsv(t + radius, 255, sin8(t*4+sin8(t * 4 - radius)+angle*nb_branches));
      sPC(y*panel_width+x, gg);
    }
  }
  t=t+speed;
  //delay(16);
}