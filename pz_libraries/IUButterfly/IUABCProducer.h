#ifndef IUABCPRODUCER_H
#define IUABCPRODUCER_H

#include "IUABCFeature.h"

/* ========================== Producer Class ============================= */

/**
 * Base class for producers, ie object that will send data to features
 * This class is abstract in principle but abstraction is not enforced.
 * In current implementation, a Producer can have a max of 5 receivers,
 * change static class memeber 'maxReceiverCount' if needed.
 */
class IUABCProducer
{
  public:
    static const uint8_t maxReceiverCount = 5;
    IUABCProducer();
    virtual ~IUABCProducer();
    void resetReceivers();
    virtual bool addReceiver(uint8_t receiverSourceIndex, IUABCFeature *receiver);
    virtual void sendToReceivers() = 0;                  // To implement in child class

  protected:
    uint8_t m_receiverCount;                            // The actual number of receivers
    uint8_t m_receiverSourceIndex[maxReceiverCount];    // The source of each recceiver to which send data
    IUABCFeature *m_receivers[maxReceiverCount];        // Array of pointers to receivers
};


#endif // IUABCPRODUCER_H
