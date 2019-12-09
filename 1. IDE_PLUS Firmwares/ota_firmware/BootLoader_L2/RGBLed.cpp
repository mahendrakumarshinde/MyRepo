#include "RGBLed.h"

/* =============================================================================
    RGB Led
============================================================================= */

/***** Color queue *****/

/**
 * Start a new color queue (all blacks by default).
 */
void RGBLed::startNewColorQueue(uint8_t colorCount)
{
    for (uint8_t i = 0; i < maxColorCount; i++)
    {
        m_colors[i] = RGB_BLACK;
        m_fadeInTimers[i] = 0;
        m_onTimers[i] = 0;
    }
    m_onTimers[0] = 1000;
    startNewColorQueue(colorCount, m_colors, m_fadeInTimers, m_onTimers);
}

/**
 * Replace the color sequence displayed by the LED.
 *
 * Colors cannot be changed when locked.
 */
void RGBLed::startNewColorQueue(uint8_t colorCount, RGBColor colors[],
                                uint32_t fadeInTimers[], uint32_t onTimers[])
{
    if (lockedColors()) {
        return;
    }
    if (colorCount > maxColorCount) {
        if (debugMode) {
            debugPrint(F("Max color count exceeded"));
        }
        m_colorCount = maxColorCount;
    } else {
        m_colorCount = colorCount;
    }
    for (uint8_t i = 0; i < m_colorCount; i++) {
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
    if (lockedColors()) {
        return;
    }
    if (m_colorCount >= maxColorCount) {
        if (debugMode) {
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
void RGBLed::replaceColor(uint8_t index, RGBColor color)
{
    if (lockedColors())
    {
        return;
    }
    if (index < m_colorCount) {
        m_colors[index] = color;
    } else if (debugMode) {
        debugPrint(F("Index out of bound"));
    }
}

/**
 * Replace a color in the existing sequence.
 *
 * Colors cannot be changed when locked.
 */
void RGBLed::replaceColor(uint8_t index, RGBColor color, uint32_t fadeInTimer,
                          uint32_t onTimer)
{
    if (lockedColors()) {
        return;
    }
    if (index < m_colorCount) {
        m_colors[index] = color;
        m_fadeInTimers[index] = fadeInTimer;
        m_onTimers[index] = onTimer;
    } else if (debugMode) {
        debugPrint(F("Index out of bound"));
    }
}

/**
 * Insert a color in the existing sequence.
 *
 * Colors cannot be changed when locked.
 */
void RGBLed::insertColor(uint8_t index, RGBColor color, uint32_t fadeInTimer,
                         uint32_t onTimer)
{
    if (lockedColors()) {
        return;
    }
    if (m_colorCount >= maxColorCount) {
        if (debugMode) { debugPrint(F("Max color count exceeded")); }
        return;
    }
    for (uint8_t i = m_colorCount; i > index; i--) {
        m_colors[i] = m_colors[i - 1];
        m_fadeInTimers[i] = m_fadeInTimers[i - 1];
        m_onTimers[i] = m_onTimers[i - 1];
    }
    m_colors[index] = color;
    m_fadeInTimers[index] = fadeInTimer;
    m_onTimers[index] = onTimer;
}


/***** Show colors *****/

/**
 * Manage the color display, with timers and transitions (fade-in / fade-out).
 */
void RGBLed::updateColors()
{
    if (m_colorCount == 0) {
        return;
    }
    uint32_t currentTime = millis();
    while (true) {
        if (m_fadingIn) {
            if(currentTime - m_previousTime > m_fadeInTimers[m_currentIndex]) {
                // Going into on-time
                m_previousTime += m_fadeInTimers[m_currentIndex];
                m_fadingIn = false;
            } else {
                break;
            }
        } else {
            if (currentTime - m_previousTime > m_onTimers[m_currentIndex]) {
                // Starting fade-in of next color
                m_previousTime += m_onTimers[m_currentIndex];
                m_currentIndex = (m_currentIndex + 1) % m_colorCount;
                m_fadingIn = true;
            } else {
                break;
            }
        }
    }
    if (m_fadingIn) {
        int previousIdx = (int) m_currentIndex - 1;
        if (previousIdx < 0) {
            previousIdx += m_colorCount;
        }
        float fadeInRatio = (float) (currentTime - m_previousTime) /
                             (float) m_fadeInTimers[m_currentIndex];
        for (uint8_t i = 0; i < 3; ++i) {
            m_rgbValues[i] = (uint8_t) (
                (float) m_colors[m_currentIndex].rgb[i] * fadeInRatio +
                (float) m_colors[previousIdx].rgb[i] * (1. - fadeInRatio)
                );
        }
    } else {
        for (uint8_t i = 0; i < 3; ++i) {
            m_rgbValues[i] = m_colors[m_currentIndex].rgb[i];
        }
    }
}


/* =============================================================================
    GPIO RGB Led
============================================================================= */

/***** Constructors & desctructors *****/

GPIORGBLed::GPIORGBLed(uint8_t redPin, uint8_t greenPin, uint8_t bluePin) :
    RGBLed()
{
    m_rgbPins[0] = redPin;
    m_rgbPins[1] = greenPin;
    m_rgbPins[2] = bluePin;
}


/***** Hardware & power management *****/

/**
 * Set up the LED pins.
 */
void GPIORGBLed::setup()
{
    pinMode(m_rgbPins[0], OUTPUT);
    pinMode(m_rgbPins[1], OUTPUT);
    pinMode(m_rgbPins[2], OUTPUT);
}


/***** Show colors *****/

/**
 * Increment RGB values and show them on the LED.
 */
void GPIORGBLed::manageHighFreqBlinking()
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


/* =============================================================================
    SPI RGB Led
============================================================================= */

/***** Constructors & desctructors *****/

APA102RGBLedStrip::APA102RGBLedStrip(uint16_t ledCount) :
    RGBLed(),
    m_ledStrip(),
    m_ledCount(ledCount)
{
}



/***** Show colors *****/

/**
 * Send intensity and RGB value to LED
 */
void APA102RGBLedStrip::updateColors()
{
    RGBLed::updateColors();
    uint8_t brightness5Bit = (float(m_intensityPercent) / 100.0 * 31);
    m_ledStrip.startFrame();
    for (uint16_t i = 0; i < m_ledCount; i++) {
        m_ledStrip.sendColor(m_rgbValues[0], m_rgbValues[1], m_rgbValues[2],
                             brightness5Bit);
        delayMicroseconds(250);
    }
    delayMicroseconds(250);
    m_ledStrip.endFrame(m_ledCount);
}


