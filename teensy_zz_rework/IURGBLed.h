#ifndef IURGBLED_H
#define IURGBLED_H

#include <Arduino.h>
#include "IUBLE.h"
#include "IUI2CTeensy.h"

/**
* Class to handle the RGB led behavior
* LEDs are active at LOW and inactive at HIGH
*/
class IURGBLed
{
  public:
    IURGBLed(IUI2CTeensy iuI2C, IUBLE iuBLE);
    virtual ~IURGBLed();
    enum PIN : uint8_t { // Led pin
                        RED_PIN = 29,
                        GREEN_PIN = 27,
                        BLUE_PIN = 28};
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

    void activate();
    void deactivate() { m_status = false; }
    bool isActive() { return m_status; }
    void ledOn(IURGBLed::PIN pin_number);
    void ledOff(IURGBLed::PIN pin_number);
    void changeColor(LEDColors color);
    bool updateFromI2C();

  private:
    IUI2CTeensy m_iuI2C;
    IUBLE m_iuBLE;
    bool m_status;
};

#endif // IURGBLED_H
