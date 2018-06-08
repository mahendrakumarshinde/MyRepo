#ifndef RGBLED_H
#define RGBLED_H

#include <Arduino.h>
#include <SPI.h>

#include <IUDebugger.h>


/* =============================================================================
    RGBColors
============================================================================= */

struct RGBColor
{
    uint8_t rgb[3];
    RGBColor() {
        rgb[0] = 0;
        rgb[1] = 0;
        rgb[2] = 0;
        }
    RGBColor(uint8_t R, uint8_t G, uint8_t B) {
        rgb[0] = R;
        rgb[1] = G;
        rgb[2] = B;
        }
    RGBColor& operator=(const RGBColor& other) {
        rgb[0] = other.rgb[0];
        rgb[1] = other.rgb[1];
        rgb[2] = other.rgb[2];
        return *this;
    }
};

const RGBColor RGB_BLACK(0, 0, 0);
const RGBColor RGB_BLUE(0, 0, 255);
const RGBColor RGB_GREEN(0, 255, 0);
const RGBColor RGB_CYAN(0, 255, 255);
const RGBColor RGB_RED(255, 0, 0);
const RGBColor RGB_PURPLE(255, 0, 255);
const RGBColor RGB_ORANGE(255, 255, 0);
const RGBColor RGB_WHITE(255, 255, 255);


/* =============================================================================
    RGB Led
============================================================================= */

/**
 * RGB led class that can handle a queue of colors to display, with transitions.
 *
 * Colors are managed as a queue, each of the color has a fade-in and a on
 * timer. The complete sequence of color can be set, or individual colors can
 * be changed in the sequence.
 */
class RGBLed
{
    public:
        /***** Preset values and default settings *****/
        static const uint8_t maxColorCount = 5;
        /***** Core *****/
        RGBLed() {}
        virtual ~RGBLed() {}
        virtual void setup() = 0;
        virtual void setIntensity(uint8_t intensityPercent)
            { m_intensityPercent = min(intensityPercent, 100); }
        /***** Color queue *****/
        virtual void deleteColorQueue() { m_colorCount = 0; }
        virtual uint8_t getColorCount() { return m_colorCount; }
        virtual void startNewColorQueue(uint8_t colorCount);
        virtual void startNewColorQueue(uint8_t colorCount, RGBColor colors[],
                                        uint32_t fadeInTimers[],
                                        uint32_t onTimers[]);
        virtual void queueColor(RGBColor color, uint32_t fadeInTimer,
                                uint32_t onTimer);
        virtual void replaceColor(uint8_t index, RGBColor color);
        virtual void replaceColor(uint8_t index, RGBColor color,
                                  uint32_t fadeInTimer,
                                  uint32_t onTimer);
        virtual void insertColor(uint8_t index, RGBColor color,
                                 uint32_t fadeInTimer,
                                 uint32_t onTimer);
        virtual void lockColors() { m_lockedColors = true; }
        virtual void unlockColors() { m_lockedColors = false; }
        virtual bool lockedColors() { return m_lockedColors; };
        /***** Color transition *****/
        virtual void manageColorTransitions();

    protected:
        /***** Hardware and power management *****/
        uint8_t m_intensityPercent = 100;
        /***** Color queue *****/
        bool m_lockedColors = false;
        RGBColor m_colors[maxColorCount];
        uint32_t m_fadeInTimers[maxColorCount];
        uint32_t m_onTimers[maxColorCount];
        uint8_t m_colorCount = 0;
        /***** Color transition *****/
        uint8_t m_rgbValues[3] = {0, 0, 0};
        uint8_t m_currentIndex = 0;
        bool m_fadingIn = true;
        uint32_t m_previousTime = 0;
};


/**
 * GPIO RGB Led
 *
 * Color display must be explicitly managed via regular calls to updateColors.
 */
class GPIORGBLed : public RGBLed
{
    public:
        /***** Constructors & desctructors *****/
        GPIORGBLed(uint8_t redPin, uint8_t greenPin, uint8_t bluePin);
        virtual ~GPIORGBLed() {}
        /***** Hardware and power management *****/
        virtual void setup();
        /***** Show colors *****/
        void updateColors();

    private:
        /***** Pin definitions *****/
        uint8_t m_rgbPins[3];
        /***** Show color *****/
        uint16_t m_rgbAccumulator[3] = {0, 0, 0};
};


/**
 * SPI RGB Led
 */
class SPIRGBLed : public RGBLed
{
    public:
        /***** Constructors & desctructors *****/
        SPIRGBLed(SPIClass *spiPtr, uint8_t csPin, SPISettings settings);
        virtual ~SPIRGBLed() {}
        /***** Hardware and power management *****/
        virtual void setup();
        /***** Show colors *****/
        void updateColors();

    private:
        /***** SPI settings *****/
        SPIClass *m_SPI;
        uint8_t m_csPin;
        SPISettings m_spiSettings;
};

#endif // RGBLED_H
