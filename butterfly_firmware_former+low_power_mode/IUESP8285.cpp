#include "IUESP8285.h"


/* ============================ Constructors, destructor, getters, setters ============================ */

IUESP8285::IUESP8285(IUI2C *iuI2C) :
  IUABCInterface(),
  m_iuI2C(iuI2C)
{
  wakeUp();
}

void IUESP8285::setBaudRate(uint32_t baudRate)
{
  m_baudRate = baudRate;
  port->flush();
  delay(2);
  port->end();
  delay(10);
  port->begin(m_baudRate);
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
  //WiFiSerial::Put_WiFi_To_Sleep(0.0f); // Passing 0 makes the WiFi sleep indefinitly
}

/**
 * Switch to SUSPEND power mode
 */
void IUESP8285::suspend()
{
  m_powerMode = powerMode::SUSPEND;
  //WiFiSerial::Put_WiFi_To_Sleep(0.0f); // Passing 0 makes the WiFi sleep indefinitly
}

