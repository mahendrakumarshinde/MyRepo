#include "IURGBLed.h"

/**
* LED is activated at construction.
*/
IURGBLed::IURGBLed()
{
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
}

void IURGBLed::ledOn(IURGBLed::PIN pin_number)
{
  digitalWrite(pin_number, LOW);
}

void IURGBLed::ledOff(IURGBLed::PIN pin_number)
{
  digitalWrite(pin_number, HIGH);
}

void IURGBLed::changeColor(bool R, bool G, bool B)
{
  digitalWrite(RED_PIN, (int) (!R));
  digitalWrite(GREEN_PIN, (int) (!G));
  digitalWrite(BLUE_PIN, (int) (!B));
}

/**
* Update the LEDs to the current RGB color
*/
void IURGBLed::changeColor(IURGBLed::LEDColors color)
{
  changeColor(COLORCODE[color][0], COLORCODE[color][1], COLORCODE[color][2]);
}
