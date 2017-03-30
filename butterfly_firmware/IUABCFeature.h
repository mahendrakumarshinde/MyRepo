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
    static const uint8_t ABCSourceCount = 1;
    static const uint16_t ABCSourceSize[ABCSourceCount];
    static const uint16_t ABCDestinationSize = 1;
    //Constructors, destructors, setters and getters
    IUABCFeature(uint8_t id, char *name);
    virtual ~IUABCFeature();
    virtual void setId(uint8_t id) { m_id = id; }
    virtual uint8_t getId() { return m_id; }
    virtual void setName(char *name) { strcpy(m_name, name); }
    virtual String getName() { return (String) m_name; }
    virtual uint8_t getSourceCount() { return ABCSourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return ABCSourceSize[index]; }
    virtual uint16_t getDestinationSize() { return ABCDestinationSize; }
    virtual q15_t* getDestination() { return m_destination; }
    virtual uint16_t const* getSourceSize() { return ABCSourceSize; }
    virtual float getLatestValue() { return m_latestValue; }

    // Feature computation, source and sending queue
    virtual void setcomputeScalarFunction(float (*computeScalarFunction) (uint8_t sourceCount, const uint16_t *sourceSize, q15_t **source))
                            { m_computeScalarFunction = computeScalarFunction; }
    virtual void setcomputeArrayFunction(bool (*computeArrayFunction) (uint8_t sourceCount, const uint16_t *sourceSize, q15_t **source,
                                                                        const uint16_t destinationSize, q15_t *destination))
                            { m_computeArrayFunction = computeArrayFunction; }
    virtual void resetSource();
    virtual bool prepareSource();
    virtual bool setSource(uint8_t sourceIndex, uint16_t valueCount, q15_t *values) {}             // To implement in child class
    virtual bool setSource(uint8_t sourceIndex, uint16_t valueCount, float *values) {}             // To implement in child class
    virtual void deleteSource();
    virtual bool activate();
    virtual void deactivate() { m_active = false; }
    virtual void resetCounters();
    virtual bool isActive() { return m_active; }
    virtual void setFeatureCheck( bool checkFeature) { m_checkFeature = checkFeature; }
    virtual bool isFeatureCheckActive() { return m_checkFeature; }

    // Thresholds and state
    virtual void setThresholds(float normalVal, float warningVal, float dangerVal);
    virtual float getThreshold(uint8_t index) { return m_thresholds[index]; }
    // Run
    virtual bool receiveScalar(uint8_t sourceIndex, q15_t value) { return false; }          // To implement in child class
    virtual bool receiveArray(uint8_t sourceIndex);
    virtual bool isTimeToEndRecord();
    virtual bool compute();
    virtual operationState getOperationState();
    virtual void stream(HardwareSerial *port);

    // Diagnostic Functions
    virtual void exposeSourceConfig();
    virtual void exposeCounterState();

  protected:
    uint8_t m_id;
    char m_name[3];
    bool m_active;
    bool m_checkFeature;
    float m_latestValue;
    float (*m_computeScalarFunction) (uint8_t sourceCount, const uint16_t *sourceSize, q15_t **source);
    bool (*m_computeArrayFunction) (uint8_t sourceCount, const uint16_t *sourceSize, q15_t **source,
                                    const uint16_t destinationSize, q15_t *destination);
    virtual bool newSource() { return false; }                        // To implement in child class
    q15_t *m_source[2][ABCSourceCount];
    q15_t m_destination[ABCDestinationSize];
    bool m_sourceReady;
    uint16_t m_sourceCounter[ABCSourceCount];
    bool m_computeNow[2][ABCSourceCount];
    bool m_recordNow[2][ABCSourceCount];
    uint8_t m_computeIndex;                                            // Computation buffer index
    uint8_t m_recordIndex;                                             // Data recording buffer index
    // Thresholds and state
    float m_thresholds[operationState::opStateCount - 1];              // Normal, warning and danger thresholds
    operationState m_state;                                            // Operation state
    operationState m_highestDangerLevel;                               // The most critical state ever measured
};


#endif // IUABCFEATURE_H
