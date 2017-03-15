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
#include "IUUtilities.h"
#include "IUABCSensor.h"
#include "IUI2C.h"

class IUBattery : public IUABCSensor
{
  public:
    static const uint8_t voltagePin  = A2;  // CHG pin to detect charging status
    static const uint8_t sensorTypeCount = 1;
    static char sensorTypes[sensorTypeCount];
    enum dataSendOption : uint8_t {voltage = 0,
                                   vdda = 1,
                                   optionCount = 2};
    IUBattery(IUI2C *iuI2C);
    virtual ~IUBattery() {}
    virtual void wakeUp();
    float getVoltage();
    float getVDDA();
    void readVoltage();
    virtual void readData();
    virtual void sendToReceivers();

  private:
    IUI2C *m_iuI2C;
    float m_VDDA;
    float m_vBattery;
};

#endif // IUBATTERY_H
