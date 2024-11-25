//F_panel080-048.sc

define horizontalPanels 5
define verticalPanels 3
define panelWidth 16
define panelHeight 16

int pins[6]; //for virtual driver, max 6 pins supported atm

//STARLIGHT_LIVE_MAPPING
void mapLed(uint16_t pos) {
  int panelnumber = pos / 256;
  int datainpanel = pos % 256;

  int Xp = horizontalPanels - 1 - panelnumber % horizontalPanels; //wired from right to left

  int yp = panelnumber / horizontalPanels;
  int X = Xp; //panel on the x axis
  int Y = yp; //panel on the y axis

  int y = datainpanel % panelHeight;
  int x = datainpanel / panelWidth;

  X = X * panelWidth + x;
  if (x % 2 == 0) //serpentine
    Y = Y * panelHeight + y;
  else
    Y = Y * panelHeight + panelHeight - y - 1;

  mapResult = Y * panelHeight * horizontalPanels + X;
}

void main() {
  //pins = [14,12,13,25,33,32]; //for esp32
  //pins = [9,10,12,8,18,17]; //for esp32-S3
  pins[0] = 9; pins[1] = 10; pins[2] = 12; pins[3] = 8; pins[4] = 18; pins[5] = 17;

  for (int panelY = 0; panelY < verticalPanels; panelY++) {

    for (int panelX = horizontalPanels-1; panelX >=0; panelX--) {

      for (int x=0; x<panelWidth;x++) {
        for (int y=panelHeight - 1; y>=0; y--) {
          int y2 = y; if (x%2 == 0) {y2 = panelHeight - 1 - y;} //serpentine
          addPixel(panelX * panelWidth + x, panelY * panelHeight + y2, 0);
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