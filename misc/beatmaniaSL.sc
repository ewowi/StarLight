//from https://github.com/MoonModules/WLED-Effects/blob/master/ARTIFX/wled/beatmania.wled
//Created by Andrew Tuline Converted by Ewoud Wijma


void setup()
{
}


void loop() {

  fadeToBlackBy(slider2/8);                    // Adjustable fade rate.

  uint8_t length = width-1;

  uint8_t locn1 = beatSin8(slider1/3+1,0,length) ;       // Adjustable speed.
  uint8_t locn2 = beatSin8(slider1/4+1,0,length);
  uint8_t locn3 = beatSin8(slider1/5+1,0,length/2+length/3);

  uint8_t colr1 = beatSin8(slider2/6+1,0,255);
  uint8_t colr2 = beatSin8(slider2/7+1,0,255);
  uint8_t colr3 = beatSin8(slider2/8+1,0,255);

  uint8_t bri1 = beatSin8(slider2/6+1,32,255);
  uint8_t bri2 = beatSin8(slider2/7+1,32,255);
  uint8_t bri3 = beatSin8(slider2/8+1,32,255)	;		// One too many beats.

  uint8_t locn12 = locn1+locn2;
  uint8_t colr12 = colr1+colr2;
  uint8_t bri12 = bri1+bri2;
  for (uint8_t y = 0; y < height; y++) {
    if (locn12 < length) {sCFP(y*panel_width+locn12,colr12,bri12);}
    sCFP(y*panel_width+locn1,colr2,bri1);
    sCFP(y*panel_width+locn2%(length-1),colr1,bri2);
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