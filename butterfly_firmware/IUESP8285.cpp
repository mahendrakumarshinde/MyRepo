#include "IUESP8285.h"


/* ============================ Constructors, destructor, getters, setters ============================ */

IUESP8285::IUESP8285(IUI2C *iuI2C) :
  IUABCInterface(),
  m_iuI2C(iuI2C)
{
  //ctor
}


/* ============================  Hardware & power management methods ============================ */

/**
 * Switch to ACTIVE power mode
 */
void IUESP8285::wakeUp()
{
  m_powerMode = powerMode::ACTIVE;
}

/**
 * Switch to SLEEP power mode
 */
void IUESP8285::sleep()
{
  m_powerMode = powerMode::SLEEP;
}

/**
 * Switch to SUSPEND power mode
 */
void IUESP8285::suspend()
{
  m_powerMode = powerMode::SUSPEND;
}

