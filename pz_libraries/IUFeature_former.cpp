#include "IUFeature.h"

/* ======================== IUABCFeature Definitions ========================== */

IUABCFeature::IUABCFeature(uint8_t id, String name) : m_id(id), m_name(name), m_active(false), m_checkFeature(false), m_state(0), m_highestDangerLevel(0)
{
  m_computeFunction = NULL;
  m_sourceSize = 0;
  m_source = NULL;
  m_sendingQueueSize = 0;
  m_sendingQueue = NULL;
  setThresholds(0, 0, 0);
}

IUABCFeature::~IUABCFeature()
{
  delete m_source; m_source = NULL;
  delete m_sendingQueue; m_sendingQueue = NULL;
}


/**
 * Prepare the feature to store its results in sending queue
 * @return true if sending queue was correctly prepared, else false.
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

void IUABCFeature::setThresholds(float normal, float warning, float danger)
{
  m_thresholds[0] = normal;
  m_thresholds[1] = warning;
  m_thresholds[2] = danger;
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
 * NB: Also update highestDangerLevel if needed.
 * @return 0 is not cutting, 1 is normal, 2 is warning, 3 is danger
 */
uint8_t IUABCFeature::getThresholdState() {
  if (!m_checkFeature)
  {
    return -1; // If feature check is disabled, early return
  }
  float value = peekNextValue();
  if (value < m_thresholds[0])
  {
    m_state = 0; // level 0: not cutting
  }
  else if (value < m_thresholds[1]) {
    m_state = 1; // level 1: normal cutting
  }
  else if (value < m_thresholds[2]) {
    m_state = 2;  // level 2: warning cutting
  }
  else {
    m_state = 3;  // level 3: bad cutting
  }
  m_highestDangerLevel = max(m_highestDangerLevel, m_state);
  return m_state;
}

/**
 * Stream feature id and value through given port
 * @return true is value was available and streamed, else false
 */
bool IUABCFeature::stream(Stream *port)
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


/* ======================== IUABCProducer Definitions ========================== */

IUABCProducer::IUABCProducer()
{
  resetReceivers();
}

IUABCProducer::~IUABCProducer()
{
  resetReceivers();
}

/**
 * Reset pointers to receivers to NULL 
 * NB: This method should be virtual since it's used in constructor and destructor.
 */
void IUABCProducer::resetReceivers()
{ 
  m_receiverCount = 0;
  delete m_receivers; m_receivers = NULL;
  delete m_receiverSourceIndex; m_receiverSourceIndex = NULL;
  delete m_toSend; m_toSend = NULL;
}

/**
 * Set receivers for this producer
 * @param receiverCount the number of receivers
 * @param receiverSourceIndex points to an array of indexes of the receiver sources to feed
 * @param receivers points to an array of pointers to features
 * @param toSend points to an array of uint8. In child class, each int should be map to a type of data to send to receivers.
 * @return true if pointers were correctly assigned, else false 
 */
bool IUABCProducer::setReceivers(uint8_t receiverCount, uint8_t *receiverSourceIndex, IUABCFeature *receivers, uint8_t *toSend)
{
  if (m_receiverSourceIndex || m_receivers || m_toSend) // if previously assigned pointers
  {
    delete m_receiverSourceIndex;
    delete m_receivers;
    delete m_toSend;
  }
  m_receiverCount = receiverCount;
  m_receiverSourceIndex = receiverSourceIndex;
  m_receivers = receivers;
  m_toSend = toSend;
  if (!(m_receiverSourceIndex && m_receivers)) // if one of the assignment failed
  {
    resetReceivers();
    return false;
  }
  return true;
}

/* ================== IUSingleQ15SourceFeature Definitions ==================== */

IUSingleQ15SourceFeature::IUSingleQ15SourceFeature(uint8_t id, String name) : IUABCProducer(), IUABCFeature(id, name)
{
  // Constructor
}

/**
 * Setter for computeFunction
 */
void IUSingleQ15SourceFeature::setComputeFunction(float (*computeFunction) (uint16_t, q15_t*))
{
  m_computeFunction = computeFunction;
}

/**
 * Set the compute function to a default function returning the first item of the source
 */
void IUSingleQ15SourceFeature::setDefaultComputeFunction()
{
  setComputeFunction(singleQ15SourceDefaultCompute);
}

/**
 * Create a new source
 */
void IUSingleQ15SourceFeature::newSource(uint16_t sourceSize)
{
  const uint16_t test = sourceSize;
  m_source = new q15_t[2][test];
}

/**
 * Prepare the feature to receive data (receive function can then be used)
 * @return true if source was correctly prepared, else false.
 * The feature will store data and be calculated once the data buffer is full.
 * The data buffer is sourceSize long.
 */
bool IUSingleQ15SourceFeature::prepareSource(uint16_t sourceSize)
{
  m_sourceSize = sourceSize;
  if (m_source) delete m_source; // delete previous assignment
  newSource(m_sourceSize);
  if (m_source) // make sure new assignment was successful
  {
    m_sourceCounter = 0;
    m_computeIndex = 0;
    m_computeIndex = 0;
    for (uint8_t i = 0; i < 2; i++)
    {
      m_computeNow[i]= false;
      m_recordNow[i] = true;
      for (uint8_t j = 0; j < m_sourceSize; j++)
      {
        m_source[i][j] = 0;
      }
    }
    return true;
  }
  else
  {
    m_source = NULL;
    return false;
  }
}

/**
 * Receive a new source value It will be stored if the source buffers are ready
 * @param sourceIndex It is not really used, it is just here so that single source and
 * multisource features have the same prototype for receive function
 * @param value The value to add to the source
 * @return true if the value was stored in source buffer, else false
 */
bool IUSingleQ15SourceFeature::receive(uint8_t sourceIndex, q15_t value)
{
  if (!m_recordNow[m_recordIndex])              // Recording buffer is not ready
  {
     return false;
  }
  if (m_sourceCounter < m_sourceSize)           // Recording buffer is not full
  {
      m_source[m_recordIndex][m_sourceCounter] = value;
      m_sourceCounter++;
  }
  if (m_sourceCounter == m_sourceSize)          // Recording buffer is now full
  {
    m_recordNow[m_recordIndex] = false;         //Do not record anymore in this buffer
    m_computeNow[m_recordIndex] = true;         //Buffer ready for computation
    m_recordIndex = m_recordIndex == 0 ? 1 : 0; //switch buffer index
    m_sourceCounter = 0;                        //Reset counter
  }
  return true;
}

/**
 * Compute the feature value, store it in sending queue, send it to its receivers and return it
 */
float IUSingleQ15SourceFeature::compute()
{
  if(!m_computeNow[m_computeIndex]) //Buffer not ready for computation
  {
    return 0;
  }
  float value = m_computeFunction(m_sourceSize, m_source[m_computeIndex]);
  // Since queue overwrite is set to true (see prepareSendingQueue),
  // pushing to a full buffer will pop the 1st item and new item will be pushed.
  m_sendingQueue->push((uint8_t *)&value);
  // Free m_source[m_computeIndex] to store new records
  m_computeNow[m_computeIndex] = false;   //Do not recompute this buffer
  m_recordNow[m_computeIndex] = true;     //Buffer ready for recording
  m_computeIndex = m_computeIndex == 0 ? 1 : 0; //switch buffer index
  for (int i = 0; i < m_receiverCount; i++)
  {
    if (m_receivers[i])
    {
      m_receivers[i].receive(value);
    }
  }
  return value;
}


/* ================== IUSingleFloatSourceFeature Definitions ==================== */


/**
 * Setter for computeFunction
 */
void IUSingleFloatSourceFeature::setComputeFunction(float (*computeFunction) (uint16_t, float*))
{
  m_computeFunction = computeFunction;
}

/**
 * Set the compute function to a default function returning the first item of the source
 */
void IUSingleFloatSourceFeature::setDefaultComputeFunction()
{
  setComputeFunction(singleFloatSourceDefaultCompute);
}

/**
 * Create a new source
 */
void IUSingleFloatSourceFeature::newSource(uint16_t sourceSize)
{
  m_source = new float[2][m_sourceSize];
}

/**
 * Receive a new source valu, it will be stored if the source buffers are ready
 * @param sourceIndex It is not really used, it is just here so that single source and
 * multisource features have the same prototype for receive function
 * @param value The value to add to the source
 * @return true if the value was stored in source buffer, else false
 */
bool IUSingleFloatSourceFeature::receive(uint8_t sourceIndex, float value)
{
  if (!m_recordNow[m_recordIndex])              // Recording buffer is not ready
  {
     return;
  }
  if (m_sourceCounter < m_sourceSize)           // Recording buffer is not full
  {
      m_source[m_recordIndex][m_sourceCounter] = value;
      m_sourceCounter++;
  }
  if (m_sourceCounter == m_sourceSize)          // Recording buffer is now full
  {
    m_recordNow[m_recordIndex] = false;         //Do not record anymore in this buffer
    m_computeNow[m_recordIndex] = true;         //Buffer ready for computation
    m_recordIndex = m_recordIndex == 0 ? 1 : 0; //switch buffer index
    m_sourceCounter = 0;                        //Reset counter
  }
}


/* ================== IUMultiQ15SourceFeature Definitions ==================== */

IUMultiQ15SourceFeature::IUMultiQ15SourceFeature(uint8_t id, String name) : IUABCProducer(), m_id(id), m_name(name), m_active(false), m_checkFeature(false), m_state(0), m_highestDangerLevel(0)
{
  m_computeFunction = NULL;
  m_sourceCount = 0;
  m_sourceSize = NULL;
  m_source = NULL;
  m_computeNow = NULL;
  m_recordNow = NULL;
  m_computeIndex = NULL;
  m_recordIndex = NULL;
  m_sendingQueueSize = 0;
  m_sendingQueue = NULL;
  setThresholds(0, 0, 0);
}

IUMultiQ15SourceFeature::~IUMultiQ15SourceFeature()
{
  // Delete all pointers
  delete m_sendingQueue; m_sendingQueue = NULL;
  delete m_sourceSize; m_sourceSize = NULL;
  delete m_computeNow; m_computeNow = NULL;
  delete m_recordNow; m_recordNow = NULL;
  delete m_computeIndex; m_computeIndex = NULL;
  delete m_recordIndex; m_recordIndex = NULL;
  for (int i=0, i < m_sourceCount)
  {
    delete m_source[i]; m_source = NULL;
  }
  delete m_source; m_source = NULL;
}

/**
 * Setter for computeFunction
 */
void IUMultiQ15SourceFeature::setComputeFunction(float (*computeFunction) (uint8_t, uint16_t*, q15_t*))
{
  m_computeFunction = computeFunction;
}

/**
 * Set the compute function to a default function returning the first item of the source
 */
void IUMultiQ15SourceFeature::setDefaultComputeFunction()
{
  setComputeFunction(multiQ15SourceDefaultCompute);
}

/**
 * Creates the source pointers
 */
void IUMultiQ15SourceFeature::newSource(uint8_t sourceCount, uint16_t *sourceSize)
{
  m_source = new q15_t[2][m_sourceCount];
  for (uint8_t i = 0; i < 2; i++)
  {
    for (uint8_t j = 0; j < sourceCount; j++)
    {
      m_source[i][j] = new q15_t[sourceSize[j]];
    }
  }
}

/**
 * Prepare the feature to receive data (receive function can then be used)
 * @return true if sources were correctly prepared, else false.
 * The feature will store data and be calculated once the data buffer is full.
 * The data buffer is sourceSize long.
 */
bool IUMultiQ15SourceFeature::prepareSource(uint8_t sourceCount, uint16_t *sourceSize)
{
  m_sourceCount = sourceCount;
  m_sourceSize = sourceSize;
  // delete previous pointer assignments
  if (m_sourceCounter) delete m_sourceCounter;
  if (m_computeNow) delete m_computeNow;
  if (m_recordNow) delete m_recordNow;
  if (m_computeIndex) delete m_computeIndex;
  if (m_recordIndex) delete m_recordIndex;
  if (m_source) delete m_source;
  // Make new pointers
  m_sourceCounter = new uint16_t[m_sourceCount];
  m_computeNow = new bool[2][m_sourceCount];
  m_recordNow = new bool[2][m_sourceCount];
  m_computeIndex = new uint8_t[m_sourceCount];
  m_recordIndex = new uint8_t[m_sourceCount];
  newSource(sourceCount, sourceSize)
   // make sure new pointer assignments were successful
  if (m_sourceCounter && m_computeNow && m_recordNow && m_computeIndex && m_recordIndex && m_source)
  {
    for (int j = 0; j < m_sourceCount; j++)
    {
      m_sourceCounter[j] = 0;
      m_computeIndex[j] = 0;
      m_computeIndex[j] = 0;
      for (int i = 0; i < 2; i++)
      {
        m_computeNow[i][j]= false;
        m_recordNow[i][j] = true;
        for (int k = 0; k < m_sourceSize[j]; k++)
        {
          m_source[i][j][k] = 0;
        }
      }
    }
    return true;
  }
  else // Don't let pointers be dangling
  {
    m_sourceCounter = NULL;
    m_computeNow = NULL;
    m_recordNow = NULL;
    m_computeIndex = NULL;
    m_recordIndex = NULL;
    m_source = NULL;
    return false;
  }
}

/**
 * Receive a new source value It will be stored if the source buffers are ready
 * @return true if the value was stored in source buffer, else false
 */
bool IUMultiQ15SourceFeature::receive(uint8_t sourceIndex, q15_t value)
{
  if (!m_recordNow[sourceIndex][m_recordIndex])              // Recording buffer is not ready
  {
     return;
  }
  if (m_sourceCounter[sourceIndex] < m_sourceSize[sourceIndex])           // Recording buffer is not full
  {
      m_source[m_recordIndex][sourceIndex][m_sourceCounter[sourceIndex]] = value;
      m_sourceCounter[sourceIndex]++;
  }
  if (m_sourceCounter[sourceIndex] == m_sourceSize[sourceIndex])          // Recording buffer is now full
  {
    m_recordNow[sourceIndex][m_recordIndex[sourceIndex]] = false;         //Do not record anymore in this buffer
    m_computeNow[sourceIndex][m_recordIndex[sourceIndex]] = true;         //Buffer ready for computation
    m_recordIndex[sourceIndex] = m_recordIndex[sourceIndex] == 0 ? 1 : 0; //switch buffer index
    m_sourceCounter[sourceIndex] = 0;                        //Reset counter
  }
}

/**
 * Compute the feature value, store it, send it to its receivers and return it
 */
float IUMultiQ15SourceFeature::compute()
{
  for (int i =0; i < m_sourceCount; i++)
  {
    if(!m_computeNow[i][m_computeIndex[i]]) //Any source not ready for computation
    {
      return 0;
    }
  }
  float value = m_computeFunction(m_source[m_computeIndex]);
  // Since queue overwrite is set to true (see prepareSendingQueue),
  // pushing to a full buffer will pop the 1st item and new item will be pushed.
  m_sendingQueue->push((uint8_t *)&value);
  // Free m_source[m_computeIndex] to store new records
  for (int i =0; i < m_sourceCount; i++)
  {
    m_computeNow[i][m_computeIndex[i]] = false;   //Do not recompute this buffer
    m_recordNow[i][m_computeIndex[i]] = true;     //Buffer ready for recording
    m_computeIndex[i] = m_computeIndex[i] == 0 ? 1 : 0; //switch buffer index
  }
  for (int i = 0; i < m_receiverCount; i++)
  {
    if (m_receivers[i])
    {
      m_receivers[i].receive(value);
    }
  }
  return value;
}


/* ================== IUMultiFloatSourceFeature Definitions ==================== */


/**
 * Setter for computeFunction
 */
void IUMultiFloatSourceFeature::setComputeFunction(float (*computeFunction) (uint8_t, uint16_t*, float*))
{
  m_computeFunction = computeFunction;
}

/**
 * Set the compute function to a default function returning the first item of the source
 */
void IUMultiFloatSourceFeature::setDefaultComputeFunction()
{
  setComputeFunction(multiFloatSourceDefaultCompute);
}

/**
 * Creates the source pointers
 */
void IUMultiQ15SourceFeature::newSource(uint8_t sourceCount, uint16_t *sourceSize)
{
  m_source = new float[2][m_sourceCount];
  for (uint8_t i = 0; i < 2; i++)
  {
    for (uint8_t j = 0; j < sourceCount; j++)
    {
      m_source[i][j] = new float[sourceSize[j]];
    }
  }
}

/**
 * Receive a new source valu, it will be stored if the source buffers are ready
 * @return true if the value was stored in source buffer, else false
 */
bool IUMultiQ15SourceFeature::receive(uint8_t sourceIndex, float value)
{
  if (!m_recordNow[sourceIndex][m_recordIndex])              // Recording buffer is not ready
  {
     return;
  }
  if (m_sourceCounter[sourceIndex] < m_sourceSize[sourceIndex])           // Recording buffer is not full
  {
      m_source[m_recordIndex][sourceIndex][m_sourceCounter[sourceIndex]] = value;
      m_sourceCounter[sourceIndex]++;
  }
  if (m_sourceCounter[sourceIndex] == m_sourceSize[sourceIndex])          // Recording buffer is now full
  {
    m_recordNow[sourceIndex][m_recordIndex[sourceIndex]] = false;         //Do not record anymore in this buffer
    m_computeNow[sourceIndex][m_recordIndex[sourceIndex]] = true;         //Buffer ready for computation
    m_recordIndex[sourceIndex] = m_recordIndex[sourceIndex] == 0 ? 1 : 0; //switch buffer index
    m_sourceCounter[sourceIndex] = 0;                        //Reset counter
  }
}


/* ================== IUMultiQ15SourceDataCollectionFeature Definitions ==================== */

IUMultiQ15SourceDataCollectionFeature::IUMultiQ15SourceDataCollectionFeature(uint8_t id, String name) : IUMultiQ15SourceFeature(id, name)
{
  m_sourceNames = NULL;
}

IUMultiQ15SourceDataCollectionFeature::~IUMultiQ15SourceDataCollectionFeature()
{
  for (int i=0, i < m_sourceCount)
  {
    delete m_sourceNames[i]; m_sourceNames = NULL;
  }
  delete m_sourceNames; m_sourceNames = NULL;
}


/**
 * Set the feature as active: it will then be calculated and streamed
 * @return true if the feature was correctly activated, else false
 * NB: The feature cannot be activated if the source or sending queue have
 * not been set.
 */
bool IUMultiQ15SourceDataCollectionFeature::activate()
{
  m_active = (bool) m_source;
  return m_active;
}

bool IUMultiQ15SourceDataCollectionFeature::stream(Stream *port)
{
  for (int i =0; i < m_sourceCount; i++)
  {
    if(!m_computeNow[i][m_computeIndex[i]]) //Any source not ready for streaming
    {
      return false;
    }
  }
  for (int i = 0; i < m_sourceCount; i++)
  {
    port->print(m_sourceNames[i]);
    port->print(",");
    port->flush();
    for (int j = 0; j < m_sourceSize[i]; j++)
    {
      port->print(m_dataTransform(m_source[m_computeIndex][i][j]));
      port->print(",");
      port->flush();
    }
    port->print(";");
    port->flush();
  }
  // Free m_source[m_computeIndex] to store new records
  for (int i =0; i < m_sourceCount; i++)
  {
    m_computeNow[i][m_computeIndex[i]] = false;   //Do not recompute this buffer
    m_recordNow[i][m_computeIndex[i]] = true;     //Buffer ready for recording
    m_computeIndex[i] = m_computeIndex[i] == 0 ? 1 : 0; //switch buffer index
  }
  return true;
}


/* ===================================== Feature Instanciations ======================================== */


uint16_t accelSourceSize[3] = {512, 512, 512};
uint16_t velocitySourceSize[3] = {1, 1, 512};

showRecordFFT = IUMultiQ15SourceDataCollectionFeature(0, "accel_data");
showRecordFFT.prepareSource(3, accelSourceSize);
showRecordFFT.setDefaultDataTransform();
showRecordFFT.activate();

accelerationEnergy = IUMultiQ15SourceFeature(1, "acceleration_energy");
accelerationEnergy.prepareSource(3, accelSourceSize);
accelerationEnergy.prepareSendingQueue(1);
accelerationEnergy.setComputeFunction(computeSignalEnergy);
accelerationEnergy.activate();

velocityX = IUMultiQ15SourceFeature(2, "velocity_x");
VelocityX.prepareSource(3, velocitySourceSize);
VelocityX.prepareSendingQueue(1);
VelocityX.setComputeFunction(computeVelocity);
VelocityX.activate();

velocityY = IUMultiQ15SourceFeature(3, "velocity_y");
velocityY.prepareSource(3, velocitySourceSize);
velocityY.prepareSendingQueue(1);
velocityY.setComputeFunction(computeVelocity);
velocityY.activate();

velocityZ = IUMultiQ15SourceFeature(4, "velocity_z");
velocityZ.prepareSource(3, velocitySourceSize);
velocityZ.prepareSendingQueue(1);
velocityZ.setComputeFunction(computeVelocity);
velocityZ.activate();

temperature = IUSingleFloatSourceFeature(5, "current_temperature");
temperature.prepareSource(1);
temperature.prepareSendingQueue(1);
temperature.setDefaultComputeFunction();
temperature.activate();

audioDB = IUSingleFloatSourceFeature(6, "audio_db");
audioDB.prepareSource(1);
audioDB.prepareSendingQueue(1);
audioDB.setDefaultComputeFunction();
audioDB.activate();


/* ================ Definitions for IUFeatureSelection class ================= */

/**
 * Collection of features is initially empty
 */
IUFeatureSelection::IUFeatureSelection() : m_size(0)
{
  //ctor
}

/**
 * Attempt to add a feature to the collection
 * @return true if success, else false (because the collection is full)
 */
bool IUFeatureSelection::addFeature(IUABCFeature feature)
{
    if (m_size == MAX_SIZE)
    {
        return false; //Unable to add a new feature because the collection is full
    }
    m_features[m_size] = feature;
    m_size++;
    return true;
}

IUABCFeature IUFeatureSelection::getFeature(uint8_t index)
{
    if (index < m_size)
    {
        return m_features[index];
    }
    // Else... Unable to throw exception with Arduino
}

