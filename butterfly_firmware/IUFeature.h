/*
  Infinite Uptime Firmware

*/


#ifndef IUFEATURE_H
#define IUFEATURE_H

#include <Arduino.h>

#include "IUABCFeature.h"
#include "IUABCProducer.h"


/* ===================== Feature calculation functions ===================== */

// Default compute functions
inline float computeDefaultQ15(uint8_t sourceCount, const uint16_t *sourceSize, q15_t **source) { return q15ToFloat(source[0][0]); }
inline float computeDefaultFloat(uint8_t sourceCount, const uint16_t *sourceSize, float **source) { return (source[0][0]); }

// Scalar feature functions
float computeSignalEnergy(uint8_t sourceCount, const uint16_t *sourceSize, q15_t **source);

float computeSumOf(uint8_t sourceCount, const uint16_t* sourceSize, float **source);

float computeRFFTMaxIndex(uint8_t sourceCount, const uint16_t* sourceSize, q15_t **source);

float computeVelocity(uint8_t sourceCount, const uint16_t *sourceSize, q15_t **source);

float computeAcousticDB(uint8_t sourceCount, const uint16_t* sourceSize, q15_t **source);

// Array feature functions

bool computeRFFT(uint8_t sourceCount, const uint16_t* sourceSize, q15_t **source, const uint16_t destinationSize, q15_t *destination);

bool computeAudioRFFT(uint8_t sourceCount, const uint16_t* sourceSize, q15_t **source, const uint16_t destinationSize, q15_t *destination);


/* ========================== Feature classes ============================= */

class IUFeature; // Forward declaration

/*
 *
 */
class IUFeatureProducer: public IUABCProducer
{
  public:
    static const uint8_t maxReceiverCount = 5;
    static const uint16_t destinationSize = 1;
    enum dataSendOption : uint8_t {value = 0,
                                   state = 1,
                                   samplingRate = 2,
                                   sampleCount = 3,
                                   scalarOptionCount = 4,
                                   valueArray = 99};
    IUFeatureProducer();
    virtual ~IUFeatureProducer() {};
    virtual uint16_t getDestinationSize() { return destinationSize; }
    virtual q15_t* getDestination() { return m_destination; }
    virtual bool prepareDestination();
    virtual void setSamplingRate(uint16_t samplingRate) { m_samplingRate = samplingRate; }
    virtual uint16_t getSamplingRate() { return m_samplingRate; }
    virtual void setSampleCount(uint16_t sampleCount) { m_sampleCount = sampleCount; }
    virtual uint16_t getSampleCount() { return m_sampleCount; }
    virtual void setLatestValue(float latestValue) { m_latestValue = latestValue; }
    virtual float getLatestValue() { return m_latestValue; }
    virtual void setState(operationState state) { m_state = state; }
    virtual operationState getState() { return m_state; }
    virtual void setHighestDangerLevel(operationState state) { m_highestDangerLevel = state; }
    virtual operationState getHighestDangerLevel() { return m_highestDangerLevel; }
    // Feature computation, source and sending queue
    virtual void sendToReceivers();
    virtual bool addArrayReceiver(uint8_t receiverSourceIndex, IUABCFeature *receiver);
    virtual bool addReceiver(uint8_t sendOption, uint8_t receiverSourceIndex, IUABCFeature *receiver);

  protected:
    float m_latestValue;
    q15_t *m_destination;
    uint16_t m_samplingRate;
    uint16_t m_sampleCount;
    operationState m_state;                  // Operation state
    operationState m_highestDangerLevel;     // The most critical state ever measured
};


class IUFeatureProducer128: public IUFeatureProducer
{
  public:
    static const uint16_t destinationSize = 128;
    IUFeatureProducer128();
    virtual ~IUFeatureProducer128() {};
    virtual uint16_t getDestinationSize() { return destinationSize; }
    
};


class IUFeatureProducer512: public IUFeatureProducer
{
  public:
    static const uint16_t destinationSize = 512;
    IUFeatureProducer512();
    virtual ~IUFeatureProducer512() {};
    virtual uint16_t getDestinationSize() { return destinationSize; }
};

/* ========================== Feature classes ============================= */

/**
 * Mixing class for IUABCFeature and IUABCProducer
 */
class IUFeature : public IUABCFeature
{
  public:
    IUFeature(uint8_t id, char *name);
    virtual ~IUFeature();
    virtual float getLatestValue() { return getProducer()->getLatestValue(); }
    virtual uint16_t getDestinationSize() { return getProducer()->getDestinationSize(); }
    virtual q15_t* getDestination() { return getProducer()->getDestination(); }
    virtual operationState getState() { return getProducer()->getState(); }
    virtual void setState(operationState state) { getProducer()->setState(state); }
    virtual operationState getHighestDangerLevel() { return getProducer()->getHighestDangerLevel(); }
    virtual void setHighestDangerLevel(operationState state) { getProducer()->setHighestDangerLevel(state); }
    
    // producer
    virtual IUFeatureProducer* getProducer() { return m_producer; }
    virtual void setProducer(IUFeatureProducer *producer) { m_producer = producer; }
    virtual bool prepareProducer();

   protected:
    IUFeatureProducer *m_producer;
};


/**
* Feature with possibly multiple sources of which number format is Q15
*/
class IUQ15Feature : public IUFeature
{
  public:
    IUQ15Feature(uint8_t id, char *name);
    virtual ~IUQ15Feature() {}
    // Feature computation, source and sending queue
    void resetSource(bool deletePtr = false);
    virtual bool setSource(uint8_t sourceIndex, uint16_t valueCount, q15_t *values);
    virtual void getSource(uint8_t idx, q15_t **values) { values = m_source[idx]; }
    virtual bool receiveScalar(uint8_t sourceIndex, q15_t value);
    virtual void record(uint8_t idx, uint8_t sourceIndex, uint16_t recordIdx, q15_t value) { m_source[idx][sourceIndex][recordIdx] = value; }

  protected:
    q15_t *m_source[2][ABCSourceCount];
    virtual bool newSource();
};


/**
* Feature with possibly multiple sources of which number format is float
*/
class IUFloatFeature : public IUFeature
{
  public:
    IUFloatFeature(uint8_t id, char *name);
    virtual ~IUFloatFeature() {}
    // Feature computation, source and sending queue
    void resetSource(bool deletePtr = false);
    virtual bool setSource(uint8_t sourceIndex, uint16_t valueCount, float *values);
    virtual void getSource(uint8_t idx, float **values) { values = m_source[idx]; }
    virtual bool receiveScalar(uint8_t sourceIndex, float value);
    virtual void record(uint8_t idx, uint8_t sourceIndex, uint16_t recordIdx, float value) { m_source[idx][sourceIndex][recordIdx] = value; }

  protected:
    float *m_source[2][ABCSourceCount];
    virtual bool newSource();
};


/* ========================== Specialized Feature Classes ============================= */

/**
 * Signal Energy along a single axis, over 128 values + FFT
 * Excepted producers:
 * 1. Sensor data (Accelerometer on 1 axis)
 * 2. Sampling Rate (from accelerometer)
 * Optionnal expected receivers:
 * 1. operationState in q15_t format
 */
class IUAccelPreComputationFeature128: public IUQ15Feature
{
  public:
    static const uint8_t sourceCount = 2;
    static const uint16_t sourceSize[sourceCount];

    IUAccelPreComputationFeature128(uint8_t id, char *name);
    virtual ~IUAccelPreComputationFeature128() {}
    virtual uint8_t getSourceCount() { return sourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    virtual uint16_t const* getSourceSize() { return sourceSize; }

    // Specific producer
    virtual bool prepareProducer();
    virtual IUFeatureProducer* getProducer() { return m_producer; }
    virtual void setProducer(IUFeatureProducer *producer) { m_producer = producer; }

  protected:
    //IUFeatureProducer *m_producer;
    // Compute functions
    virtual void m_computeScalar (uint8_t computeIndex);
    virtual void m_computeArray (uint8_t computeIndex);    
};


/**
 * Signal Energy along a single axis, over 128 values + FFT
 * Excepted producers:
 * 1. Sensor data (Accelerometer on 1 axis)
 * 2. Sampling Rate (from accelerometer)
 * Optionnal expected receivers:
 * 1. operationState in q15_t format
 */
class IUAccelPreComputationFeature512: public IUQ15Feature
{
  public:
    static const uint8_t sourceCount = 2;
    static const uint16_t sourceSize[sourceCount];

    IUAccelPreComputationFeature512(uint8_t id, char *name);
    virtual ~IUAccelPreComputationFeature512() {}
    virtual uint8_t getSourceCount() { return sourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    virtual uint16_t const* getSourceSize() { return sourceSize; }

    // Specific producer
    virtual bool prepareProducer();
    virtual IUFeatureProducer* getProducer() { return m_producer; }
    virtual void setProducer(IUFeatureProducer *producer) { m_producer = producer; }

  protected:
    //IUFeatureProducer *m_producer;
    // Compute functions
    virtual void m_computeScalar (uint8_t computeIndex);
    virtual void m_computeArray (uint8_t computeIndex);
};


/**
 * Signal Energy along a single axis, over 128 values
 * Excepted producers:
 * 1. Sensor data (eg: Accelerometer on 1 axis)
 * Optionnal expected receivers:
 * 1. operationState in q15_t format
 */
class IUSingleAxisEnergyFeature128: public IUQ15Feature
{
  public:
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
    virtual uint8_t getSourceCount() { return sourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    virtual uint16_t const* getSourceSize() { return sourceSize; }

    IUSingleAxisEnergyFeature128(uint8_t id, char *name);
    virtual ~IUSingleAxisEnergyFeature128() {}

  protected:
    // Compute functions
    virtual void m_computeScalar(uint8_t computeIndex);
};


/**
 * Signal Energy along 3 axis, over 128 values
 * Excepted producers:
 * 1. Sensor (eg: Accelerometer on X axis)
 * 2. Sensor (eg: Accelerometer on Y axis)
 * 3. Sensor (eg: Accelerometer on Z axis)
 * Optionnal expected receivers:
 * 1. operationState in q15_t format
 */
class IUTriAxisEnergyFeature128: public IUQ15Feature
{
  public:
    static const uint8_t sourceCount = 3;
    static const uint16_t sourceSize[sourceCount];
    virtual uint8_t getSourceCount() { return sourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    virtual uint16_t const* getSourceSize() { return sourceSize; }

    IUTriAxisEnergyFeature128(uint8_t id, char *name);
    virtual ~IUTriAxisEnergyFeature128() {}

  protected:
    // Compute functions
    virtual void m_computeScalar (uint8_t computeIndex);
};


/**
 * Signal Energy along a single axis, over 512 values
 * Excepted producers:
 * 1. Sensor data (eg: Accelerometer on 1 axis)
 * Optionnal expected receivers:
 * 1. operationState in q15_t format
 */
class IUSingleAxisEnergyFeature512: public IUQ15Feature
{
  public:
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
    virtual uint8_t getSourceCount() { return sourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    virtual uint16_t const* getSourceSize() { return sourceSize; }

    IUSingleAxisEnergyFeature512(uint8_t id, char *name);
    virtual ~IUSingleAxisEnergyFeature512() {}

  protected:
    // Compute functions
    virtual void m_computeScalar (uint8_t computeIndex);
};


/**
 * Signal Energy along 3 axis, over 512 values
 * Excepted producers:
 * 1. Sensor (eg: Accelerometer on X axis)
 * 2. Sensor (eg: Accelerometer on Y axis)
 * 3. Sensor (eg: Accelerometer on Z axis)
 * Optionnal expected receivers:
 * 1. operationState in q15_t format
 */
class IUTriAxisEnergyFeature512: public IUQ15Feature
{
  public:
    static const uint8_t sourceCount = 3;
    static const uint16_t sourceSize[sourceCount];
    virtual uint8_t getSourceCount() { return sourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    virtual uint16_t const* getSourceSize() { return sourceSize; }

    IUTriAxisEnergyFeature512(uint8_t id, char *name);
    virtual ~IUTriAxisEnergyFeature512() {}

  protected:
    // Compute functions
    virtual void m_computeScalar (uint8_t computeIndex);
};


/**
 * Summing feature (can be use to sum single axis energy on 3 axis)
 * Excepted producers:
 * 1. Feature data (eg: Acceleration Energy on X axis)
 * 2. Feature data (eg: Acceleration Energy on Y axis)
 * 3. Feature data (eg: Acceleration Energy on Z axis)
 * Optionnal expected receivers:
 * 1. operationState in q15_t format
 */
class IUTriSourceSummingFeature: public IUFloatFeature
{
  public:
    static const uint8_t sourceCount = 3;
    static const uint16_t sourceSize[sourceCount];
    virtual uint8_t getSourceCount() { return sourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    virtual uint16_t const* getSourceSize() { return sourceSize; }

    IUTriSourceSummingFeature(uint8_t id, char *name);
    virtual ~IUTriSourceSummingFeature() {}

  protected:
    // Compute functions
    virtual void m_computeScalar (uint8_t computeIndex);
};


/**
 * Summing feature (can be use to sum single axis energy on 3 axis)
 * Excepted producers:
 * 1. Sensor data (eg: Acceleration Energy on 1 axis)
 * 2. Feature state (eg: Acceleration Energy feature state)
 * Optionnal expected receivers: None
 */
class IUVelocityFeature512: public IUQ15Feature
{
  public:
    static const uint8_t sourceCount = 4;
    static const uint16_t sourceSize[sourceCount];
    virtual uint8_t getSourceCount() { return sourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    virtual uint16_t const* getSourceSize() { return sourceSize; }

    IUVelocityFeature512(uint8_t id, char *name);
    virtual ~IUVelocityFeature512() {}

  protected:
    virtual bool newSource();
    // Compute functions
    virtual void m_computeScalar (uint8_t computeIndex);
};


/**
 * Default feature, just take a float value and pass it along
 * Excepted producers:
 * 1. Sensor data (eg: temperature)
 * Optionnal expected receivers: None
 */
class IUDefaultFloatFeature: public IUFloatFeature
{
  public:
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
    virtual uint8_t getSourceCount() { return sourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    virtual uint16_t const* getSourceSize() { return sourceSize; }

    IUDefaultFloatFeature(uint8_t id, char *name);
    virtual ~IUDefaultFloatFeature() {}

  protected:
    // Compute functions
    virtual void m_computeScalar (uint8_t computeIndex);
};


/**
 * Feature that compute audio volume in DB
 * Excepted producers:
 * 1. Sensor data (audio)
 * Optionnal expected receivers: None
 */
class IUAudioDBFeature2048: public IUQ15Feature
{
  public:
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
    virtual uint8_t getSourceCount() { return sourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    virtual uint16_t const* getSourceSize() { return sourceSize; }

    IUAudioDBFeature2048(uint8_t id, char *name);
    virtual ~IUAudioDBFeature2048() {}

  protected:
    virtual bool newSource();
    // Compute functions
    virtual void m_computeScalar (uint8_t computeIndex);
};


/**
 * Feature that compute audio volume in DB
 * Excepted producers:
 * 1. Sensor data (audio)
 * Optionnal expected receivers: None
 */
class IUAudioDBFeature4096: public IUQ15Feature
{
  public:
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
    virtual uint8_t getSourceCount() { return sourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    virtual uint16_t const* getSourceSize() { return sourceSize; }

    IUAudioDBFeature4096(uint8_t id, char *name);
    virtual ~IUAudioDBFeature4096() {}

  protected:
    virtual bool newSource();
    // Compute functions
    virtual void m_computeScalar (uint8_t computeIndex);
};




#endif // IUFEATURE_H
