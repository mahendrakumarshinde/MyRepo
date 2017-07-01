#ifndef IUABCPRODUCER_H
#define IUABCPRODUCER_H

#include <Arduino.h>
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
    static const uint8_t maxReceiverCount = 20;
    enum dataSendOption : uint8_t {optionCount = 0};   // Options of data to send to receivers
    IUABCProducer();
    virtual ~IUABCProducer();
    virtual void resetReceivers();
    virtual bool addScalarReceiver(uint8_t sendOption, uint8_t receiverSourceIndex, IUABCFeature *receiver);
    virtual bool addArrayReceiver(uint8_t sendOption, uint16_t valueCount, q15_t *values, uint8_t receiverSourceIndex, IUABCFeature *receiver);
    virtual bool addArrayReceiver(uint8_t sendOption, uint16_t valueCount, float *values, uint8_t receiverSourceIndex, IUABCFeature *receiver);
    virtual void sendToReceivers() {}                   // To implement in child class
    virtual uint8_t getReceiverCount() { return m_receiverCount; }
    // Diagnostic Functions
    virtual void exposeReceivers();

  protected:
    uint8_t m_receiverCount;                            // The actual number of receivers
    uint8_t m_receiverSourceIndex[maxReceiverCount];    // The source of each receiver to which send data
    uint8_t m_toSend[maxReceiverCount];                 // Data to send to each receiver. Sending option should be
                                                        // used in the function 'sendToReceivers'
    IUABCFeature *m_receivers[maxReceiverCount];        // Array of pointers to receivers
};


#endif // IUABCPRODUCER_H
