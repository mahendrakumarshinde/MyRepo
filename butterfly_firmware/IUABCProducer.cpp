#include "IUABCProducer.h"

/* ======================== IUABCProducer Definitions ========================== */

IUABCProducer::IUABCProducer() :
  m_receiverCount(0)
{
  for (int i = 0; i < maxReceiverCount; i++)
  {
    m_receivers[i] = NULL;
  }
}

IUABCProducer::~IUABCProducer()
{
  resetReceivers();
}

/**
 * Reset pointers to receivers to NULL
 * 
 * DO NOT DELETE the receivers, since they are not the producer responsability
 * NB: This method should be virtual since it's used in constructor and destructor.
 */
void IUABCProducer::resetReceivers()
{
  m_receiverCount = 0;
  for (int i = 0; i < maxReceiverCount; i++)
  {
    if (m_receivers[i])
    {
      Serial.print("resetting "); Serial.println(m_receivers[i]->getName());
    }
    m_receivers[i] = NULL;
    m_receiverSourceIndex[i] = 0;
  }
}

/**
 * Add a receiver to the producer
 * @return true if the receiver was added, else false
 */
bool IUABCProducer::addScalarReceiver(uint8_t sendOption, uint8_t receiverSourceIndex, IUABCFeature *receiver)
{
  if (m_receiverCount >= maxReceiverCount)
  {
    if(setupDebugMode) { debugPrint("Producer's receiver list is full"); }
    return false;
  }
  m_receivers[m_receiverCount] = receiver;
  if (!m_receivers[m_receiverCount])
  {
    if(setupDebugMode) { debugPrint("Pointer assignment for scalar source failed"); }
    m_receivers[m_receiverCount] = NULL;
    return false;
  }
  m_receiverSourceIndex[m_receiverCount] = receiverSourceIndex;
  m_toSend[m_receiverCount] = sendOption;
  m_receiverCount++;
  return true;
}

/**
 * Add a receiver to the producer
 * @return true if the receiver was added, else false
 */
bool IUABCProducer::addArrayReceiver(uint8_t sendOption, uint16_t valueCount, q15_t *values, uint8_t receiverSourceIndex, IUABCFeature *receiver)
{
  if (m_receiverCount >= maxReceiverCount)
  {
    if(setupDebugMode) { debugPrint("Producer's receiver list is full"); }
    return false;
  }
  m_receivers[m_receiverCount] = receiver;
  if (!m_receivers[m_receiverCount])
  {
    if(setupDebugMode) { debugPrint("Pointer assignment for array source failed"); }
    m_receivers[m_receiverCount] = NULL;
    return false;
  }
  m_receiverSourceIndex[m_receiverCount] = receiverSourceIndex;
  m_toSend[m_receiverCount] = sendOption;
  m_receiverCount++;
  bool success = receiver->setSource(receiverSourceIndex, valueCount, values);
  if (!success)
  {
    if(setupDebugMode) { debugPrint("Array source setting failed"); }
  }
  return success;
}

/**
 * Add a receiver to the producer
 * @return true if the receiver was added, else false
 */
bool IUABCProducer::addArrayReceiver(uint8_t sendOption, uint16_t valueCount, float *values, uint8_t receiverSourceIndex, IUABCFeature *receiver)
{
  if (m_receiverCount >= maxReceiverCount)
  {
    if(setupDebugMode) { debugPrint("Producer's receiver list is full"); }
    return false;
  }
  m_receivers[m_receiverCount] = receiver;
  bool success = receiver->setSource(receiverSourceIndex, valueCount, values);
  if (!m_receivers[m_receiverCount] || success)
  {
    if(setupDebugMode) { debugPrint("Pointer assignment failed"); }
    m_receivers[m_receiverCount] = NULL;
    return false;
  }
  m_receiverSourceIndex[m_receiverCount] = receiverSourceIndex;
  m_toSend[m_receiverCount] = sendOption;
  m_receiverCount++;
  return true;
}

/* ====================== Diagnostic Functions, only active when setupDebugMode = true ====================== */

/**
 * Shows the name of features receiving Producer data and associated config
 */
void IUABCProducer::exposeReceivers()
{
  #ifdef DEBUGMODE
  if (m_receiverCount == 0)
  {
    debugPrint("No receiver");
    return;
  }
  debugPrint("Receivers count: ", false);
  debugPrint(m_receiverCount);
  for (int i = 0; i < m_receiverCount; i++)
  {
    debugPrint(m_receivers[i]->getName(), false);
    debugPrint(F(", send option is "), false);
    debugPrint(m_toSend[i]);
  }
  #endif
}

