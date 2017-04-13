#include "IUBattery.h"


char IUBattery::sensorTypes[IUBattery::sensorTypeCount] = {IUABCSensor::sensorType_battery};

IUBattery::IUBattery(IUI2C *iuI2C) :
  IUABCSensor(),
  m_iuI2C(iuI2C),
  m_VDDA(0),
  m_vBattery(0)
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

int IUBattery::getVoltage()
{
  readVoltage();
  return (int) (1000 * m_vBattery);
}

float IUBattery::getVDDA()
{
  readVoltage();
  return m_VDDA;
}

uint8_t IUBattery::getBatteryStatus()
{
  uint8_t battteryStatus  = (uint8_t) ((float) getVoltage() * 100. / (float) maxVoltage);
  return max(min(battteryStatus, 100), 0);
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
      if (m_toSend[i] == (uint8_t) dataSendOption::voltage)
      {
        m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_vBattery);
      }
      else if (m_toSend[i] == (uint8_t) dataSendOption::vdda)
      {
        m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_VDDA);
      }
    }
  }
}
