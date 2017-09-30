#ifndef IURGBLED_H
#define IURGBLED_H

#include <Arduino.h>
#include "Keywords.h"
#include "IUComponent.h"


namespace IUComponent
{
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
    class IURGBLed : public ABCComponent
    {
        public:
            /***** Preset values and default settings *****/
            enum PIN : uint8_t {RED_PIN = A5,
                                GREEN_PIN = A3,
                                BLUE_PIN = A0};
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
            /***** Constructors & desctructors *****/
            IURGBLed();
            virtual ~IURGBLed() {}
            /***** Hardware and power management *****/
            virtual void sleep();
            virtual void suspend();
            /***** Color management *****/
            void setOnTimer(uint16_t duration) { m_onTimer = duration; }
            void turnOff();
            void autoTurnOff();
            void lock() { m_locked = true; }
            void unlock() { m_locked = false; }
            void changeColor(bool R, bool G, bool B);
            void changeColor(LEDColors color);
            void showOperationState(OperationState::option state);

        private:
            bool m_locked;
            uint16_t m_onTimer;
            uint32_t m_nextOffTime;
    };

    /***** Instanciation *****/
    extern IURGBLed iuRGBLed;
};

#endif // IURGBLED_H
