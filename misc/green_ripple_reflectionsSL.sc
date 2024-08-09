//https://github.com/MoonModules/WLED-Effects/blob/master/ARTIFX/wled/green_ripple_reflections.wled
//original from https://patterns.electromage.com/pattern/Ktjben4j36Wqxnk8N
//Converted by Andrew Tuline and Ewoud Wijma

float PI2;
float PI6;
float PI10;

float t1;
float t2;
float t3;

float a;
float b;
float c;
float v;

void setup()
{
  PI2 =  6.28318;
  PI6 =  PI2 * 3;
  PI10 = PI2 * 5;
}


void loop() {

  t1 = time(0.03) * PI2;
  t2 = time(0.05) * PI2;
  t3 = time(0.04) * PI2;

  for (uint8_t index = 0; index < width; index++) {
    a = sin(index * PI10 / width + t1);
    a = a * a;
    b = sin(index * PI6 / width - t2);
    c = triangle(index * 3 / width + 1 + sin(t3) / 2 % 1);
    v = (a + b + c) / 3;
    v = v * v;
    //printCustom((float)index,a,b,c,v);
    CRGB gg = hsv(0.3 * 255, a * 255, v * 255);
    for (uint8_t y = 0; y < height; y++) {
      sPC(y*panel_width+index, gg);
    }
  }
}

void main() {
  resetStat();
  setup();
  while (2>1) {
    loop();
    show();
  }
}