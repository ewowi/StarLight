//F_panel080-048.sc

define horizontalPanels 5
define verticalPanels 3
define panelWidth 16
define panelHeight 16

//for virtual driver, max 6 pins supported atm
//int pins[6] = {14,12,13,25,33,32}; //for esp32 (wrover)
int pins[6] = {9,10,12,8,18,17}; //for esp32-S3

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

  int y2 = (x%2)? y: panelHeight - 1 - y; //serpentine

  Y = Y * panelHeight + y2;

  mapResult = Y * panelHeight * horizontalPanels + X;
}

void main() {

  //virtual driver settings
  clockPin = 3; //3 for S3, 26 for ESP32 (wrover)
  latchPin = 46; //46 for S3, 27 for ESP32 (wrover)
  clockFreq = 10; //clockFreq==10?clock_1000KHZ:clockFreq==11?clock_1111KHZ:clockFreq==12?clock_1123KHZ:clock_800KHZ
  dmaBuffer = 30; //not implemented yet  

  for (int panelY = 0; panelY < verticalPanels; panelY++) {

    for (int panelX = horizontalPanels-1; panelX >=0; panelX--)
      for (int x=0; x<panelWidth;x++)
        for (int y=panelHeight - 1; y>=0; y--)
          addPixel(panelX * panelWidth + x, panelY * panelHeight + (x%2)? y: panelHeight - 1 - y, 0); //serpentine

    addPin(pins[panelY]);
  }

  //temporary as nr of pins is fixed to 6:
  addPin(pins[3]);
  addPin(pins[4]);
  addPin(pins[5]);
}