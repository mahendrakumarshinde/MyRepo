#include "IUCAMM8Q.h"


/* =============================================================================
    Constructors and destructors
============================================================================= */

IUCAMM8Q::IUCAMM8Q(IUI2C *iuI2C, const char* name) :
    SynchronousSensor(name, 0),
    m_iuI2C(iuI2C),
    m_onTime(defaultOnTime),
    m_period(defaultPeriod),
    m_forcedMode(defaultForcedMode)
{
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set up the component and finalize the object initialization
 */
void IUCAMM8Q::setupHardware()
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

/**
 * Set the power mode to ACTIVE
 */
void IUCAMM8Q::wakeUp()
{
    SynchronousSensor::wakeUp();
    //setPeriodic(m_onTime, m_period, true);
}

/**
 * Set the power mode to SLEEP
 */
void IUCAMM8Q::sleep()
{
    SynchronousSensor::sleep();
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
    SynchronousSensor::suspend();
    //GNSS.sleep();
}


/* =============================================================================
    Configuration and calibration
============================================================================= */

void IUCAMM8Q::configure(JsonVariant &config)
{
    SynchronousSensor::configure(config);  // General sensor config
    JsonVariant value = config["TON"];  // Active duration
    if (value.success())
    {
        m_onTime = (uint32_t) (value.as<int>());
    }
    value = config["TCY"];  // Cycle duration
    if (value.success())
    {
        m_period = (uint32_t) (value.as<int>());
    }
    value = config["FORCE"];  // Forced mode
    if (value.success())
    {
        m_forcedMode = (bool) (value.as<int>());
    }
    setPeriodic(m_onTime, m_period, m_forcedMode);
}

/**
 * Wrapper around GNSS.setPeriodic method - set the active / inactive behavior
 *
 * @param onTime The active time (in s) over 'period' duration
 * @param period The duration of a complete (active + inactive) cycle
 * @param force If yes, activate forced mode (location can be read even if
 *  device is in its inactive period). If no, location can't be read while
 *  inactive.
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
 * Send geolocation data to serial
 */
void IUCAMM8Q::sendData(HardwareSerial *port)
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


/* =============================================================================
    Instantiation
============================================================================= */

IUCAMM8Q iuGNSS(&iuI2C, "GPS");
