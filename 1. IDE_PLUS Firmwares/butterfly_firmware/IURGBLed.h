#ifndef IURGBLED_H
#define IURGBLED_H

#include "Component.h"
#include "Logger.h"


/**
 * RGB Led
 *
 * Component:
 *  Name:
 *    RGB Led
 *  Description:
 *    LEDs are active LOW.
 */
class IURGBLed : public Component
{
    public:
        /***** Preset values and default settings *****/
        enum LEDColors : uint8_t {RED,
                                  BLUE,
                                  GREEN,
                                  ORANGE,
                                  PURPLE,
                                  CYAN,
                                  WHITE,
                                  SLEEP};
        // Color code                  R  G  B
        const bool COLORCODE[8][3] = {{1, 0, 0}, // RED
                                      {0, 0, 1}, // BLUE
                                      {0, 1, 0}, // GREEN
                                      {1, 1, 0}, // ORANGE
                                      {1, 0, 1}, // PURPLE
                                      {0, 1, 1}, // CYAN
                                      {1, 1, 1}, // WHITE
                                      {0, 0, 0}};// SLEEP
        /***** Status *****/
        enum LEDStatus : uint8_t {NONE,
                                  WIFI_WORKING,
                                  WIFI_CONNECTED};
        const uint32_t statusShowTime[3] = {0, 100, 100};
        const LEDColors statusColors[3] = {LEDColors::SLEEP,
                                           LEDColors::PURPLE,
                                           LEDColors::CYAN};
        // The default duration the LED stays on after being lit.
        static const uint8_t onTimer = 50;  //ms
        /***** Constructors & desctructors *****/
        IURGBLed(uint8_t redPin, uint8_t greenPin, uint8_t bluePin);
        virtual ~IURGBLed() {}
        void changeStatus(LEDStatus newStatus) { m_status = newStatus; }
        /***** Hardware and power management *****/
        virtual void setupHardware();
        virtual void setPowerMode(PowerMode::option pMode);
        void setBlinking(bool blinking) { m_blinking = blinking; }
        /***** Color management *****/
        void turnOff();
        void autoTurnOff(uint32_t currentTime=0);
        void autoManage();
        void lock() { m_locked = true; }
        void unlock() { m_locked = false; }
        void changeColor(LEDColors color, bool temporary=false);
        void manualChangeColor(bool R, bool G, bool B);

    private:
        /***** Pin definitions *****/
        uint8_t m_redPin;
        uint8_t m_greenPin;
        uint8_t m_bluePin;
        /***** Color handling *****/
        LEDColors m_color;
        bool m_blinking;
        bool m_locked;
        uint32_t m_startTime;
        /***** Status management *****/
        LEDStatus m_status;
        bool m_showingStatus;
        uint32_t m_nextSwitchTime;
};

#endif // IURGBLED_H
