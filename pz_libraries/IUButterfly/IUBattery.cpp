#include "IUBattery.h"

IUBattery::IUBattery() : m_VDDA(0), m_vBattery(0)
{
  // Constructor
}

/**
 * Enables the battery readings
 */
void IUBattery::wakeUp()
{
  pinMode(voltagePin, INPUT);        // Enable battery read.
  analogReadResolution(12); // take advantage of 12-bit ADCs
  readVoltage();
}


/**
 * Reads VDDA and the battery voltage
 */
void IUBattery::readVoltage()
{
  m_VDDA = STM32.getVREF();
  m_vBattery = (127.0f / 100.0f) * 3.30f * ((float) analogRead(voltagePin)) / 4095.0f;
}


