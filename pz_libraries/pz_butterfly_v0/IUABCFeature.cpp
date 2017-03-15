#include "IUABCFeature.h"

/* ======================== IUABCFeature Definitions ========================== */

const uint16_t IUABCFeature::sourceSize[IUABCFeature::sourceCount] = {1};


IUABCFeature::IUABCFeature(uint8_t id, String name, String fullName) :
  m_id(id), m_name(name), m_fullName(fullName), m_active(false), m_checkFeature(false)
{
  m_state = operationState::notCutting;
  m_highestDangerLevel = operationState::notCutting;
  prepareSendingQueue(1); // Always prepare the queue with a default size of 1
  m_computeFunction = NULL;
  m_sendingQueueSize = 0;
  m_sendingQueue = NULL;
  setThresholds(0, 0, 0);
}

IUABCFeature::~IUABCFeature()
{
  resetSource();
  delete m_sendingQueue; m_sendingQueue = NULL;
}

/**
 * Reset all source pointers to NULL
 * NB: Do not make this method virtual as it is called in constructors and/ or destructors.
 */
void IUABCFeature::resetSource()
{
  for (uint8_t i = 0; i < 2; i++)
  {
    for (uint16_t j = 0; j < sourceCount; j++)
    {
      delete m_source[i][j]; m_source[i][j] = NULL;
    }
  }
}

/**
 * Set the feature as active: it will then be calculated and streamed
 * @return true if the feature was correctly activated, else false
 * NB: The feature cannot be activated if the source or sending queue have
 * not been set.
 */
bool IUABCFeature::activate()
{
  if (m_source && m_sendingQueue)
  {
    m_active = true;
  }
  else
  {
    m_active = false;
  }
  return m_active;
}

/**
 * Prepare the feature to receive data (receive function can then be used)
 * @return true if sources were correctly prepared, else false.
 * NB: Do not make this method virtual as it is called in contructor.
 * The feature will store data and be calculated once the data buffer is full.
 * The data buffer is sourceSize long.
 */
bool IUABCFeature::prepareSource()
{
  resetSource(); // delete previous pointer assignments
  bool success = newSource(); // Make new pointers
  if (!success)
  {
    // Don't let pointers dangling if failed
    resetSource();
    return false;
  }
  m_computeIndex = 0;
  m_recordIndex = 0;
  // Init arrays
  for (int j = 0; j < sourceCount; j++)
  {
    m_sourceCounter[j] = 0;
    for (int i = 0; i < 2; i++)
    {
      m_computeNow[i][j]= false;
      m_recordNow[i][j] = true;
      for (int k = 0; k < sourceSize[j]; k++)
      {
        m_source[i][j][k] = 0;
      }
    }
  }
  resetCounters();
  return true;
}

/**
 * Reset source counters to start collection from scratch
 * Do this when starting the board (after prepareSource),
 * or when switching operation modes.
 */
void IUABCFeature::resetCounters()
{
  for (uint8_t i = 0; i < sourceCount; i++)
  {
    m_sourceCounter[i] = 0;
    for (uint16_t j = 0; j < 2; j++)
    {
      m_computeNow[j][i] = false;
      m_recordNow[j][i] = true;
    }
  }
  m_computeIndex = 0;
  m_recordIndex = 0;
}

void IUABCFeature::setThresholds(float normalVal, float warningVal, float dangerVal)
{
  m_thresholds[0] = normalVal;
  m_thresholds[1] = warningVal;
  m_thresholds[2] = dangerVal;
}

/**
 * Prepare the feature to store its results in sending queue
 * @return true if sending queue was correctly prepared, else false.
 * NB: Do not make this method virtual as it is called in contructor.
 * A feature can store up to sendingQueueSize calculated value
 */
bool IUABCFeature::prepareSendingQueue(uint16_t sendingQueueSize)
{
  m_sendingQueueSize = sendingQueueSize;
  if (m_sendingQueue)
  {
    delete m_sendingQueue;
  }
  m_sendingQueue = new MD_CirQueue(m_sendingQueueSize, sizeof(float));
  if (m_sendingQueue) // make sure new assignment was successful
  {
    m_sendingQueue->setFullOverwrite(true);
    m_sendingQueue->begin();
    return true;
  }
  else
  {
    m_sendingQueue = NULL; // Don't let pointer dangling
    return false;
  }
}

/**
 * Is it time to switch record buffer?
 */
bool IUABCFeature::isTimeToEndRecord()
{
  //
  bool doNotChange = false;
  for (uint16_t i = 0; i < sourceCount; i++)
  {
    doNotChange |= m_recordNow[m_recordIndex][i];  //if any of the recordNow is true, do not change
  }
  return !doNotChange;
}

/**
 * Compute the feature value and store it
 * @return true if a new value was calculated, else false
 */
bool IUABCFeature::compute()
{
  for (int i = 0; i < sourceCount; i++)
  {
    if(!m_computeNow[m_computeIndex][i]) //Any source not ready for computation
    {
      return false;
    }
  }
  m_latestValue = m_computeFunction(sourceCount, sourceSize, m_source[m_computeIndex]);
  // Since queue overwrite is set to true (see prepareSendingQueue),
  // pushing to a full buffer will pop the 1st item and new item will be pushed.
  m_sendingQueue->push((uint8_t *)&m_latestValue);
  // Free m_source[m_computeIndex] to store new records
  for (int i =0; i < sourceCount; i++)
  {
    m_computeNow[m_computeIndex][i] = false;   //Do not recompute this buffer
    m_recordNow[m_computeIndex][i] = true;     //Buffer ready for recording
  }
  m_computeIndex = m_computeIndex == 0 ? 1 : 0; //switch buffer index
  return true;
}

/**
 * Peek at the next value on the sending queue.
 */
float IUABCFeature::peekNextValue()
{
  float value;
  // peek method handles when queue is empty and will return a NULL pointer
  m_sendingQueue->peek((uint8_t *)&value);
  if (value)
  {
    return value;
  }
  return 0; // if no value in queue, return 0
}

/**
 * Update and return the state of the feature in regards to defined thresholds
 * NB: Also update highestDangerLevel
 * @return 0 is not cutting, 1 is normal, 2 is warning, 3 is danger
 */
operationState IUABCFeature::getOperationState() {
  if (!m_checkFeature)
  {
    return operationState::notCutting; // If feature check is disabled, early return
  }
  float value = peekNextValue();
  for (uint8_t i = 0; i < operationState::stateCount - 1; i++)
  {
    if (value < m_thresholds[i])
    {
      m_state = (operationState) i;
      break;
    }
  }
  m_highestDangerLevel = max(m_highestDangerLevel, m_state);
  return m_state;
}

/**
 * Stream feature id and value through given port
 * @return true if value was available and streamed, else false
 */
bool IUABCFeature::stream(HardwareSerial *port)
{
  if (m_sendingQueue->isEmpty())
  {
      return false; //No feature value available
  }
  else
  {
    float value;
    m_sendingQueue->pop((uint8_t *)&value);
    port->print("000");
    port->print(m_id);
    port->print(",");
    port->print(value);
    return true;
  }
}


