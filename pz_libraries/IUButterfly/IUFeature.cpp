#include "IUFeature.h"


/* ================== IUQ15DataCollectionFeature Definitions ==================== */

const uint16_t IUQ15DataCollectionFeature::sourceSize[IUQ15DataCollectionFeature::sourceCount] = {1};

IUQ15DataCollectionFeature::IUQ15DataCollectionFeature(uint8_t id, String name) : IUABCFeature(id, name), m_streamNow(false)
{
  for (int i =0; i < sourceCount; i++)
  {
    m_sourceNames[i] = "";
  }
}

void IUQ15DataCollectionFeature::setSourceNames(String *sourceNames)
{
  for (int i =0; i < sourceCount; i++)
  {
    m_sourceNames[i] = sourceNames[i];
  }
}

float IUQ15DataCollectionFeature::compute()
{
  if (m_streamNow)
  {
    return 0;
  }
  IUABCFeature::compute();
  m_streamNow = true;
  return m_latestValue;
}


/**
 *
 */
bool IUQ15DataCollectionFeature::stream(Stream *port)
{
  if (!m_streamNow)
  {
    return false;
  }
  for (int i = 0; i < sourceCount; i++)
  {
    port->print(m_sourceNames[i]);
    port->print(",");
    port->flush();
    for (int j = 0; j < sourceSize[i]; j++)
    {
      port->print(m_destination[i][j]);
      port->print(",");
      port->flush();
    }
    port->print(";");
    port->flush();
  }
  m_streamNow = false;
  return true;
}


/* ================== IUQ15Feature Definitions ==================== */

const uint16_t IUQ15Feature::sourceSize[IUQ15Feature::sourceCount] = {1};

IUQ15Feature::IUQ15Feature(uint8_t id, String name) : IUABCFeature(id, name), IUABCProducer()
{
  // construtor
}

/**
 * Creates the source pointers
 * @return true is pointer assignments succeeded, else false
 */
bool IUQ15Feature::newSource()
{
  for (uint8_t i = 0; i < 2; i++)
  {
    for (uint16_t j = 0; j < sourceCount; j++)
    {
      m_source[i][j] = new q15_t[sourceSize[j]];
      if (!m_source[i][j]) // Pointer assignment failed
      {
        return false;
      }
    }
  }
  return true;
}

/**
 * Receive a new source value It will be stored if the source buffers are ready
 * @return true if the value was stored in source buffer, else false
 */
bool IUQ15Feature::receive(uint8_t sourceIndex, q15_t value)
{
  if (!m_recordNow[m_recordIndex][sourceIndex])                         // Recording buffer is not ready
  {
     return false;
  }
  if (m_sourceCounter[sourceIndex] < sourceSize[sourceIndex])           // Recording buffer is not full
  {
      m_source[m_recordIndex][sourceIndex][m_sourceCounter[sourceIndex]] = value;
      m_sourceCounter[sourceIndex]++;
  }
  if (m_sourceCounter[sourceIndex] == sourceSize[sourceIndex])          // Recording buffer is now full
  {
    m_recordNow[m_recordIndex][sourceIndex] = false;         //Do not record anymore in this buffer
    m_computeNow[m_recordIndex][sourceIndex] = true;         //Buffer ready for computation
    m_sourceCounter[sourceIndex] = 0;                        //Reset counter
  }
  if (isTimeToEndRecord())
  {
    m_recordIndex = m_recordIndex == 0 ? 1 : 0; //switch buffer index
  }
  return true;
}


float IUQ15Feature::compute()
{
  IUABCFeature::compute();
  sendToReceivers();
  return m_latestValue;
}


/* ================== IUFloatFeature Definitions ==================== */

const uint16_t IUFloatFeature::sourceSize[IUFloatFeature::sourceCount] = {1};

IUFloatFeature::IUFloatFeature(uint8_t id, String name) : IUABCFeature(id, name), IUABCProducer()
{
  // construtor
}

/**
 * Creates the source pointers
 * @return true is pointer assignments succeeded, else false
 */
bool IUFloatFeature::newSource()
{
  for (uint8_t i = 0; i < 2; i++)
  {
    for (uint16_t j = 0; j < sourceCount; j++)
    {
      m_source[i][j] = new float[sourceSize[j]];
      if (!m_source[i][j]) // Pointer assignment failed
      {
        return false;
      }
    }
  }
  return true;
}

/**
 * Receive a new source valu, it will be stored if the source buffers are ready
 * @return true if the value was stored in source buffer, else false
 */
bool IUFloatFeature::receive(uint8_t sourceIndex, float value)
{
  if (!m_recordNow[m_recordIndex][sourceIndex])              // Recording buffer is not ready
  {
     return false;
  }
  if (m_sourceCounter[sourceIndex] < sourceSize[sourceIndex])           // Recording buffer is not full
  {
      m_source[m_recordIndex][sourceIndex][m_sourceCounter[sourceIndex]] = value;
      m_sourceCounter[sourceIndex]++;
  }
  if (m_sourceCounter[sourceIndex] == sourceSize[sourceIndex])          // Recording buffer is now full
  {
    m_recordNow[m_recordIndex][sourceIndex] = false;         //Do not record anymore in this buffer
    m_computeNow[m_recordIndex][sourceIndex] = true;         //Buffer ready for computation
    m_sourceCounter[sourceIndex] = 0;                        //Reset counter
  }
  if (isTimeToEndRecord())
  {
    m_recordIndex = m_recordIndex == 0 ? 1 : 0; //switch buffer index
  }
  return true;
}

float IUFloatFeature::compute()
{
  IUABCFeature::compute();
  sendToReceivers();
  return m_latestValue;
}

/* ========================== Speicalized Feature Classes ============================= */

const uint16_t IUSingleAxisEnergyFeature128::sourceSize[IUSingleAxisEnergyFeature128::sourceCount] = {128};

void IUSingleAxisEnergyFeature128::sendToReceivers()
{
  if (m_receivers[0])
  {
    m_receivers[0]->receive(m_receiverSourceIndex[0], (q15_t) m_state);
  }
}


const uint16_t IUSingleAxisEnergyFeature512::sourceSize[IUSingleAxisEnergyFeature512::sourceCount] = {512};

void IUSingleAxisEnergyFeature512::sendToReceivers()
{
  if (m_receivers[0])
  {
    m_receivers[0]->receive(m_receiverSourceIndex[0], (q15_t) m_state);
  }
}


const uint16_t IUTriAxisEnergyFeature128::sourceSize[IUTriAxisEnergyFeature128::sourceCount] = {128, 128, 128};

void IUTriAxisEnergyFeature128::sendToReceivers()
{
  if (m_receivers[0])
  {
    m_receivers[0]->receive(m_receiverSourceIndex[0], (q15_t) m_state);
  }
}


const uint16_t IUTriAxisEnergyFeature512::sourceSize[IUTriAxisEnergyFeature512::sourceCount] = {512, 512, 512};

void IUTriAxisEnergyFeature512::sendToReceivers()
{
  if (m_receivers[0])
  {
    m_receivers[0]->receive(m_receiverSourceIndex[0], (q15_t) m_state);
  }
}


const uint16_t IUTriSourceSummingFeature::sourceSize[IUTriSourceSummingFeature::sourceCount] = {1, 1, 1};

void IUTriSourceSummingFeature::sendToReceivers()
{
  if (m_receivers[0])
  {
    m_receivers[0]->receive(m_receiverSourceIndex[0], (q15_t) m_state);
  }
}


const uint16_t IUVelocityFeature512::sourceSize[IUVelocityFeature512::sourceCount] = {512, 1};


const uint16_t IUDefaultFloatFeature::sourceSize[IUDefaultFloatFeature::sourceCount] = {1};


const uint16_t IUAudioDBFeature2048::sourceSize[IUAudioDBFeature2048::sourceCount] = {2048};




