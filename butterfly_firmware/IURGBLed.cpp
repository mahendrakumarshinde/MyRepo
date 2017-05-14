#include "IURGBLed.h"

char IURGBLed::sensorTypes[IURGBLed::sensorTypeCount] = {IUABCSensor::sensorType_rgbled};

/**
* LED is activated at construction.
*/
IURGBLed::IURGBLed(IUI2C *iuI2C) : IUABCSensor  (), m_iuI2C(iuI2C)
{
  activate();
}

/**
 * Set up the RGB LED and activate it.
 */
void IURGBLed::wakeUp()
{
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  activate();
}

/**
 * Set the status to active and set LED pins to output mode
 */
void IURGBLed::activate()
{
  m_status = true;
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
  if (!m_status) return;
  digitalWrite(RED_PIN, (int) (!R));
  digitalWrite(GREEN_PIN, (int) (!G));
  digitalWrite(BLUE_PIN, (int) (!B));
}

/**
* Update the LEDs to the current RGB color
* Note: status must be active (see activate method)
*/
void IURGBLed::changeColor(IURGBLed::LEDColors color)
{
  changeColor(COLORCODE[color][0], COLORCODE[color][1], COLORCODE[color][2]);
}


/* ==================== Update and Control Functions =============================== */
/*
 * The following functions check and read the I2C wire buffer to see if configuration updates
 * have been received. If so, the updates are done.
 * Functions return true if an update happened.
 */


/**
 * Check the I2C wire buffer for accel range update
 */
bool IURGBLed::updateFromI2C()
{
  String wireBuffer = m_iuI2C->getBuffer();
  if (wireBuffer.indexOf("rgb") > -1)
  {
    int rgblocation = wireBuffer.indexOf("rgb");
    int red = wireBuffer.charAt(rgblocation + 7) - 48;
    int green = wireBuffer.charAt(rgblocation + 8) - 48;
    int blue = wireBuffer.charAt(rgblocation + 9) - 48;
    if (red == 1) {
      ledOn(RED_PIN);
    }
    else {
      ledOff(RED_PIN);
    }
    if (green == 1) {
      ledOn(GREEN_PIN);
    }
    else {
      ledOff(GREEN_PIN);
    }
    if (blue == 1) {
      ledOn(BLUE_PIN);
    }
    else {
      ledOff(BLUE_PIN);
    }
    return true;
  }
  return false;
}
