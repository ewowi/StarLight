//F_Cube202020.sc

define width 16
define height 16
define depth 48

define panelWidth 16
define panelHeight 16

int pins[6]; //for virtual driver, max 6 pins supported atm

void main()
{
  pins[0] = 9; pins[1] = 10; pins[2] = 12; pins[3] = 8; pins[4] = 18; pins[5] = 17;
  setLedSize(2); //smaller leds (default 5)

  for (int z=0; z<depth;z++) {

    for (int x=0; x<panelWidth;x++) {
      for (int y=panelHeight - 1; y>=0; y--) {
        int y2 = y; if (x%2 == 0) {y2 = panelHeight - 1 - y;} //serpentine
        addPixel(x, y2, z);
      }
    }

    addPin(pins[z/8]); //every 8 panels one pin on the esp32
  }
}