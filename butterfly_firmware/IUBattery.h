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
#include "IUABCSensor.h"
#include "IUI2C.h"

class IUBattery : public IUABCSensor
{
  public:
    static const uint8_t voltagePin  = A2;  // CHG pin to detect charging status
    static const uint8_t sensorTypeCount = 1;
    static const uint16_t maxVoltage = 4100; // mV full battery voltage
    static char sensorType = IUABCSensor::BATTERY;
    enum dataSendOption : uint8_t {voltage = 0,
                                   vdda = 1,
                                   optionCount = 2};
    // Constructor, Destructor, getters and setters
    IUBattery(IUI2C *iuI2C);
    virtual ~IUBattery() {}
    virtual char getSensorType() { return (char) sensorType; }
    // Hardware and power management methods
    virtual void wakeUp();
    virtual void sleep();
    virtual void suspend();
    // Data acquisition methods
    int getVoltage();
    float getVDDA();
    uint8_t getBatteryStatus();
    void readVoltage();
    virtual void readData();
    // Communication methods
    virtual void sendToReceivers();

  protected:
    IUI2C *m_iuI2C;
    float m_VDDA;
    float m_vBattery;
};

#endif // IUBATTERY_H
