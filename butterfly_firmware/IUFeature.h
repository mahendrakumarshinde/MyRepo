/*
  Infinite Uptime Firmware

*/


#ifndef IUFEATURE_H
#define IUFEATURE_H

#include <Arduino.h>

#include "IUABCFeature.h"
#include "IUABCProducer.h"


/* ===================== Feature calculation functions ===================== */

// Scalar feature functions

float computeVelocity(q15_t *accelFFT, float accelRMS, uint16_t sampleCount, uint16_t samplingRate, uint16_t FreqLowerBound, uint16_t FreqHigherBound); 

float computeFullVelocity(q15_t *accelFFT, uint16_t sampleCount, uint16_t samplingRate, uint16_t FreqLowerBound, uint16_t FreqHigherBound, float scalingFactor);

float computeFullVelocity(q15_t *accelFFT, uint16_t sampleCount, uint16_t samplingRate, uint16_t FreqLowerBound, uint16_t FreqHigherBound, float scalingFactor, q15_t *window);

float computeAcousticDB(uint8_t sourceCount, const uint16_t* sourceSize, q15_t **source);

// Array feature functions

bool computeAudioRFFT(uint8_t sourceCount, const uint16_t* sourceSize, q15_t **source, const uint16_t destinationSize, q15_t *destination);


/* ========================== Feature classes ============================= */

/*
 *
 */
class IUFeatureProducer: public IUABCProducer
{
  public:
    static const uint8_t maxReceiverCount = 10;
    static const uint16_t destinationSize = 1;
    enum dataSendOption : uint8_t {value = 0,
                                   state = 1,
                                   samplingRate = 2,
                                   sampleCount = 3,
                                   RMS = 4,
                                   mainFreq = 5,
                                   resolution = 6,
                                   scalarOptionCount = 7,
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
    virtual void setRMS(float rms) { m_rms = rms; }
    virtual float getRMS() { return m_rms; }
    virtual void setMainFreq(float value) { m_mainFreq = value; }
    virtual float getMainFreq() { return m_mainFreq; }
    virtual void setResolution(q15_t value) { m_resolution = value; }
    virtual q15_t getResolution() { return m_resolution; }
    virtual void setState(operationState state) { m_state = state; }
    virtual operationState getState() { return m_state; }
    virtual void setHighestDangerLevel(operationState state) { m_highestDangerLevel = state; }
    virtual operationState getHighestDangerLevel() { return m_highestDangerLevel; }
    // Feature computation, source and sending queue
    virtual void sendToReceivers();
    virtual bool addArrayReceiver(uint8_t sendOption, uint8_t receiverSourceIndex, IUABCFeature *receiver);
    virtual bool addReceiver(uint8_t sendOption, uint8_t receiverSourceIndex, IUABCFeature *receiver);

  protected:
    float m_latestValue;
    float m_rms;
    float m_mainFreq;
    q15_t *m_destination;
    uint16_t m_samplingRate;
    uint16_t m_sampleCount;
    q15_t m_resolution;
    operationState m_state;                  // Operation state
    operationState m_highestDangerLevel;     // The most critical state ever measured
};


/*
 *
 */
class IUFeatureProducer256: public IUFeatureProducer
{
  public:
    static const uint16_t destinationSize = 256;
    IUFeatureProducer256();
    virtual ~IUFeatureProducer256() {};
    virtual uint16_t getDestinationSize() { return destinationSize; }
    
};


/*
 *
 */
class IUFeatureProducer1024: public IUFeatureProducer
{
  public:
    static const uint16_t destinationSize = 1024;
    IUFeatureProducer1024();
    virtual ~IUFeatureProducer1024() {};
    virtual uint16_t getDestinationSize() { return destinationSize; }
};

/* ========================== Feature classes ============================= */

/**
 * Feature class
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
    virtual void resetReceivers() { m_producer->resetReceivers(); }

   protected:
    IUFeatureProducer *m_producer;
};


/**
* Feature with possibly multiple sources of which number format is Q15
*/
class IUQ15Feature : public IUFeature
{
  public:
    static const q15_t rescaleFloatScalar = 512;
    IUQ15Feature(uint8_t id, char *name);
    virtual ~IUQ15Feature() {}
    // Feature computation, source and sending queue
    void resetSource(bool deletePtr = false);
    virtual bool setSource(uint8_t sourceIndex, uint16_t valueCount, q15_t *values);
    virtual void getSource(uint8_t idx, q15_t **values) { values = m_source[idx]; }
    virtual bool receiveScalar(uint8_t sourceIndex, q15_t value);
    virtual bool receiveScalar(uint8_t sourceIndex, float value);
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
    virtual bool receiveScalar(uint8_t sourceIndex, q15_t value);
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
    static const uint8_t sourceCount = 3;
    static const uint16_t sourceSize[sourceCount];
    static constexpr float accelRMSThreshold = 0.25;

    IUAccelPreComputationFeature128(uint8_t id, char *name);
    virtual ~IUAccelPreComputationFeature128() {}
    virtual uint8_t getSourceCount() { return sourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    virtual uint16_t const* getSourceSize() { return sourceSize; }
    virtual void streamSourceData(HardwareSerial *port, String macAddr, String keyword);

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
    static const uint8_t sourceCount = 3;
    static const uint16_t sourceSize[sourceCount];
    static constexpr float accelRMSThreshold = 0.25;

    IUAccelPreComputationFeature512(uint8_t id, char *name);
    virtual ~IUAccelPreComputationFeature512() {}
    virtual uint8_t getSourceCount() { return sourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    virtual uint16_t const* getSourceSize() { return sourceSize; }
    virtual void streamSourceData(HardwareSerial *port, String macAddr, String keyword);

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
    static const uint8_t sourceCount = 3;
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
    static const uint8_t sourceCount = 5;
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
    static const uint8_t sourceCount = 3;
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
    static const uint8_t sourceCount = 5;
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
    static const uint8_t sourceCount = 5;
    static const uint16_t sourceSize[sourceCount];
    static const q15_t accelRMSThreshold = (q15_t) (0.25 * rescaleFloatScalar);
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
