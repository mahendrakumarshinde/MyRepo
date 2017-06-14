#include "IUABCSensor.h"

IUABCSensor::IUABCSensor() :
  IUABCProducer(),
  m_downclocking(0),
  m_downclockingCount(0),
  m_asynchronous(false)
{
  m_samplingRate = defaultSamplingRate;
  m_callbackRate = defaultCallbackRate;
}

IUABCSensor::~IUABCSensor()
{
  //dtor
}

/**
 * Convenience function to change the power mode
 * Depending on the powerMode::option, calls the right function
 */
void IUABCSensor::switchToPowerMode(powerMode::option pMode)
{
  switch (pMode)
  {
    case powerMode::ACTIVE:
      wakeUp();
      break;
    case powerMode::SLEEP:
      sleep();
      break;
    case powerMode::SUSPEND:
      suspend();
      break;
    default:
      if (debugMode) { debugPrint("Error - This power mode is not managed by the sensor"); }
      break;
  }
}

/**
 * Rate cannot be set to zero + call prepareDataAcquisition at the end
 */
void IUABCSensor::setSamplingRate(uint16_t samplingRate)
{
  if (samplingRate == 0)
  {
    m_samplingRate = defaultSamplingRate;
  }
  else
  {
    m_samplingRate = samplingRate;
  }
  prepareDataAcquisition();
}

/**
 * Rate cannot be set to zero + call prepareDataAcquisition at the end
 */
void IUABCSensor::setCallbackRate(uint16_t callbackRate)
{
  if (callbackRate == 0)
  {
    m_callbackRate = defaultCallbackRate;
  }
  else
  {
    m_callbackRate = callbackRate;
  }
  prepareDataAcquisition();
}

/**
 * Prepare data acquisition by setting up the sampling rates
 * NB: Should be called everytime the callback or sampling rates
 * are modified.
 */
void IUABCSensor::prepareDataAcquisition()
{
  m_downclocking = m_callbackRate / m_samplingRate;
  m_downclockingCount = 0;
}

/**
 * Acquire new data while handling downclocking
 * @return true if new data were acquired, else false
 */
bool IUABCSensor::acquireData()
{
  m_downclockingCount++;
  if (m_downclocking != m_downclockingCount)
  {
    return false;
  }
  m_downclockingCount = 0;
  readData();
  return true;
}

