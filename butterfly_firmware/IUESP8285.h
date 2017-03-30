#ifndef IUESP8285_H
#define IUESP8285_H

#include <Arduino.h>
#include "IUUtilities.h"
#include "IUI2C.h"

class IUESP8285 : public IUABCInterface
{
  public:
    static constexpr HardwareSerial *port = &Serial3;
    //Constructor, destructor, setters and getters
    IUESP8285(IUI2C *iuI2C);
    virtual ~IUESP8285() {}
    virtual HardwareSerial* getPort() { return port; }
    // Methods
    void activate() {}

  protected:
    IUI2C *m_iuI2C;
};

#endif // IUESP8285_H
