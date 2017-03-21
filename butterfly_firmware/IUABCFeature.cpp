#include "IUABCFeature.h"

/* ======================== IUABCFeature Definitions ========================== */

const uint16_t IUABCFeature::ABCSourceSize[IUABCFeature::ABCSourceCount] = {1};

/**
 * 
 * Should call newSource or prepareSource immediatly after construction
 */
IUABCFeature::IUABCFeature(uint8_t id, String name, String fullName) :
  m_id(id),
  m_name(name),
  m_fullName(fullName),
  m_active(false),
  m_checkFeature(false),
  m_sendingQueueMaxSize(1),
  m_sourceReady(false)
{
  m_state = operationState::idle;
  m_highestDangerLevel = operationState::idle;
  m_computeFunction = NULL;
  setThresholds(0, 0, 0);
}

IUABCFeature::~IUABCFeature()
{
}

/**
 * Reset all source pointers to NULL
 */
void IUABCFeature::resetSource()
{
  for (uint8_t i = 0; i < 2; i++)
  {
    for (uint16_t j = 0; j < getSourceCount(); j++)
    {
      if(m_source[i][j])
      {
        delete m_source[i][j];
      }
      m_source[i][j] = NULL;
    }
  }
  m_sourceReady = false;
}

/**
 * Set the feature as active: it will then be calculated and streamed
 * @return true if the feature was correctly activated, else false
 * NB: The feature cannot be activated if the source or sending queue have
 * not been set.
 */
bool IUABCFeature::activate()
{
  m_active = m_sourceReady;
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
  for (int j = 0; j < getSourceCount(); j++)
  {
    m_sourceCounter[j] = 0;
    for (int i = 0; i < 2; i++)
    {
      m_computeNow[i][j]= false;
      m_recordNow[i][j] = true;
      for (int k = 0; k < getSourceSize(j); k++)
      {
        m_source[i][j][k] = 0;
      }
    }
  }
  resetCounters();
  m_sourceReady = true;
  return true;
}

/**
 * Reset source counters to start collection from scratch
 * Do this when starting the board (after prepareSource),
 * or when switching operation modes.
 */
void IUABCFeature::resetCounters()
{
  for (uint8_t i = 0; i < getSourceCount(); i++)
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
 * Is it time to switch record buffer?
 */
bool IUABCFeature::isTimeToEndRecord()
{
  //
  bool doNotChange = false;
  for (uint16_t i = 0; i < getSourceCount(); i++)
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
  for (int i = 0; i < getSourceCount(); i++)
  {
    if(!m_computeNow[m_computeIndex][i]) //Any source not ready for computation
    {
      return false;
    }
  }
  m_latestValue = m_computeFunction(getSourceCount(), getSourceSize(), m_source[m_computeIndex]);
  m_sendingQueue.push_back(m_latestValue);
  if (m_sendingQueue.size() >= m_sendingQueueMaxSize) // Manage sending queue size
  {
    m_sendingQueue.pop_front();
  }
  // Free m_source[m_computeIndex] to store new records
  for (int i =0; i < getSourceCount(); i++)
  {
    m_computeNow[m_computeIndex][i] = false;   //Do not recompute this buffer
    m_recordNow[m_computeIndex][i] = true;     //Buffer ready for recording
  }
  m_computeIndex = m_computeIndex == 0 ? 1 : 0; //switch buffer index
  return true;
}

/**
 * Update and return the state of the feature in regards to defined thresholds
 * NB: Also update highestDangerLevel
 * @return 0 is not cutting, 1 is normal, 2 is warning, 3 is danger
 */
operationState IUABCFeature::getOperationState() {
  if (!m_checkFeature)
  {
    return operationState::idle; // If feature check is disabled, early return
  }
  float value = m_sendingQueue.get(0);
  for (uint8_t i = 0; i < operationState::opStateCount - 1; i++)
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
  if (m_sendingQueue.size() > 0)
  {
    port->print("000");
    port->print(m_id);
    port->print(",");
    port->print(m_sendingQueue.get(0));
    m_sendingQueue.pop_front();
    return true;
  }
  return false;
}

/* ====================== Diagnostic Functions, only active when debugMode = true ====================== */
/**
 * Shows feature info: source and queue count and size
 */ 
void IUABCFeature::exposeSourceConfig()
{
  if (!debugMode)
  {
    return; // Inactive if not in debugMode
  }
  uint8_t count = getSourceCount();
  debugPrint("Source count: ", false);
  debugPrint(count);
  debugPrint("Source size:");
  for (int i = 0; i < count; i++)
  {
    debugPrint(getSourceSize(i), false);
    debugPrint(", ", false);
  }
  debugPrint("Queue max size:", false);
  debugPrint(m_sendingQueueMaxSize);
  debugPrint("Queue current size:", false);
  debugPrint(m_sendingQueue.size());
}


/**
 * Shows feature info: compute and record buffer indexes
 */ 
void IUABCFeature::exposeCounterState()
{
  if (!debugMode)
  {
    return; // Inactive if not in debugMode
  }
  uint8_t count = getSourceCount();
  debugPrint("Counters :");
  for (int i = 0; i < count; i++)
  {
    debugPrint(m_sourceCounter[i], false);
    debugPrint(", ");
  }
  debugPrint("Record index :", false);
  debugPrint(m_recordIndex);
  debugPrint("Record now :");
  for (int i = 0; i < count; i++)
  {
    debugPrint(m_recordNow[0][i], false);
    debugPrint(", ", false);
    debugPrint(m_recordNow[1][i], false);
    debugPrint(", ");
  }
  debugPrint("Compute index :", false);
  debugPrint(m_computeIndex);
  debugPrint("Compute now :");
  for (int i = 0; i < count; i++)
  {
    debugPrint(m_computeNow[0][i], false);
    debugPrint(", ", false);
    debugPrint(m_computeNow[1][i], false);
    debugPrint(", ");
  }
}

