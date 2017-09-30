#include "IURGBLed.h"

using namespace IUComponent;

/* =============================================================================
    Constructor & desctructors
============================================================================= */

/**
* LED is activated at construction
*/
IURGBLed::IURGBLed() :
    ABCComponent(),
    m_onTimer(50),
    m_nextOffTime(0)
{
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  unlock();
  wakeUp();
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Switch to SLEEP power mode
 */
void IURGBLed::sleep()
{
    ABCComponent::sleep();
    turnOff();
}

/**
 * Switch to SUSPEND power mode
 */
void IURGBLed::suspend()
{
    ABCComponent::sleep();
    turnOff();
}


/* =============================================================================
    Color management
============================================================================= */

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

/**
 * Turn on / off each LED
 *
 * The color can't be changed if the LEDs are locked or if their current
 * PowerMode is not ACTIVE.
 */
void IURGBLed::changeColor(bool R, bool G, bool B)
{
    if (!m_locked && m_powerMode == PowerMode::ACTIVE)
    {
        digitalWrite(RED_PIN, (int) (!R));
        digitalWrite(GREEN_PIN, (int) (!G));
        digitalWrite(BLUE_PIN, (int) (!B));
        m_nextOffTime = millis() + m_onTimer;
    }
}

/**
 * Turn on / off the LEDs to create the requested RGB color
 */
void IURGBLed::changeColor(IURGBLed::LEDColors color)
{
    changeColor(COLORCODE[color][0], COLORCODE[color][1], COLORCODE[color][2]);
}

/**
 * Lit the LEDs with a color specific to the OperationState
 *
 * Colors are BLUE (idle), GREEN (normal), ORANGE (warning) and RED (danger).
 */
void IURGBLed::showOperationState(OperationState::option state)
{
    switch (state)
    {
    case OperationState::IDLE:
        changeColor(LEDColors::BLUE);
        break;
    case OperationState::NORMAL:
        changeColor(LEDColors::GREEN);
        break;
    case OperationState::WARNING:
        changeColor(LEDColors::ORANGE);
        break;
    case OperationState::DANGER:
        changeColor(LEDColors::RED);
        break;
    default:
        turnOff();
    }
}


/* =============================================================================
    Instanciation
============================================================================= */

IURGBLed iuRGBLed();
