#include "IUESP8285.h"

IUESP8285::IUESP8285(IUI2C *iuI2C) :
  IUABCInterface(),
  m_iuI2C(iuI2C)
{
  //ctor
}

