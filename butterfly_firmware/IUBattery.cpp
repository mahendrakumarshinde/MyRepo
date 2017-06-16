#include "IUBattery.h"


/* ================================= Method definition ================================= */

IUBattery::IUBattery(IUI2C *iuI2C) :
  IUABCSensor(),
  m_iuI2C(iuI2C),
  m_VDDA(0),
  m_vBattery(0)
{
  pinMode(voltagePin, INPUT); // Enable battery read.
  analogReadResolution(12);   // take advantage of 12-bit ADCs
  readVoltage();
}


/* =========================== Hardware and power management methods =========================== */

/**
 * Switch to ACTIVE power mode
 */
void IUBattery::wakeUp()
{
  m_powerMode = powerMode::ACTIVE;
}

/**
 * Switch to SLEEP power mode
 */
void IUBattery::sleep()
{
  m_powerMode = powerMode::SLEEP;
}

/**
 * Switch to SUSPEND power mode
 */
void IUBattery::suspend()
{
  m_powerMode = powerMode::SUSPEND;
}


/* =========================== Data acquisition methods =========================== */

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


/* =========================== Communication methods =========================== */

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
