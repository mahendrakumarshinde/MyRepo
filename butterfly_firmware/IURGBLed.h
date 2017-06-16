/*
  Infinite Uptime Firmware

  Update:
    03/03/2017

  Component:
    Name: RGB Led
    Description:
      RGB led at pins 27, 28, 29
*/

#ifndef IURGBLED_H
#define IURGBLED_H

#include <Arduino.h>

/**
* Class to handle the RGB led behavior
* LEDs are active at LOW and inactive at HIGH
*/
class IURGBLed
{
  public:
    enum PIN : uint8_t {RED_PIN = A5,
                        GREEN_PIN = A3,
                        BLUE_PIN = A0};
    enum LEDColors {RED, BLUE, GREEN, ORANGE, PURPLE, CYAN, WHITE, SLEEP};
    // Color code                  R  G  B
    const bool COLORCODE[8][3] = {{1, 0, 0}, // RED
                                  {0, 0, 1}, // BLUE
                                  {0, 1, 0}, // GREEN
                                  {1, 1, 0}, // ORANGE
                                  {1, 0, 1}, // PURPLE
                                  {0, 1, 1}, // CYAN
                                  {1, 1, 1}, // WHITE
                                  {0, 0, 0}};// SLEEP
    // Constructor, destructor, getters and setters
    IURGBLed();
    virtual ~IURGBLed() {}
     // Methods
    void ledOn(IURGBLed::PIN pin_number);
    void ledOff(IURGBLed::PIN pin_number);
    void changeColor(bool R, bool G, bool B);
    void changeColor(LEDColors color);
    void lock() { m_locked = true; }
    void unlock() { m_locked = false; }

  private:
    bool m_locked;
};

#endif // IURGBLED_H
