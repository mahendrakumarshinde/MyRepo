/*

Feature abstract base class

*/

#ifndef IUABCFEATURE_H
#define IUABCFEATURE_H

#include <Arduino.h>
#include <arm_math.h>
#include "QList.h"
#include "QList.cpp"
#include "IUUtilities.h"

/**
 * Base class for features
 * This class is abstract in principle but abstraction is not enforced to avoid method
 * duplication.
 * Can be subclassed to have different number of source, with different sizes.
 * Also have a sending queue: setting the queue size to N allows to keep a buffer of features
 * values, to be sent later.
 */
class IUABCFeature
{
  public:
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
    //Constructors, destructors, setters and getters
    IUABCFeature(uint8_t id, String name = "", String fullName = "");
    virtual ~IUABCFeature();
    void setId(uint8_t id) { m_id = id; }
    uint8_t getId() { return m_id; }
    void setName(String name) { m_name = name; }
    String getName() { return m_name; }
    void setFullName(String fullName) { m_fullName = fullName; }
    String getFullName() { return m_fullName; }
    void setSendingQueueMaxSize(uint8_t sendingQueueMaxSize) { m_sendingQueueMaxSize = sendingQueueMaxSize; }
    uint8_t getSendingQueueMaxSize() { return m_sendingQueueMaxSize; }
    // Feature computation, source and sending queue
    virtual void setComputeFunction(float (*computeFunction) (uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[]))
                            { m_computeFunction = computeFunction; }
    virtual void setDefaultComputeFunction() { return; }                    // To implement in child class
    virtual bool activate();
    virtual void deactivate() { m_active = false; }
    bool prepareSource();
    void resetCounters();
    bool isActive() { return m_active; }
    void setFeatureCheck( bool checkFeature) { m_checkFeature = checkFeature; }
    bool isFeatureCheckActive() { return m_checkFeature; }
    // Thresholds and state
    void setThresholds(float normalVal, float warningVal, float dangerVal);
    float getThreshold(uint8_t index) { return m_thresholds[index]; }
    // Run
    virtual bool receive(uint8_t sourceIndex, q15_t value) { return false; } // To implement in child class
    virtual bool receive(uint8_t sourceIndex, float value) { return false; } // To implement in child class
    bool isTimeToEndRecord();
    virtual bool compute();
    operationState getOperationState();
    bool stream(HardwareSerial *port);

  protected:
    uint8_t m_id;
    String m_name;
    String m_fullName;
    bool m_active;
    bool m_checkFeature;
    float m_latestValue;
    float (*m_computeFunction) (uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[]);
    virtual bool newSource() { return false; }                         // To implement in child class
    void resetSource();
    q15_t *m_source[2][sourceCount];
    bool m_sourceReady;
    uint16_t m_sourceCounter[sourceCount];
    bool m_computeNow[2][sourceCount];
    bool m_recordNow[2][sourceCount];
    uint8_t m_computeIndex;                                            // Computation buffer index
    uint8_t m_recordIndex;                                             // Data recording buffer index
    uint8_t m_sendingQueueMaxSize;                                        // MD_Queue size
    QList<float> m_sendingQueue;    
    // Thresholds and state
    float m_thresholds[operationState::opStateCount - 1];                // Normal, warning and danger thresholds
    operationState m_state;                                            // Operation state
    operationState m_highestDangerLevel;                               // The most critical state ever measured
};


#endif // IUABCFEATURE_H
