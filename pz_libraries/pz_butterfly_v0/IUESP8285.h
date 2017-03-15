#ifndef IUESP8285_H
#define IUESP8285_H

#include <Arduino.h>
#include "IUUtilities.h"
#include "IUI2C.h"

class IUESP8285 : public IUInterface
{
  public:
    IUESP8285(IUI2C *iuI2C);
    virtual ~IUESP8285() {}
    void activate() {}

  protected:
    IUI2C *m_iuI2C;
};

#endif // IUESP8285_H
