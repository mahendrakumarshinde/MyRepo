#include "IUABCSensor.h"

char IUABCSensor::sensorTypes[IUABCSensor::sensorTypeCount] = {IUABCSensor::sensorType_none};

IUABCSensor::IUABCSensor() : IUABCProducer(), m_downclocking(0), m_downclockingCount(0)
{
  m_samplingRate = defaultSamplingRate;
  m_callbackRate = defaultCallbackRate;
}

IUABCSensor::~IUABCSensor()
{
  //dtor
}

/**
 * Rate cannot be set to zero + call prepareDataAcquisition at the end
 */
void IUABCSensor::setSamplingRate(uint32_t samplingRate)
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
void IUABCSensor::setCallbackRate(uint32_t callbackRate)
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

