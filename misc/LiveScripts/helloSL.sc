//helloSL.sc

int index;

void setup()
{
}

void loop() {
  fadeToBlackBy(2);
  CRGB gg = rgb(slider1,slider2,slider3);
  sPC(index, gg);
  index = index + 1; 
  if ( index == width * height) {
    index =0;
  }
}