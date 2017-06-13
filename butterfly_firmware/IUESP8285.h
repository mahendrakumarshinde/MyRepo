#ifndef IUESP8285_H
#define IUESP8285_H

#include <Arduino.h>
#include "IUI2C.h"

class IUESP8285 : public IUABCInterface
{
  public:
    static constexpr HardwareSerial *port = &Serial3;
    //Constructor, destructor, setters and getters
    IUESP8285(IUI2C *iuI2C);
    virtual ~IUESP8285() {}
    // Hardware and power management
    virtual void wakeUp();
    virtual void sleep();
    virtual void suspend();
    // Communication methods


  protected:
    IUI2C *m_iuI2C;
};

#endif // IUESP8285_H
