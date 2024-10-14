//helloSL.sc

int index;

void setup()
{
}

void loop() {
  fadeToBlackBy(2);
  sPC(index, rgb(slider1,slider2,slider3));
  index = index + 1; 
  if ( index == width * height) {
    index =0;
  }
}