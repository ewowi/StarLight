//E_hello.sc

int index;

void setup()
{
}

void loop() {
  fadeToBlackBy(2);
  sPC(index, CRGB(custom1Control,custom2Control,custom3Control));
  index += 1; 

  if ( index == width * height) index = 0;
}