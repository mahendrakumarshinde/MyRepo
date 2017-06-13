#include "IUCAMM8Q.h"

/* ================================= Method definition ================================= */

IUCAMM8Q::IUCAMM8Q(IUI2C *iuI2C) :
  IUABCSensor(),
  m_iuI2C(iuI2C),
  m_onTime(defaultOnTime),
  m_period(defaultPeriod),
  m_forcedMode(defaultForcedMode)
{
  GNSS.begin(Serial2, GNSS.MODE_UBLOX, GNSS.RATE_5HZ); // Start GNSS
  while (!GNSS.done()) { }                             // wait for begin to complete
  GNSS.setConstellation(GNSS.CONSTELLATION_GPS);       // choose satellites
  while (!GNSS.done()) { }                             // wait for set to complete
  GNSS.setSBAS(true);                                  // choose satellites
  while (!GNSS.done()) { }                             // wait for set to complete
  setPeriodic(m_onTime, m_period, m_forcedMode);       // set periodic wake and sleep mode
  while (!GNSS.done()) { }                             // wait for set to complete
}


/* ============================ Hardware & power management methods ============================ */

/**
 * Set the power mode to ACTIVE
 */
void IUCAMM8Q::wakeUp()
{
  m_powerMode = powerMode::ACTIVE;
  GNSS.wakeup();
}

/**
 * Set the power mode to SLEEP
 */
void IUCAMM8Q::sleep()
{
  m_powerMode = powerMode::SLEEP;
  GNSS.sleep();
}

/**
 * Set the power mode to SUSPEND
 * The SUSPEND mode is actually the same than the SLEEP mode,
 * since GNSS API doesn't offer a suspend mode.
 */
void IUCAMM8Q::suspend()
{
  m_powerMode = powerMode::SUSPEND;
  GNSS.sleep();
}

/**
 * Wrapper around GNSS.setPeriodic method - set the active / inactive behavior
 * @param onTime  the active time (in s) over 'period' duration
 * @param period  the duration of a complete (active + inactive) cycle
 * @param force   if yes, activate forced mode (location can be read even if device
 *                is in its inactive period). If no, location can be read while inactive.
 */
void IUCAMM8Q::setPeriodic(uint32_t onTime, uint32_t period, bool forced)
{
  m_onTime = onTime;
  m_period = period;
  m_forcedMode = forced;
  GNSS.setPeriodic(onTime, period, force); // set periodic wake and sleep mode
  while (!GNSS.done()) { }                 // wait for set to complete
}

/**
 * Enter Forced mode: location can be read even in SLEEP mode
 */
void IUCAMM8Q::enterForcedMode()
{
  setPeriodic(m_onTime, m_period, true);
}

/**
 * Exit Forced mode
 */
void IUCAMM8Q::exitForcedMode()
{
  setPeriodic(m_onTime, m_period, false);
}


/* ============================ Data acquisition methods ============================ */

/**
 *
 */
void IUCAMM8Q::readData()
{
  m_location = GNSS.read();
}


/* ============================ Communication methods ============================ */

/**
 *
 */
void IUCAMM8Q::sendToReceivers()
{
  // TODO Implement
}

/**
 *
 */
void IUCAMM8Q::dumpDataThroughI2C()
{
  // TODO Implement
}

/**
 *
 */
void IUCAMM8Q::dumpDataForDebugging()
{
  // TODO Implement
}


/* ============================ Diagnostic methods ============================ */

/**
 *
 */
void IUCAMM8Q::exposeCalibration()
{

}
