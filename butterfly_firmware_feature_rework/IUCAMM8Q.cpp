#include "IUCAMM8Q.h"

using namespace IUComponent;


/* =============================================================================
    Constructors and destructors
============================================================================= */

IUCAMM8Q::IUCAMM8Q(IUI2C *iuI2C, uint8_t id) :
  Sensor(id, 0),
  m_iuI2C(iuI2C),
  m_onTime(defaultOnTime),
  m_period(defaultPeriod),
  m_forcedMode(defaultForcedMode)
{
    // Start GNSS
    GNSS.begin(Serial2, GNSS.MODE_UBLOX, GNSS.RATE_5HZ);
    while (!GNSS.done()) { }
    // Choose satellites
    GNSS.setConstellation(GNSS.CONSTELLATION_GPS);
    while (!GNSS.done()) { }
    GNSS.setSBAS(true);
    while (!GNSS.done()) { }
    wakeUp();
    // GNSS deactivated for now, irregardless of the power mode
    GNSS.sleep();
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set the power mode to ACTIVE
 */
void IUCAMM8Q::wakeUp()
{
    Sensor::wakeUp();
    //setPeriodic(m_onTime, m_period, true);
}

/**
 * Set the power mode to SLEEP
 */
void IUCAMM8Q::sleep()
{
  Sensor::sleep();
  //GNSS.sleep();
}

/**
 * Set the power mode to SUSPEND
 *
 * The SUSPEND mode is actually the same than the SLEEP mode,
 * since GNSS API doesn't offer a suspend mode.
 */
void IUCAMM8Q::suspend()
{
  Sensor::suspend();
  //GNSS.sleep();
}


/* =============================================================================
    Configuration and calibration
============================================================================= */

/**
 * Wrapper around GNSS.setPeriodic method - set the active / inactive behavior
 *
 * @param onTime  the active time (in s) over 'period' duration
 * @param period  the duration of a complete (active + inactive) cycle
 * @param force   if yes, activate forced mode (location can be read even if
 *                device is in its inactive period). If no, location can be read
 *                while inactive.
 */
void IUCAMM8Q::setPeriodic(uint32_t onTime, uint32_t period, bool forced)
{
    m_onTime = onTime;
    m_period = period;
    m_forcedMode = forced;
    GNSS.setPeriodic(onTime, period, forced);  // set periodic wake / sleep mode
    while (!GNSS.done()) { }
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


/* =============================================================================
    Data acquisition
============================================================================= */

/**
 * Read data
 */
void IUCAMM8Q::readData()
{
  m_location = GNSS.read();
}


/* =============================================================================
    Communication
============================================================================= */

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


/* =============================================================================
    Debugging
============================================================================= */

/**
 *
 */
void IUCAMM8Q::exposeCalibration()
{
    #ifdef DEBUGMODE
    // TODO Implement
    #endif
}
