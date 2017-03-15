#include "IUBattery.h"


char IUBattery::sensorTypes[IUBattery::sensorTypeCount] = {IUABCSensor::sensorType_battery};

IUBattery::IUBattery(IUI2C *iuI2C) : IUABCSensor(), m_iuI2C(iuI2C), m_VDDA(0), m_vBattery(0)
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

float IUBattery::getVoltage()
{
  readVoltage();
  return m_vBattery;
}

float IUBattery::getVDDA()
{
  readVoltage();
  return m_VDDA;
}

void IUBattery::readData()
{
  readVoltage();
}

void IUBattery::sendToReceivers()
{
  for (int i = 0; i < m_receiverCount; i++)
  {
    if (m_receivers[i])
    {
      if (m_toSend[i] == dataSendOption::voltage)
      {
        m_receivers[i]->receive(m_receiverSourceIndex[i], m_vBattery);
      }
      else if (m_toSend[i] == dataSendOption::vdda)
      {
        m_receivers[i]->receive(m_receiverSourceIndex[i], m_VDDA);
      }
    }
  }
}
