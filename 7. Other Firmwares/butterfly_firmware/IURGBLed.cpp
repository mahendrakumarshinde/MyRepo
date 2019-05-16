#include "IURGBLed.h"

/**
* LED is activated at construction.
*/
IURGBLed::IURGBLed()
{
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  unlock();
}

void IURGBLed::ledOn(IURGBLed::PIN pin_number)
{
  if (!m_locked)
  {
    digitalWrite(pin_number, LOW);
  }
}

void IURGBLed::ledOff(IURGBLed::PIN pin_number)
{
  if (!m_locked)
  {
    digitalWrite(pin_number, HIGH);
  }
}

void IURGBLed::changeColor(bool R, bool G, bool B)
{
  if (!m_locked)
  {
    digitalWrite(RED_PIN, (int) (!R));
    digitalWrite(GREEN_PIN, (int) (!G));
    digitalWrite(BLUE_PIN, (int) (!B));
  }
}

/**
* Update the LEDs to the current RGB color
*/
void IURGBLed::changeColor(IURGBLed::LEDColors color)
{
  if (!m_locked)
  {
    changeColor(COLORCODE[color][0], COLORCODE[color][1], COLORCODE[color][2]);
  }
}
