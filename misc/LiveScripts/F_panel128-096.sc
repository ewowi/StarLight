//F_panel128-96.sc

define horizontalPanels 8
define verticalPanels 6

void mapLed(uint16_t pos) {
  int panelnumber = pos / 256;
  int datainpanel = pos % 256;

  int Xp = horizontalPanels - 1 - panelnumber % horizontalPanels; //wired from right to left
  Xp = (Xp + 1) % horizontalPanels; //fix for ewowi panels

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

  mapResult = (verticalPanels * 16 - 1 - Y) * 16 * horizontalPanels + (horizontalPanels * 16 - 1 - X);
}

void main() {
  for (int panelY = 0; panelY < verticalPanels; panelY++) {
    for (int panelX = horizontalPanels-1; panelX >=0; panelX--) {

      for (int x=0; x<16;x++) {
        for (int y=15; y>=0; y--) {
          int y2 = y; if (x%2 == 0) {y2=15-y;} //serpentine
          int panelX2 = (panelX + 1) % horizontalPanels; //ewowi panel correction
          addPixel(panelX*16+x,panelY*16+y2,0);
        }
      }

    }
  }
}