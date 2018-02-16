#include "IURGBLed.h"


/* =============================================================================
    Constructor & desctructors
============================================================================= */

/**
* LED is activated at construction
*/
IURGBLed::IURGBLed(uint8_t redPin, uint8_t greenPin, uint8_t bluePin) :
    Component(),
    m_redPin(redPin),
    m_greenPin(greenPin),
    m_bluePin(bluePin),
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
    pinMode(m_redPin, OUTPUT);
    pinMode(m_greenPin, OUTPUT);
    pinMode(m_bluePin, OUTPUT);
    setPowerMode(PowerMode::REGULAR);
    changeColor(LEDColors::WHITE);
}

/**
 * Manage component power modes
 */
void IURGBLed::setPowerMode(PowerMode::option pMode)
{
    m_powerMode = pMode;
    switch (m_powerMode)
    {
        case PowerMode::PERFORMANCE:
        case PowerMode::ENHANCED:
        case PowerMode::REGULAR:
            setBlinking(false);
            unlock();
            break;
        case PowerMode::LOW_1:
        case PowerMode::LOW_2:
            setBlinking(true);
            unlock();
            break;
        case PowerMode::SLEEP:
        case PowerMode::DEEP_SLEEP:
        case PowerMode::SUSPEND:
            unlock();
            turnOff();
            lock();
        default:
            if (debugMode)
            {
                debugPrint(F("Unhandled power Mode "), false);
                debugPrint(m_powerMode);
            }
    }
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
    digitalWrite(m_redPin, HIGH);
    digitalWrite(m_greenPin, HIGH);
    digitalWrite(m_bluePin, HIGH);
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
    if (!m_locked)
    {
        digitalWrite(m_redPin, (int) (!R));
        digitalWrite(m_greenPin, (int) (!G));
        digitalWrite(m_bluePin, (int) (!B));
        m_startTime = millis();
    }
}
