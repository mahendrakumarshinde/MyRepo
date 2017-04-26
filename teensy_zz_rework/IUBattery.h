#ifndef IUBATTERY_H
#define IUBATTERY_H

#include <Arduino.h>

class IUBattery
{
  public:
    const int chargerCHG  = 2;  // CHG pin to detect charging status
    IUBattery();
    virtual ~IUBattery();
    int getInputVoltage();
    void activate();
    int getStatus() { return m_status; }
    void updateStatus();

  private:
    int m_status; // ex bat // Battery status
};

#endif // IUBATTERY_H
