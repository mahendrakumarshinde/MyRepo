#include "IUBattery.h"


/* =============================================================================
    Constructors and destructors
============================================================================= */

IUBattery::IUBattery(const char* name, Feature *batteryLoad) :
  LowFreqSensor(name, 1, batteryLoad),
  m_vBattery(0)
{
}


/* =============================================================================
    Hardware and power management
============================================================================= */

/**
 * Set up the component and finalize the object initialization
 */
void IUBattery::setupHardware()
{
    pinMode(voltagePin, INPUT); // Enable battery read.
    analogReadResolution(12);   // take advantage of 12-bit ADCs
    setPowerMode(PowerMode::REGULAR);
}

/**
 * Manage component power modes
 */
void IUBattery::setPowerMode(PowerMode::option pMode)
{
    m_powerMode = pMode;
    switch (m_powerMode)
    {
        case PowerMode::PERFORMANCE:
        case PowerMode::ENHANCED:
            setSamplingPeriod(5000);  // 5s
            break;
        case PowerMode::REGULAR:
        case PowerMode::LOW_1:
        case PowerMode::LOW_2:
            setSamplingPeriod(30000);  // 30s
            break;
        case PowerMode::SLEEP:
        case PowerMode::DEEP_SLEEP:
        case PowerMode::SUSPEND:
            setSamplingPeriod(3600000);  // 1 hour
            break;
        default:
            if (debugMode)
            {
                debugPrint(F("Unhandled power Mode "), false);
                debugPrint(m_powerMode);
            }
    }
}


/* =============================================================================
    Data acquisition
============================================================================= */

/**
 * Reads battery voltage and compute battery load.
 */
void IUBattery::readData()
{
    m_batteryLoad = (float) (analogRead(voltagePin)) / 40.95f;
    m_vBattery = m_batteryLoad * maxVoltage / 100.0f;
    m_destinations[0]->addFloatValue(m_batteryLoad);
}
