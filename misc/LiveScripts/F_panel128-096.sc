//F_panel128-96.sc

define horizontalPanels 8
define verticalPanels 6

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