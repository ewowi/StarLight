external void show();
external void pinMode(uint8_t pin, uint8_t mode);
external void digitalWrite(uint8_t pin, uint8_t val);
external void delay(uint32_t ms);
//external void pinsM->allocatePin(unsigned8 pinNr, const char * owner, const char * details); //classes supported?

//how to deal with external defines?
define OUTPUT            0x03 
define LOW               0x0
define HIGH              0x1

uint8_t blinkPin; //tbd: assigment here

void setup()
{
  blinkPin = 5;  //tbd make blinkPin an ui control

  //pinsM->allocatePin(2, "Blink", "On board led"); //class methods and strings supported?
  pinMode(blinkPin, OUTPUT); //tbd: part of allocatePin?
}

void loop() {
    digitalWrite(blinkPin, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(1000);                      // wait for a second
    digitalWrite(blinkPin, LOW);   // turn the LED off by making the voltage LOW
    delay(1000);                      // wait for a second
}

void main()
{
  setup();
  while (0==0)
  {
    loop();
    show();
  }
}