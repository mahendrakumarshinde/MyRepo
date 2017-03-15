#include "IUFeature.h"

/* ================== IUQ15DataCollectionFeature Definitions ==================== */

const uint16_t IUQ15DataCollectionFeature::sourceSize[IUQ15DataCollectionFeature::sourceCount] = {1};

IUQ15DataCollectionFeature::IUQ15DataCollectionFeature(uint8_t id, String name, String fullName) : IUABCFeature(id, name, fullName), m_streamNow(false)
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

bool IUQ15DataCollectionFeature::compute()
{
  if (m_streamNow)
  {
    return false;
  }
  IUABCFeature::compute();
  m_streamNow = true;
  return true;
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


/* ================== IUFeature Definitions ==================== */

const uint16_t IUFeature::sourceSize[IUFeature::sourceCount] = {1};

IUFeature::IUFeature(uint8_t id, String name, String fullName) : IUABCFeature(id, name, fullName), IUABCProducer()
{
  // construtor
}

void IUFeature::sendToReceivers()
{
  for (uint8_t i = 0; i < m_receiverCount; i++)
  {
    if (m_receivers[i])
    {
      switch(m_toSend[i])
      {
        case dataSendOption::value:
          m_receivers[i]->receive(m_receiverSourceIndex[i], m_latestValue);
          break;
        case dataSendOption::state:
          m_receivers[i]->receive(m_receiverSourceIndex[i], (q15_t) m_state);
          break;
      }
    }
  }
}


/* ================== IUQ15Feature Definitions ==================== */

IUQ15Feature::IUQ15Feature(uint8_t id, String name, String fullName) : IUFeature(id, name, fullName)
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


/* ================== IUFloatFeature Definitions ==================== */

IUFloatFeature::IUFloatFeature(uint8_t id, String name, String fullName) : IUFeature(id, name, fullName)
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

/* ========================== Speicalized Feature Classes ============================= */

const uint16_t IUSingleAxisEnergyFeature128::sourceSize[IUSingleAxisEnergyFeature128::sourceCount] = {128};

IUSingleAxisEnergyFeature128::IUSingleAxisEnergyFeature128(uint8_t id, String name, String fullName) : IUQ15Feature(id, name, fullName)
{
  //ctor
}

const uint16_t IUSingleAxisEnergyFeature512::sourceSize[IUSingleAxisEnergyFeature512::sourceCount] = {512};

IUSingleAxisEnergyFeature512::IUSingleAxisEnergyFeature512(uint8_t id, String name, String fullName) : IUQ15Feature(id, name, fullName)
{
  //ctor
}

const uint16_t IUTriAxisEnergyFeature128::sourceSize[IUTriAxisEnergyFeature128::sourceCount] = {128, 128, 128};

IUTriAxisEnergyFeature128::IUTriAxisEnergyFeature128(uint8_t id, String name, String fullName) : IUQ15Feature(id, name, fullName)
{
  //ctor
}

const uint16_t IUTriAxisEnergyFeature512::sourceSize[IUTriAxisEnergyFeature512::sourceCount] = {512, 512, 512};

IUTriAxisEnergyFeature512::IUTriAxisEnergyFeature512(uint8_t id, String name, String fullName) : IUQ15Feature(id, name, fullName)
{
  //ctor
}

const uint16_t IUTriSourceSummingFeature::sourceSize[IUTriSourceSummingFeature::sourceCount] = {1, 1, 1};

IUTriSourceSummingFeature::IUTriSourceSummingFeature(uint8_t id, String name, String fullName) : IUFloatFeature(id, name, fullName)
{
  //ctor
}

const uint16_t IUVelocityFeature512::sourceSize[IUVelocityFeature512::sourceCount] = {512, 1};

IUVelocityFeature512::IUVelocityFeature512(uint8_t id, String name, String fullName) : IUQ15Feature(id, name, fullName)
{
  //ctor
}

const uint16_t IUDefaultFloatFeature::sourceSize[IUDefaultFloatFeature::sourceCount] = {1};

IUDefaultFloatFeature::IUDefaultFloatFeature(uint8_t id, String name, String fullName) : IUFloatFeature(id, name, fullName)
{
  //ctor
}

const uint16_t IUAudioDBFeature2048::sourceSize[IUAudioDBFeature2048::sourceCount] = {2048};

IUAudioDBFeature2048::IUAudioDBFeature2048(uint8_t id, String name, String fullName) : IUQ15Feature(id, name, fullName)
{
  //ctor
}

const uint16_t IUAudioDBFeature4096::sourceSize[IUAudioDBFeature2048::sourceCount] = {4096};

IUAudioDBFeature4096::IUAudioDBFeature4096(uint8_t id, String name, String fullName) : IUQ15Feature(id, name, fullName)
{
  //ctor
}



