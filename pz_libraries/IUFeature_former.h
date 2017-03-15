#ifndef IUFEATURE_H
#define IUFEATURE_H

#include <Arduino.h>
#include <arm_math.h>
#include <MD_CirQueue.h>
#include "IUUtilities.h"


/* ========================== Abstract Base Classes ============================= */

/**
 * Base class for features
 * This class is abstract in principle but abstraction is not enforced to avoid method 
 * duplication.
 */
class IUABCFeature
{
  public:
    IUABCFeature(uint8_t id, String name);
    virtual ~IUABCFeature();
    void setId(uint8_t id) { m_id = id; }
    uint8_t getId() { return m_id; }
    void setName(String name) { m_name = name; }
    String getName() { return m_name; }
    // Feature computation, source and sending queue
    virtual void setComputeFunction(float (*computeFunction) ()) { return; } // Not implemented
    virtual void setDefaultComputeFunction() { return; }                     // Not implemented
    virtual bool prepareSource(uint16_t sourceSize) { return false; }        // Not implemented
    virtual bool prepareSendingQueue(uint16_t sendingQueueSize);
    virtual bool activate();
    bool isActive() { return m_active; }
    void enableFeatureCheck() { m_checkFeature = true; }
    void disableFeatureCheck() { m_checkFeature = false; }
    bool isFeatureCheckActive() { return m_checkFeature; }
    // Thresholds and state
    void setThresholds(float normal, float warning, float danger);
    float getThreshold(uint8_t index) { return m_thresholds[index]; }
    // Run
    virtual bool receive() { return false; }                                 // Not implemented
    virtual float compute() { return 0; }                                    // Not implemented
    uint8_t getThresholdState();
    float peekNextValue();
    bool stream(Stream *port);

  protected:
    uint8_t m_id;
    String m_name;
    bool m_active;
    bool m_checkFeature;
    float (*m_computeFunction) ();
    uint16_t m_sourceSize;
    q15_t *m_source;
    virtual void newSource() { return; }                                      // Not implemented
    uint16_t m_sourceCounter;
    bool m_computeNow[2];
    bool m_recordNow[2];
    uint8_t m_computeIndex;                                              // Computation buffer index
    uint8_t m_recordIndex;                                               // Data recording buffer index
    // Using a queue for sending, so that we always have the n latest results
    uint8_t m_sendingQueueSize;                                          // MD_Queue size
    MD_CirQueue *m_sendingQueue;
    // Thresholds and state
    float m_thresholds[3];                                               // Normal, warning and danger thresholds
    uint8_t m_state; // possible states are 0: not cutting, 1: normal, 2: warning, 3: danger
    uint8_t m_highestDangerLevel; // The most critical state ever measured
};



/**
 * Base class for producers, ie object that will send data to features
 * This class is abstract in principle but abstraction is not enforced to avoid method 
 * duplication.
 */
class IUABCProducer
{
  public:
    IUABCProducer();
    virtual ~IUABCProducer();
    void resetReceivers();
    virtual bool setReceivers(uint8_t receiverCount, uint8_t *receiverSourceIndex, IUABCFeature *receivers, uint8_t *toSend);
    
  protected:
    uint8_t m_receiverCount;
    uint8_t *m_receiverSourceIndex;
    uint8_t *m_toSend;
    IUABCFeature *m_receivers;
};


/* ========================== General Feature Classes ============================= */

/**
* Feature with a single source, with number in Q15 format
*/
class IUSingleQ15SourceFeature : public IUABCFeature, public IUABCProducer
{
  public:
    IUSingleQ15SourceFeature(uint8_t id, String name);
    virtual ~IUSingleQ15SourceFeature() {}
    // Feature computation, source and sending queue
    virtual void setComputeFunction(float (*computeFunction) (uint16_t, q15_t*));
    virtual void setDefaultComputeFunction();
    virtual bool prepareSource(uint16_t sourceSize);
    // Run
    virtual bool receive(uint8_t sourceIndex, q15_t value);
    virtual float compute();

  protected:
    float (*m_computeFunction) (uint16_t sourceSize, q15_t* source);
    uint16_t m_sourceSize;
    q15_t *m_source;
    virtual void newSource(const uint16_t sourceSize);
    uint16_t m_sourceCounter;
    bool m_computeNow[2];
    bool m_recordNow[2];
    uint8_t m_computeIndex;   // Computation buffer index
    uint8_t m_recordIndex;    // Data recording buffer index
    // Using a queue for sending, so that we always have the n latest results
    uint8_t m_sendingQueueSize; // MD_Queue size
    MD_CirQueue *m_sendingQueue;
      
};


/**
* Feature with a single source, with number in float format
*/
class IUSingleFloatSourceFeature : public IUSingleQ15SourceFeature
{
  public:
    // Feature computation, source and sending queue
    virtual void setComputeFunction(float (*computeFunction) (uint16_t, float*));
    virtual void setDefaultComputeFunction();
    // Run
    virtual bool receive(uint8_t sourceIndex, float value);

  protected:
    float (*m_computeFunction) (uint16_t sourceSize, float* source);
    float *m_source;
    virtual void newSource(uint16_t *sourceSize);
};


/**
* Feature with multiple sources, with number in Q15 format
*/
class IUMultiQ15SourceFeature : public IUABCFeature, public IUABCProducer
{
  public:
    IUMultiQ15SourceFeature(uint8_t id, String name="");
    virtual ~IUMultiQ15SourceFeature();
    // Feature computation, source and sending queue
    virtual void setComputeFunction(float (*computeFunction) (uint8_t, uint16_t*, q15_t*));
    virtual void setDefaultComputeFunction();
    virtual bool prepareSource(uint8_t sourceCount, uint16_t *sourceSize);
    // Run
    virtual bool receive(uint8_t sourceIndex, q15_t value);
    virtual float compute();

  protected:
    float (*m_computeFunction) (uint8_t sourceCount, uint16_t* sourceSize, q15_t* source);
    uint8_t m_sourceCount;
    uint16_t *m_sourceSize;
    q15_t *m_source;
    void newSource(uint8_t sourceCount, uint16_t *sourceSize);
    uint16_t *m_sourceCounter;
    bool *m_computeNow;
    bool *m_recordNow;
    uint8_t *m_computeIndex;   // Computation buffer index
    uint8_t *m_recordIndex;    // Data recording buffer index
    // Using a queue for sending, so that we always have the n latest results
    uint8_t m_sendingQueueSize; // MD_Queue size
    MD_CirQueue *m_sendingQueue;
};


/**
* Feature with multiple sources, with number in float format
*/
class IUMultiFloatSourceFeature : public IUMultiQ15SourceFeature
{
  public:
    // Feature computation, source and sending queue
    virtual void setComputeFunction(float (*computeFunction) (uint8_t, uint16_t*, float*));
    virtual void setDefaultComputeFunction();
    // Run
    virtual bool receive(uint8_t sourceIndex, float value);

  protected:
    float (*m_computeFunction) (uint8_t sourceCount, uint16_t* sourceSize, float* source);
    float *m_source;
    virtual void newSource(uint8_t sourceCount, uint16_t *sourceSize);
};


/* ========================== Speicalized Feature Classes ============================= */

/**
* Feature for data collection, with multiple sources with number in Q15 format
* 
* Data collection feature has no sendingQueue (NULL pointer), the prepareSendingQueue,
* setReceivers and compute feature do nothing. When the stream function is called, the 
* source at bufferComputeIndex is directly streamed, while applying the dataTransform
* function item-wise.
*/
class IUMultiQ15SourceDataCollectionFeature : public IUMultiQ15SourceFeature
{
  public:
    IUMultiQ15SourceDataCollectionFeature(uint8_t id, String name="");
    virtual ~IUMultiQ15SourceDataCollectionFeature();
    virtual bool prepareSendingQueue() { return false; } //Does nothing
    virtual bool activate();
    virtual float compute() { return 0; } //Does nothing
    virtual void setDefaultComputeFunction() { return; } //Does nothing
    void setReceivers() { return; } //Does nothing
    bool stream(Stream *port);
    void setDataTransform(float (*dataTransform) (q15_t)) { m_dataTransform = dataTransform;}
    void setDefaultDataTransform() { m_dataTransform = q15ToFloat;}
    void setSourceNames(String *sourceNames) { m_sourceNames = sourceNames; }

  protected:
    uint8_t m_sendingQueueSize;                                          // MD_Queue size
    MD_CirQueue *m_sendingQueue;
    float (*m_dataTransform) (q15_t item);
    String *m_sourceNames;
};


/* ===================================== Feature Instanciations ======================================== */

extern IUMultiQ15SourceDataCollectionFeature showRecordFFT;
extern IUMultiQ15SourceFeature accelerationEnergy;
extern IUMultiQ15SourceFeature velocityX;
extern IUMultiQ15SourceFeature velocityY;
extern IUMultiQ15SourceFeature velocityZ;
extern IUSingleFloatSourceFeature temperature;
extern IUSingleFloatSourceFeature audioDB;


/* =================================== Feature Collection Class ====================================== */

class IUFeatureSelection
{
    public:
        // Arrays have to be initialized with a fixed size,
        // so we chose max_size = max number of features per collections
        static const uint8_t MAX_SIZE = 10;
        IUFeatureSelection();
        bool addFeature(IUABCFeature feature);
        IUABCFeature getFeature(uint8_t index);
        uint8_t getSize() { return m_size; }

    private:
        IUABCFeature m_features[MAX_SIZE];
        uint8_t m_size; // Dynamic


};



#endif // IUFEATURE_H
