#include "IUABCFeature.h"

/* ======================== IUABCFeature Definitions ========================== */

const uint16_t IUABCFeature::ABCSourceSize[IUABCFeature::ABCSourceCount] = {1};

/**
 *
 * Should call prepareSource immediatly after construction
 */
IUABCFeature::IUABCFeature(uint8_t id, char *name) :
  m_id(id),
  m_active(false),
  m_checkFeature(false),
  m_sourceReady(false),
  m_producerReady(false)
{
  strcpy(m_name, name);
  setThresholds(0, 0, 0);
}

IUABCFeature::~IUABCFeature()
{
  resetSource(true);
}

/**
 * Reset all source pointers to NULL
 */
void IUABCFeature::resetSource(bool deletePtr)
{
  uint8_t count = getSourceCount();
  for (uint8_t i = 0; i < 2; i++)
  {
    for (uint16_t j = 0; j < count; j++)
    {
      if (deletePtr)
      {
        delete m_source[i][j];
      }
      m_source[i][j] = NULL;
    }
  }
  m_sourceReady = false;
}

/**
 * Prepare the feature to receive data (receive functions can then be used)
 * @return true if sources were correctly prepared, else false.
 * NB: Do not make this method virtual as it is called in contructor.
 * The feature will store data and be calculated once the data buffer is full.
 * The data buffer is sourceSize long.
 */
bool IUABCFeature::prepareSource()
{
  resetSource(); // reset pointers assignments
  bool success = newSource(); // Make new pointers
  if (!success)
  {
    // Don't let pointers dangling if failed
    resetSource();
    return false;
  }
  // Init arrays
  resetCounters();
  m_sourceReady = true;
  return true;
}

/**
 * Set the feature as active: it will then be calculated and streamed
 * @return true if the feature was correctly activated, else false
 * NB: The feature cannot be activated if the source or sending queue have
 * not been set.
 */
bool IUABCFeature::activate()
{
  m_active = m_sourceReady && m_producerReady;
  return m_active;
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
    m_recordNow[0][i] = true;
    m_recordNow[1][i] = true;
    m_computeNow[0][i] = false;
    m_computeNow[1][i] = false;
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
 * Tells the feature that the array at sourceIndex is ready to be computed
 * @param sourceIndex   the index of the source
 * @return              true if the feature received the values, else false
 */
bool IUABCFeature::receiveArray(uint8_t sourceIndex)
{
  if (!m_recordNow[m_recordIndex][sourceIndex])
  {
     return false; // Recording buffer is not ready
  }
  m_recordNow[m_recordIndex][sourceIndex] = false;         //Do not record anymore in this buffer
  m_computeNow[m_recordIndex][sourceIndex] = true;         //Buffer ready for computation
  // Printing during callbacks can cause issues (do it only for debugging)
  if (callbackDebugMode)
  {
    debugPrint(m_name, false);
    debugPrint(" source #", false);
    debugPrint(sourceIndex, false);
    debugPrint(" is full");
  }
  if (isTimeToEndRecord())
  {
    m_recordIndex = m_recordIndex == 0 ? 1 : 0; //switch buffer index
  }
  return true;
}

/**
 * Is it time to switch record buffer?
 */
bool IUABCFeature::isTimeToEndRecord()
{
  // If any of the recordNow is true, do not change
  for (uint8_t i = 0; i < getSourceCount(); i++)
  {
    if(m_recordNow[m_recordIndex][i])
    {
      return false;
    }
  }
  // Printing during callbacks can cause issues (do it only for debugging)
  if (callbackDebugMode)
  {
    debugPrint(m_name, false);
    debugPrint(F(" all buffers full at idx "), false);
    debugPrint(m_recordIndex, false);
    debugPrint(": ", false);
    debugPrint(millis());
  }
  return true;
}

/**
 * Compute the feature value and store it
 * @return true if a new value was calculated, else false
 */
bool IUABCFeature::compute()
{
  uint32_t startT = 0;
  if (loopDebugMode)
  {
    startT = millis();
  }
  for (int i = 0; i < getSourceCount(); i++)
  {
    if(!m_computeNow[m_computeIndex][i]) // Any source not ready for computation
    {
      return false;
    }
  }
  //Compute first the scalar value, as computeArrayFunctions are often destructive (they modify the source to save space,
  // because they are generally heavier)
  m_computeScalar(m_computeIndex);
  m_computeArray(m_computeIndex);
  if (loopDebugMode)
  {
    debugPrint(m_name, false);
    debugPrint(F("computation => index, time, value: "), false);
    debugPrint(m_computeIndex, false);
    debugPrint(F(", "), false);
    debugPrint(millis() - startT, false);
    debugPrint(F(", "), false);
    debugPrint(getLatestValue());
  }
  for (int i =0; i < getSourceCount(); i++)
  {
    m_computeNow[m_computeIndex][i] = false;   // Do not recompute this buffer
    m_recordNow[m_computeIndex][i] = true;     // Buffer ready for recording
  }
  m_computeIndex = m_computeIndex == 0 ? 1 : 0; //switch buffer index
  return true;
}

/**
 * Update and return the state of the feature in regards to defined thresholds
 * NB: Also update highestDangerLevel
 * @return 0 is not cutting, 1 is normal, 2 is warning, 3 is danger
 */
operationState IUABCFeature::updateState()
{
  for (uint8_t i = 0; i < operationState::opStateCount - 1; i++)
  {
    if (getLatestValue() < m_thresholds[i])
    {
      setState((operationState) i);
      break;
    }
  }
  if (!m_checkFeature) // If feature check is disabled, return idle independently of real state
  {
    return operationState::idle;
  }
  setHighestDangerLevel(max(getHighestDangerLevel(), getState()));
  return getState();
}

/**
 * Stream feature id and value through given port
 * @return true if value was available and streamed, else false
 */
void IUABCFeature::stream(HardwareSerial *port)
{
  port->print("000");
  port->print(m_id);
  port->print(",");
  port->print(getLatestValue());
}

/* ====================== Diagnostic Functions, only active when setupDebugMode = true ====================== */
/**
 * Shows feature info: source and queue count and size
 */
void IUABCFeature::exposeSourceConfig()
{
  #ifdef DEBUGMODE
  uint8_t count = getSourceCount();
  debugPrint(F("Source count: "), false);
  debugPrint(count);
  debugPrint(F("Source size:"), false);
  for (int i = 0; i < count; i++)
  {
    debugPrint(getSourceSize(i), false);
    debugPrint(F(", "), false);
  }
  debugPrint(' ');
  #endif
}


/**
 * Shows feature info: compute and record buffer indexes
 */
void IUABCFeature::exposeCounterState()
{
  #ifdef DEBUGMODE
  debugPrint(F("Counter | recordNow | computeNow :"));
  for (uint8_t i = 0; i < getSourceCount(); i++)
  {
    debugPrint(m_sourceCounter[i], false);
    debugPrint(F(" | "), false);
    debugPrint(m_recordNow[0][i], false);
    debugPrint(F(", "), false);
    debugPrint(m_recordNow[1][i], false);
    debugPrint(F(" | "), false);
    debugPrint(m_computeNow[0][i], false);
    debugPrint(F(", "), false);
    debugPrint(m_computeNow[1][i], false);
    debugPrint(' ');
  }
  debugPrint(F("Record index :"), false);
  debugPrint(m_recordIndex);
  debugPrint(F("Compute index :"), false);
  debugPrint(m_computeIndex);
  #endif
}

