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
 * NB: This method should be virtual since it's used in constructor and destructor.
 */
void IUABCProducer::resetReceivers()
{
  m_receiverCount = 0;
  for (int i = 0; i < maxReceiverCount; i++)
  {
    if (m_receivers[i])
    {
      delete m_receivers[i]; m_receivers[i] = NULL;
    }
    m_receiverSourceIndex[i] = 0;
  }
}

/**
 * Add a receiver to the producer
 * @return true if the receiver was added, else false
 */
bool IUABCProducer::addReceiver(uint8_t sendOption, uint8_t receiverSourceIndex, IUABCFeature *receiver)
{
  if (m_receiverCount >= maxReceiverCount)
  {
    if(debugMode) { debugPrint("Producer's receiver list is full"); }
    return false;
  }
  m_receivers[m_receiverCount] = receiver;
  if (!m_receivers[m_receiverCount])
  {
    if(debugMode) { debugPrint("Pointer assignment failed"); }
    m_receivers[m_receiverCount] = NULL;
    return false;
  }
  m_receiverSourceIndex[m_receiverCount] = receiverSourceIndex;
  m_toSend[m_receiverCount] = (dataSendOption) sendOption;
  m_receiverCount++;
  return true;
}

