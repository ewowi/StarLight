//F_panel128-64.sc

void main()
{
  addPixelsPre(128,64,1,8192, 3, 0);

  for (int panely = 0; panely < 4; panely++) {
    for (int panelx = 7; panelx >=0; panelx--) {
      for (int x=0; x<16;x++) {
        for (int y=0; y<16; y++) {
          int y2; y2 = y; if (x%2 == 0) {y2=15-y;} //serpentine
          int panelx2; panelx2 = panelx + 1; if (panelx2==8) {panelx2 = 0;} //ewowi panel correction
          addPixel((panelx2*16+x)*10,(panely*16+y2)*10,0);
        }
      }
    }
  }

  addPixelsPost();
}