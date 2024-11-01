//F_panel80-48.sc

define horizontalPanels 5
define verticalPanels 3

//to be modified for 80x48
void map(int pos) {
  int panelnumber = pos / 256;
  int datainpanel = pos % 256;
  int Xp = 7 - panelnumber % 8;

  //fix for ewowi panels
  Xp=Xp+1;
  if (Xp==8) {Xp=0;}

  int yp = panelnumber / 8;
  int X = Xp; //panel on the x axis
  int Y = yp; //panel on the y axis

  int y = datainpanel % 16;
  int x = datainpanel / 16;

  if (x % 2 == 0) //serpentine
  {
    Y = Y * 16 + y;
    X = X * 16 + x;
  }
  else
  {
    Y = Y * 16 + 16 -y-1;
    X = X * 16 + x;
  }

  mapResult = (95-Y) * 16 * 8 + (127-X);
}

void main() {
  for (int panelY = 0; panelY < verticalPanels; panelY++) {
    for (int panelX = horizontalPanels-1; panelX >=0; panelX--) {

      for (int x=0; x<16;x++) {
        for (int y=0; y<16; y++) {
          int y2 = y; if (x%2 == 0) {y2=15-y;} //serpentine
          //int panelX2 = panelX + 1; if (panelX2==8) {panelX2 = 0;} //ewowi panel correction
          addPixel(panelX*16+x,panelY*16+y2,0);
        }
      }

    }
  }
}