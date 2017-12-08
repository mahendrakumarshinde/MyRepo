#include "IURGBLed.h"

/**
* LED is activated at construction.
*/
IURGBLed::IURGBLed() :
  m_onTimer(50),
  m_nextOffTime(0)
{
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  unlock();
}

/**
 * Turn off the LED (it doesn't matter if the LEDs are locked or not)
 */
void IURGBLed::turnOff()
{
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, HIGH);
}

/**
 * Automatically turn off the LEDs after they have been on for more than onTimer
 */
void IURGBLed::autoTurnOff()
{
  uint32_t now = millis();
  if (now > m_nextOffTime)
  {
    turnOff();
  }
}

void IURGBLed::changeColor(bool R, bool G, bool B)
{
  if (!m_locked)
  {
    digitalWrite(RED_PIN, (int) (!R));
    digitalWrite(GREEN_PIN, (int) (!G));
    digitalWrite(BLUE_PIN, (int) (!B));
    m_nextOffTime = millis() + m_onTimer;
  }
}

/**
 * Update the LEDs to the current RGB color
 */
void IURGBLed::changeColor(IURGBLed::LEDColors color)
{ 
  changeColor(COLORCODE[color][0], COLORCODE[color][1], COLORCODE[color][2]);
}

/**
 * Lit the LEDs with a color specific to the operationState
 * 
 * Colors are BLUE (idle), GREEN (normal), ORANGE (warning) and RED (danger).
 */
void IURGBLed::showOperationState(operationState::option state)
{
  switch (state)
  {
    case operationState::IDLE:
      changeColor(LEDColors::BLUE);
      break;
    case operationState::NORMAL:
      changeColor(LEDColors::GREEN);
      break;
    case operationState::WARNING:
      changeColor(LEDColors::ORANGE);
      break;
    case operationState::DANGER:
      changeColor(LEDColors::RED);
      break;
    default:
      turnOff();
  }
}

