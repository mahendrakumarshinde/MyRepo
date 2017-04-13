/*
  Infinite Uptime Firmware

  Update:
    03/03/2017

  Component:
    Name:

    Description:
      RGB led at pins 27, 28, 29
*/

#ifndef IURGBLED_H
#define IURGBLED_H

#include <Arduino.h>
#include "IUABCSensor.h"
#include "IUI2C.h"

/**
* Class to handle the RGB led behavior
* LEDs are active at LOW and inactive at HIGH
*/
class IURGBLed : public IUABCSensor
{
  public:
    enum PIN : uint8_t { // Led pin
                        RED_PIN = A5,
                        GREEN_PIN = A3,
                        BLUE_PIN = A0};
    enum LEDColors { // LED color code
      RED_BAD         , // L H H
      BLUE_NOOP       , // H H L
      GREEN_OK        , // H L H
      ORANGE_WARNING  , // L L H
      PURPLE_CHARGE   , // L H L
      CYAN_DATA       , // H L L
      WHITE_NONE      , // L L L
      SLEEP_MODE      };// H H H
    // RGB LOW / HIGH code
    const bool COLORCODE[8][3] = {{0, 1, 1}, //RED_BAD
                                  {1, 1, 0}, //BLUE_NOOP
                                  {1, 0, 1}, //GREEN_OK
                                  {0, 0, 1}, //ORANGE_WARNING
                                  {0, 1, 0}, //PURPLE_CHARGE
                                  {1, 0, 0}, //CYAN_DATA
                                  {0, 0, 0}, //WHITE_NONE
                                  {1, 1, 1} //SLEEP_MODE
                                 };
    static const uint8_t sensorTypeCount = 1;
    static char sensorTypes[sensorTypeCount];
    enum dataSendOption : uint8_t {optionCount = 0}; // Sends nothing

   
    // Constructor, destructor, getters and setters
    IURGBLed(IUI2C *iuI2C);
    virtual ~IURGBLed() {}
    virtual uint8_t getSensorTypeCount() { return sensorTypeCount; }
    virtual char getSensorType(uint8_t index) { return sensorTypes[index]; }
    
     // Methods
    virtual void wakeUp();
    void activate();
    void deactivate() { m_status = false; }
    bool isActive() { return m_status; }
    void ledOn(IURGBLed::PIN pin_number);
    void ledOff(IURGBLed::PIN pin_number);
    void changeColor(LEDColors color);
    bool updateFromI2C();

  protected:
    IUI2C *m_iuI2C;
    bool m_status;
};

#endif // IURGBLED_H
