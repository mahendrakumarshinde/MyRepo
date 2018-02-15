#include "IURGBLed.h"


/* =============================================================================
    Constructor & desctructors
============================================================================= */

/**
* LED is activated at construction
*/
IURGBLed::IURGBLed() :
    Component(),
    m_color(LEDColors::SLEEP),
    m_blinking(false),
    m_startTime(0),
    m_status(LEDStatus::NONE),
    m_showingStatus(false),
    m_nextSwitchTime(0)
{
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set up the LED pins, unlock and wake up the LED.
 */
void IURGBLed::setupHardware()
{
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    unlock();
    wakeUp();
    changeColor(LEDColors::WHITE);
}

/**
 * Switch to ACTIVE power mode
 */
void IURGBLed::wakeUp()
{
    Component::wakeUp();
    setBlinking(false);
}

/**
 * Switch to ECONOMY power mode
 */
void IURGBLed::lowPower()
{
    Component::lowPower();
    setBlinking(true);
}

/**
 * Switch to SUSPEND power mode
 */
void IURGBLed::suspend()
{
    Component::suspend();
    turnOff();
}


/* =============================================================================
    Color management
============================================================================= */

/**
 * Turn off the LED
 */
void IURGBLed::turnOff()
{
    if (m_locked)
    {
        return;  // Do not turn off when locked
    }
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(GREEN_PIN, HIGH);
    digitalWrite(BLUE_PIN, HIGH);
}

/**
 * Automatically turn off the LED after it has been on for more than onTimer.
 *
 * Does nothing if the blinking is deactivated.
 */
void IURGBLed::autoTurnOff(uint32_t currentTime)
{
    uint32_t now;
    if (currentTime > 0)
    {
        now = currentTime;
    }
    else
    {
        now = millis();
    }
    if (m_blinking)
    {
        if (now - m_startTime > onTimer)
        {
            turnOff();
        }
    }
}

/**
 *
 */
void IURGBLed::autoManage()
{
    if (m_locked)
    {
        return;
    }
    uint32_t current = millis();
    switch (m_status)
    {
        case LEDStatus::NONE:
            autoTurnOff(current);
            break;
        case LEDStatus::WIFI_WORKING:
        case LEDStatus::WIFI_CONNECTED:
            if (m_showingStatus)
            {
                if (m_nextSwitchTime < current)
                {
                    changeColor(m_color, true);
                    m_showingStatus = true;
                    m_nextSwitchTime = current +
                            (1000 - statusShowTime[(uint8_t) m_status]);
                }
            }
            else
            {
                if (m_nextSwitchTime < current)
                {
                    changeColor(statusColors[(uint8_t) m_status],
                                true);
                    m_showingStatus = false;
                    m_nextSwitchTime = current +
                            statusShowTime[(uint8_t) m_status];
                }
            }
            break;
        default:
            if (loopDebugMode)
            {
                debugPrint(F("Unmanaged LED status: "), false);
                debugPrint((uint8_t) m_status);
            }
    }
}

/**
 * Turn on / off the LEDs to create the requested RGB color
 */
void IURGBLed::changeColor(IURGBLed::LEDColors color, bool temporary)
{
    if (!m_locked)
    {
        if (!temporary)
        {
            m_color = color;
        }
        manualChangeColor(COLORCODE[color][0], COLORCODE[color][1],
                          COLORCODE[color][2]);
    }
}

/**
 * Turn on / off each LED
 *
 * The color can't be changed if the LEDs are locked or if their current
 * PowerMode is not ACTIVE.
 */
void IURGBLed::manualChangeColor(bool R, bool G, bool B)
{
    if (!m_locked && m_powerMode == PowerMode::ACTIVE)
    {
        digitalWrite(RED_PIN, (int) (!R));
        digitalWrite(GREEN_PIN, (int) (!G));
        digitalWrite(BLUE_PIN, (int) (!B));
        m_startTime = millis();
    }
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
