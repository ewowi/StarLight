//F_panel080-048.sc

define horizontalPanels 5
define verticalPanels 3

int pins[6]; //for virtual driver, max 6 pins supported atm

//STARLIGHT_LIVE_MAPPING
void mapLed(uint16_t pos) {
  int panelnumber = pos / 256;
  int datainpanel = pos % 256;

  int Xp = horizontalPanels - 1 - panelnumber % horizontalPanels; //wired from right to left

  int yp = panelnumber / horizontalPanels;
  int X = Xp; //panel on the x axis
  int Y = yp; //panel on the y axis

  int y = datainpanel % 16;
  int x = datainpanel / 16;

  X = X * 16 + x;
  if (x % 2 == 0) //serpentine
    Y = Y * 16 + y;
  else
    Y = Y * 16 + 16 - y - 1;

  mapResult = Y * 16 * horizontalPanels + X;
}

void main() {
  //pins = [9,10,12,8,18,17];
  pins[0] = 9; pins[1] = 10; pins[2] = 12; pins[3] = 8; pins[4] = 18; pins[5] = 17;

  for (int panelY = 0; panelY < verticalPanels; panelY++) {

    for (int panelX = horizontalPanels-1; panelX >=0; panelX--) {

      for (int x=0; x<16;x++) {
        for (int y=15; y>=0; y--) {
          int y2 = y; if (x%2 == 0) {y2 = 15 - y;} //serpentine
          addPixel(panelX * 16 + x, panelY * 16 + y2, 0);
        }
      }

    }

    addPin(pins[panelY]);

  }
  //temporary as nr of pins is fixed to 6:
  addPin(pins[3]);
  addPin(pins[4]);
  addPin(pins[5]);
}