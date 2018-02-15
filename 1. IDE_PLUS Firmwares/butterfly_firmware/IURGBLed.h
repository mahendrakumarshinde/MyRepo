#ifndef IURGBLED_H
#define IURGBLED_H

#include <Arduino.h>
#include "Keywords.h"
#include "Component.h"


/**
 * RGB Led
 *
 * Component:
 *  Name:
 *    RGB Led
 *  Description:
 *    RGB led at pins 27, 28, 29. LEDs are active at LOW and inactive at
 *    HIGH
 */
class IURGBLed : public Component
{
    public:
        /***** Preset values and default settings *****/
        #ifdef DRAGONFLY_V03
            static const uint8_t RED_PIN = 25;
            static const uint8_t GREEN_PIN = 26;
            static const uint8_t BLUE_PIN = 38;
        #else
            static const uint8_t RED_PIN = A5;
            static const uint8_t GREEN_PIN = A3;
            #ifdef BUTTERFLY_V04
                static const uint8_t BLUE_PIN = A4;
            #else
                static const uint8_t BLUE_PIN = A0;
            #endif //BUTTERFLY_V04
        #endif // DRAGONFLY_V03
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
        IURGBLed();
        virtual ~IURGBLed() {}
        void changeStatus(LEDStatus newStatus) { m_status = newStatus; }
        /***** Hardware and power management *****/
        virtual void setupHardware();
        void setBlinking(bool blinking) { m_blinking = blinking; }
        virtual void wakeUp();
        virtual void lowPower();
        virtual void suspend();
        /***** Color management *****/
        void turnOff();
        void autoTurnOff(uint32_t currentTime=0);
        void autoManage();
        void lock() { m_locked = true; }
        void unlock() { m_locked = false; }
        void changeColor(LEDColors color, bool temporary=false);
        void manualChangeColor(bool R, bool G, bool B);
        void showOperationState(OperationState::option state);

    private:
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
