#ifndef RGBLED_H
#define RGBLED_H

#include "Component.h"
#include "Logger.h"


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
};

extern RGBColor RGB_BLACK;
extern RGBColor RGB_BLUE;
extern RGBColor RGB_GREEN;
extern RGBColor RGB_CYAN;
extern RGBColor RGB_RED;
extern RGBColor RGB_PURPLE;
extern RGBColor RGB_ORANGE;
extern RGBColor RGB_WHITE;


/* =============================================================================
    RGB Led
============================================================================= */

/**
 * RGB Led
 *
 * Component:
 *  Name:
 *    RGB Led
 *  Description:
 *    LEDs are active LOW.
 *    Colors are managed as a queue, each of the color has a fade-in and a on
 *    timer. The complete sequence of color can be set, or individual colors can
 *    be changed in the sequence.
 */
class RGBLed : public Component
{
    public:
        /***** Preset values and default settings *****/
        static const uint8_t maxColorCount = 5;
        /***** Constructors & desctructors *****/
        RGBLed(uint8_t redPin, uint8_t greenPin, uint8_t bluePin);
        virtual ~RGBLed() {}
        /***** Hardware and power management *****/
        virtual void setupHardware();
        virtual void setPowerMode(PowerMode::option pMode);
        void setIntensity(uint8_t intensityPercent);
        /***** Color queue *****/
        void deleteColorQueue() { m_colorCount = 0; }
        void startNewColorQueue(uint8_t colorCount, RGBColor colors[],
                                uint32_t fadeInTimers[], uint32_t onTimers[]);
        void queueColor(RGBColor color, uint32_t fadeInTimer,
                        uint32_t onTimer);
        void replaceColor(uint8_t index, RGBColor color, uint32_t fadeInTimer,
                          uint32_t onTimer);
        void insertColor(uint8_t index, RGBColor color, uint32_t fadeInTimer,
                         uint32_t onTimer);
        uint8_t getColorCount() { return m_colorCount; }
        void lockColors() { m_lockedColors = true; };
        void unlockColors() { m_lockedColors = false; };
        bool lockedColors() { return m_lockedColors; };
        /***** Color transition *****/
        void updateColors();
        void manageColorTransitions();

    private:
        /***** Pin definitions *****/
        uint8_t m_rgbPins[3];
        /***** Color queue *****/
        bool m_lockedColors;
        RGBColor m_colors[maxColorCount];
        uint32_t m_fadeInTimers[maxColorCount];
        uint32_t m_onTimers[maxColorCount];
        uint8_t m_colorCount;
        uint8_t m_intensityPercent;
        /***** Color transition*****/
        uint16_t m_rgbAccumulator[3];
        uint8_t m_rgbValues[3];
        uint8_t m_currentIndex;
        bool m_fadingIn;
        uint32_t m_previousTime;
};

#endif // RGBLED_H
