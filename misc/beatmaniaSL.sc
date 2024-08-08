//from https://github.com/MoonModules/WLED-Effects/blob/master/ARTIFX/wled/beatmania.wled
//Created by Andrew Tuline Converted by Ewoud Wijma

uint8_t locn1;
uint8_t locn2;
uint8_t locn3;
uint8_t locn12;

uint8_t colr1;
uint8_t colr2;
uint8_t colr3;
uint8_t colr12;

uint8_t bri1;
uint8_t bri2;
uint8_t bri3;
uint8_t bri12;

void setup()
{
}


void loop() {

    //fadeToBlackBy(slider2/8);                    // Adjustable fade rate.

    locn1 = beatSin8(slider1/3+1,0,width) ;       // Adjustable speed.
    locn2 = beatSin8(slider1/4+1,0,width);
    locn3 = beatSin8(slider1/5+1,0,width/2+width/3);

    colr1 = beatSin8(slider2/6+1,0,255);
    colr2 = beatSin8(slider2/7+1,0,255);
    colr3 = beatSin8(slider2/8+1,0,255);

    bri1 = beatSin8(slider2/6+1,32,255);
    bri2 = beatSin8(slider2/7+1,32,255);
    bri3 = beatSin8(slider2/8+1,32,255)	;		// One too many beats.

    locn12 = locn1+locn2;
    colr12 = colr1+colr2;
    bri12 = bri1+bri2;
    sCFP(locn12,colr12,bri12);
    sCFP(locn1,colr2,bri1);
    sCFP(locn2%(width-1),colr1,bri2);
}

void main() {
  resetStat();
  setup();
  while (2>1) {
    loop();
    show();
  }
}