/*
  Infinite Uptime Firmware

  Update:
    03/03/2017
  
  Component:
    Name:
      
    Description:
      Battery + LiPo charger
*/

#ifndef IUBATTERY_H
#define IUBATTERY_H

#include <Arduino.h>

class IUBattery
{
  public:
    static const uint8_t voltagePin  = A2;  // CHG pin to detect charging status
    IUBattery();
    void wakeUp();
    float getVoltage() { return m_vBattery; }
    float getVDDA() { return m_VDDA; }
    void readVoltage();

  private:
    float m_VDDA;
    float m_vBattery;
};

#endif // IUBATTERY_H
