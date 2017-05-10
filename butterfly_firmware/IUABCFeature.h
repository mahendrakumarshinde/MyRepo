
/*

Feature abstract base class

*/

#ifndef IUABCFEATURE_H
#define IUABCFEATURE_H

#include <Arduino.h>
#include <arm_math.h>
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
    static const uint8_t ABCSourceCount = 5;
    static const uint16_t ABCSourceSize[ABCSourceCount];
    //Constructors, destructors, setters and getters
    IUABCFeature(uint8_t id, char *name);
    virtual ~IUABCFeature();
    virtual void setId(uint8_t id) { m_id = id; }
    virtual uint8_t getId() { return m_id; }
    virtual void setName(char *name) { strcpy(m_name, name); }
    virtual String getName() { return (String) m_name; }
    virtual uint8_t getSourceCount() { return ABCSourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return ABCSourceSize[index]; }
    virtual uint16_t const* getSourceSize() { return ABCSourceSize; }

    // Feature computation, source and sending queue
    virtual void resetSource(bool deletePtr = false);
    virtual bool prepareSource();
    virtual void getSource(uint8_t idx, q15_t **values) { }
    virtual void getSource(uint8_t idx, float **values) { }
    virtual bool setSource(uint8_t sourceIndex, uint16_t valueCount, q15_t *values) {}             // To implement in child class
    virtual bool setSource(uint8_t sourceIndex, uint16_t valueCount, float *values) {}             // To implement in child class
    virtual bool prepareProducer() { return false; }
    virtual bool activate();
    virtual void deactivate() { m_active = false; }
    virtual void resetCounters();
    virtual bool isActive() { return m_active; }
    virtual bool getFeatureCheck() { return m_checkFeature; }
    virtual void setFeatureCheck( bool checkFeature) { m_checkFeature = checkFeature; }
    virtual bool isFeatureCheckActive() { return m_checkFeature; }
    virtual void setStreaming( bool enableStreaming) { m_isStreamed = enableStreaming; }
    virtual bool isStreamed() { return m_isStreamed; }
    // Results
    virtual float getLatestValue() = 0;                                            // To implement in child class
    virtual uint16_t getDestinationSize() = 0;                                     // To implement in child class
    virtual q15_t* getDestination() = 0;                                           // To implement in child class

    // Thresholds and state
    virtual void setThresholds(float normalVal, float warningVal, float dangerVal);
    virtual float getThreshold(uint8_t index) { return m_thresholds[index]; }
    virtual operationState updateState();
    virtual operationState getState() = 0;                                         // To implement in child class
    virtual void setState(operationState state) = 0;                               // To implement in child class
    virtual operationState getHighestDangerLevel() = 0;                            // To implement in child class
    virtual void setHighestDangerLevel(operationState state) = 0;                  // To implement in child class
    // Run
    virtual bool receiveScalar(uint8_t sourceIndex, q15_t value) { return false; } // To implement in child class
    virtual bool receiveScalar(uint8_t sourceIndex, float value) { return false; } // To implement in child class
    virtual bool receiveArray(uint8_t sourceIndex);
    virtual void record(uint8_t idx, uint8_t sourceIndex, uint16_t recordIdx, q15_t value) { }
    virtual bool isTimeToEndRecord();
    virtual bool compute();
    virtual void stream(HardwareSerial *port);
    virtual void streamSourceData(HardwareSerial *port) {}

    // Diagnostic Functions
    virtual void exposeSourceConfig();
    virtual void exposeCounterState();

  protected:
    uint8_t m_id;
    char m_name[3];
    bool m_active;
    bool m_checkFeature;
    bool m_isStreamed;
    virtual void m_computeScalar (uint8_t computeIndex) { return; }   // Implement in children classes if needed
    virtual void m_computeArray (uint8_t computeIndex) { return; }    // Implement in children classes if needed
    virtual bool newSource() { return false; }                        // To implement in child class
    q15_t *m_source[2][ABCSourceCount];
    bool m_sourceReady;
    bool m_producerReady;
    //TODO see below
    //FIXME computeNow and recordNow should be of shape (2, ABCSourceCount) but for some reason memory is not properly allocated for inheriting classes
    // should be working (cf http://stackoverflow.com/questions/14447192/override-array-size-in-subclass) but it is not
    uint16_t m_sourceCounter[ABCSourceCount];
    bool m_computeNow[2][ABCSourceCount];
    bool m_recordNow[2][ABCSourceCount];
    uint8_t m_computeIndex;                                            // Computation buffer index
    uint8_t m_recordIndex;                                             // Data recording buffer index
    // Thresholds and state
    float m_thresholds[operationState::opStateCount - 1];              // Normal, warning and danger thresholds
};


#endif // IUABCFEATURE_H
