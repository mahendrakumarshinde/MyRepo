#include "IUBattery.h"


/* =============================================================================
    Constructors and destructors
============================================================================= */

IUBattery::IUBattery(IUI2C *iuI2C, const char* name, Feature *batteryLoad) :
  SynchronousSensor(name, 1, batteryLoad),
  m_iuI2C(iuI2C),
  m_VDDA(0),
  m_vBattery(0)
{
}

/**
 * Set up the component and finalize the object initialization
 */
void IUBattery::setupHardware()
{
    pinMode(voltagePin, INPUT); // Enable battery read.
    analogReadResolution(12);   // take advantage of 12-bit ADCs
    wakeUp();
    switchToRegularUsage();
}


/* =============================================================================
    Configuration and calibration
============================================================================= */


void IUBattery::switchToLowUsage()
{
    m_usagePreset = usagePreset::P_LOW;
    setSamplingPeriod(3600000);  // 1 hour
}

void IUBattery::switchToRegularUsage()
{
    m_usagePreset = usagePreset::P_REGULAR;
    setSamplingPeriod(30000);  // 30s
}

void IUBattery::switchToEnhancedUsage()
{
    m_usagePreset = usagePreset::P_ENHANCED;
    setSamplingPeriod(10000);  // 10s
}

void IUBattery::switchToHighUsage()
{
    m_usagePreset = usagePreset::P_HIGH;
    setSamplingPeriod(5000);  // 5s
}


/* =============================================================================
    Data acquisition
============================================================================= */

/**
 * Reads VDDA, battery voltage and compute battery load.
 */
void IUBattery::readData()
{
    m_VDDA = STM32.getVREF();
    m_batteryLoad = (float) (analogRead(voltagePin)) / 40.95f;
    m_vBattery = m_batteryLoad * maxVoltage / 100.0f;
    m_destinations[0]->addFloatValue(m_batteryLoad);
}


/* =============================================================================
    Instantiation
============================================================================= */

IUBattery iuBattery(&iuI2C, "BAT", &batteryLoad);
