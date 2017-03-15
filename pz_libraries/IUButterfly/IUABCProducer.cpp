#include "IUABCProducer.h"

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
  for (int i = 0; i < maxReceiverCount; i++)
  {
    delete m_receivers[i]; m_receivers[i] = NULL;
    m_receiverSourceIndex[i] = 0;
  }
}

/**
 * Add a receiver to the producer
 * @return true if the receiver was added, else false
 */
bool IUABCProducer::addReceiver(uint8_t receiverSourceIndex, IUABCFeature *receiver)
{
  if (m_receiverCount >= maxReceiverCount)
  {
    // Producer's receiver list is full
    return false;
  }
  m_receivers[m_receiverCount] = receiver;
  if (!m_receivers[m_receiverCount])
  {
    // Don't let pointer dangling if failed
    delete m_receivers[m_receiverCount]; m_receivers[m_receiverCount] = NULL;
    return false;
  }
  m_receiverSourceIndex[m_receiverCount] = receiverSourceIndex;
  m_receiverCount++;
  return true;
}

