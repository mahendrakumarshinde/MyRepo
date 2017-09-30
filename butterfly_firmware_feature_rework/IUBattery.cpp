#include "IUBattery.h"

using namespace IUComponent;


/* =============================================================================
    Constructors and destructors
============================================================================= */

IUBattery::IUBattery(IUI2C *iuI2C, uint8_t id,
                     FeatureBuffer *batteryLoad) :
  Sensor(id, destinationCount=1, batteryLoad),
  m_iuI2C(iuI2C),
  m_VDDA(0),
  m_vBattery(0)
{
    pinMode(voltagePin, INPUT); // Enable battery read.
    analogReadResolution(12);   // take advantage of 12-bit ADCs
    wakeUp();
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
    m_vBattery = 1000 * (127.0f / 100.0f) * 3.30f * \
        ((float) analogRead(voltagePin)) / 4095.0f;
    m_batteryLoad = (m_vBattery * 100. / (float) maxVoltage);
    m_destinations[0]->addFloatValue(m_batteryLoad);
}
