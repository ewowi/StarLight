//F_panel80-48.sc

define horizontalPanels 5
define verticalPanels 3

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

void drawPanel(int panelX, int panelY) {
  for (int x=0; x<16;x++) {
    for (int y=15; y>=0; y--) {
      int y2 = y; if (x%2 == 0) {y2=15-y;} //serpentine
      addPixel(panelX * 16 + x, panelY * 16 + y2, 0);
    }
  }
}

void main() {
  drawPanel(2, 1); //first panel is in the middle

  //first row
  drawPanel(4, 0); //2
  drawPanel(3, 0); //3
  drawPanel(1, 0); //4 swapped !!!
  drawPanel(2, 0); //5 swapped !!!
  drawPanel(0, 0); //6

  //second row
  drawPanel(4, 1); //7
  drawPanel(3, 1); //8

  drawPanel(0, 2); //9  !!!

  drawPanel(1, 1); //10
  drawPanel(0, 1); //11

  //third row
  drawPanel(4, 2); //12
  drawPanel(3, 2); //13
  drawPanel(2, 2); //14
  drawPanel(1, 2); //15 no data coming from pin...
}