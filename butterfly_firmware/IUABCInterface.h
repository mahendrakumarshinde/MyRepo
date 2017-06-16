#ifndef IUABCINTERFACE_H
#define IUABCINTERFACE_H

#include <Arduino.h>
#include "IUKeywords.h"

class IUABCInterface
{
  public:
    // Constructors, destructor, getters and setters
    IUABCInterface();
    virtual ~IUABCInterface();
    virtual void setBaudRate() {}   // Implement in child class
    virtual uint32_t getBaudRate() { return m_baudRate; }
    // Hardware & power management methods
    virtual void wakeUp() {}        // May be defined in Child class
    virtual void sleep() {}         // May be defined in Child class
    virtual void suspend() {}       // May be defined in Child class

  protected:
    powerMode::option m_powerMode;
    uint32_t m_baudRate;
};

#endif // IUABCINTERFACE_H
