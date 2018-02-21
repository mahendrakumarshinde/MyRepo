#include "RGBLed.h"


/* =============================================================================
    RGBColors
============================================================================= */

RGBColor RGB_BLACK(0,   0,   0);
RGBColor RGB_BLUE(0,   0,   255);
RGBColor RGB_GREEN(0,   255, 0);
RGBColor RGB_CYAN(0,   255, 255);
RGBColor RGB_RED(255, 0,   0);
RGBColor RGB_PURPLE(255, 0,   255);
RGBColor RGB_ORANGE(255, 255, 0);
RGBColor RGB_WHITE(255, 255, 255);


/* =============================================================================
    Constructor & desctructors
============================================================================= */

/**
* LED is activated at construction
*/
RGBLed::RGBLed(uint8_t redPin, uint8_t greenPin, uint8_t bluePin) :
    Component(),
    m_lockedColors(false),
    m_colorCount(0),
    m_intensityPercent(100),
    m_currentIndex(0),
    m_fadingIn(true),
    m_previousTime(0)
{
    m_rgbPins[0] = redPin;
    m_rgbPins[1] = greenPin;
    m_rgbPins[2] = bluePin;
    for (uint8_t i = 0; i < 3; ++i)
    {
        m_rgbAccumulator[i] = 0;
        m_rgbValues[i] = 0;
    }
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set up the LED pins, unlock and wake up the LED.
 */
void RGBLed::setupHardware()
{
    pinMode(m_rgbPins[0], OUTPUT);
    pinMode(m_rgbPins[1], OUTPUT);
    pinMode(m_rgbPins[2], OUTPUT);
    setPowerMode(PowerMode::REGULAR);
}

/**
 * Manage component power modes
 */
void RGBLed::setPowerMode(PowerMode::option pMode)
{
    m_powerMode = pMode;
    switch (m_powerMode)
    {
        case PowerMode::PERFORMANCE:
            m_intensityPercent = 100;
            break;
        case PowerMode::ENHANCED:
            m_intensityPercent = 90;
            break;
        case PowerMode::REGULAR:
            m_intensityPercent = 80;
            break;
        case PowerMode::LOW_1:
            m_intensityPercent = 40;
            break;
        case PowerMode::LOW_2:
            m_intensityPercent = 30;
            break;
        case PowerMode::SLEEP:
            m_intensityPercent = 20;
            break;
        case PowerMode::DEEP_SLEEP:
            m_intensityPercent = 0;
            break;
        case PowerMode::SUSPEND:
            m_intensityPercent = 0;
            break;
        default:
            if (debugMode)
            {
                debugPrint(F("Unhandled power Mode "), false);
                debugPrint(m_powerMode);
            }
    }
}

void RGBLed::setIntensity(uint8_t intensityPercent)
{
    if (intensityPercent > 100)
    {
        m_intensityPercent = 100;
    }
    m_intensityPercent = intensityPercent;
}


/* =============================================================================
    Color queue
============================================================================= */

/**
 * Replace the color sequence displayed by the LED.
 *
 * Colors cannot be changed when locked.
 */
void RGBLed::startNewColorQueue(uint8_t colorCount, RGBColor colors[],
                                uint32_t fadeInTimers[], uint32_t onTimers[])
{
    if (m_lockedColors)
    {
        return;
    }
    if (colorCount > maxColorCount)
    {
        if (debugMode)
        {
            debugPrint(F("Max color count exceeded"));
        }
        m_colorCount = maxColorCount;
    }
    else
    {
        m_colorCount = colorCount;
    }
    for (uint8_t i = 0; i < m_colorCount; i++)
    {
        m_colors[i] = colors[i];
        m_fadeInTimers[i] = fadeInTimers[i];
        m_onTimers[i] = onTimers[i];
    }
}

/**
 * Add a color (with fade-in and durations) at the end of existing sequence.
 *
 * Colors cannot be changed when locked.
 */
void RGBLed::queueColor(RGBColor color, uint32_t fadeInTimer,
                        uint32_t onTimer)
{
    if (m_lockedColors)
    {
        return;
    }
    if (m_colorCount >= maxColorCount)
    {
        if (debugMode)
        {
            debugPrint(F("Max color count exceeded"));
        }
        return;
    }
    m_colors[m_colorCount] = color;
    m_fadeInTimers[m_colorCount] = fadeInTimer;
    m_onTimers[m_colorCount] = onTimer;
    m_colorCount++;
}

/**
 * Replace a color in the existing sequence.
 *
 * Colors cannot be changed when locked.
 */
void RGBLed::replaceColor(uint8_t index, RGBColor color, uint32_t fadeInTimer,
                          uint32_t onTimer)
{
    if (m_lockedColors)
    {
        return;
    }
    if (index >= m_colorCount)
    {
        if (debugMode)
        {
            debugPrint(F("Index out of bound"));
        }
        return;
    }
    m_colors[index] = color;
    m_fadeInTimers[index] = fadeInTimer;
    m_onTimers[index] = onTimer;
}

/**
 * Insert a color in the existing sequence.
 *
 * Colors cannot be changed when locked.
 */
void RGBLed::insertColor(uint8_t index, RGBColor color, uint32_t fadeInTimer,
                         uint32_t onTimer)
{
    if (m_lockedColors)
    {
        return;
    }
    if (m_colorCount >= maxColorCount)
    {
        if (debugMode)
        {
            debugPrint(F("Max color count exceeded"));
        }
        return;
    }
    for (uint8_t i = m_colorCount; i > index; i--)
    {
        m_colors[i] = m_colors[i - 1];
        m_fadeInTimers[i] = m_fadeInTimers[i - 1];
        m_onTimers[i] = m_onTimers[i - 1];
    }
    m_colors[index] = color;
    m_fadeInTimers[index] = fadeInTimer;
    m_onTimers[index] = onTimer;
}


/* =============================================================================
    Color transition
============================================================================= */

/**
 * Increment RGB values and show them on the LED.
 */
void RGBLed::updateColors()
{
    for (uint8_t i = 0; i < 3; ++i)
    {
        m_rgbAccumulator[i] += (uint16_t) m_rgbValues[i] * m_intensityPercent;
        if (m_rgbAccumulator[i] >= 25500)
        {
            m_rgbAccumulator[i] -= 25500;
            digitalWrite(m_rgbPins[i], LOW);
        }
        else
        {
            digitalWrite(m_rgbPins[i], HIGH);
        }
    }
}

/**
 * Manage the color display, with timers and transitions (fade-in / fade-out).
 */
void RGBLed::manageColorTransitions()
{
    if (m_colorCount == 0)
    {
        return;
    }
    uint32_t currentTime = millis();
    while (true)
    {
        if (m_fadingIn)
        {
            if(currentTime - m_previousTime > m_fadeInTimers[m_currentIndex])
            {
                // Going into on-time
                m_previousTime += m_fadeInTimers[m_currentIndex];
                m_fadingIn = false;
            }
            else
            {
                break;
            }
        }
        else
        {
            if (currentTime - m_previousTime > m_onTimers[m_currentIndex])
            {
                // Starting fade-in of next color
                m_previousTime += m_onTimers[m_currentIndex];
                m_currentIndex = (m_currentIndex + 1) % m_colorCount;
                m_fadingIn = true;
            }
            else
            {
                break;
            }
        }
    }
    if (m_fadingIn)
    {
        int previousIdx = (int) m_currentIndex - 1;
        if (previousIdx < 0)
        {
            previousIdx += m_colorCount;
        }
        float fadeInRatio = (float) (currentTime - m_previousTime) /
                             (float) m_fadeInTimers[m_currentIndex];
        for (uint8_t i = 0; i < 3; ++i)
        {
            m_rgbValues[i] = (uint8_t) (
                (float) m_colors[m_currentIndex].rgb[i] * fadeInRatio +
                (float) m_colors[previousIdx].rgb[i] * (1. - fadeInRatio)
                );
        }
    }
    else
    {
        for (uint8_t i = 0; i < 3; ++i)
        {
            m_rgbValues[i] = m_colors[m_currentIndex].rgb[i];
        }
    }
}
