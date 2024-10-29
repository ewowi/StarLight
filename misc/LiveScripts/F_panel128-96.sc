//F_panel128-64.sc

void main()
{
  addPixelsPre(128,96,1,12288, 1, 3, 0);

  for (int panely = 0; panely < 6; panely++) {
    for (int panelx = 7; panelx >=0; panelx--) {
      for (int x=0; x<16;x++) {
        for (int y=0; y<16; y++) {
          int y2 = y; if (x%2 == 0) {y2=15-y;} //serpentine
          int panelx2 = panelx + 1; if (panelx2==8) {panelx2 = 0;} //ewowi panel correction
          addPixel(panelx2*16+x,panely*16+y2,0);
        }
      }
    }
  }

  addPixelsPost();
}