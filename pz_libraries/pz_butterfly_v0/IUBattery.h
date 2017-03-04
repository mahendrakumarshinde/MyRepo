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
    const int chargerCHG  = 2;  // CHG pin to detect charging status
    IUBattery() { m_status = 100; }
    int getInputVoltage();
    void activate();
    int getStatus() { return m_status; }
    void updateStatus();

  private:
    int m_status; // ex bat // Battery status
};

#endif // IUBATTERY_H
