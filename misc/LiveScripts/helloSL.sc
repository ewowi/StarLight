int i;

void setup()
{

}


void loop() {

  fadeToBlackBy(2);                    // Adjustable fade rate.
//  for (uint8_t y = 0; y < height; y++) {
//  for (uint8_t x = 0; x < width; x++) {
    //sCFP(y*panel_width+x,i, 255);
    CRGB gg = rgb(slider1,0,slider3);
    //CRGB gg = hsv(beatSin8(i, 0, 255),255,255);
    //CRGB gg = hsv(i,255,beatSin8(i, 0, 255));
    //sPC(y*panel_width+x, gg);
    sPC(i, gg);
    i= i + 1; 
    if ( i == width * height) {
      i =0;
}
//  }
//}
}