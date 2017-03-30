/*
  Infinite Uptime Firmware

*/


#ifndef IUFEATURE_H
#define IUFEATURE_H

#include <Arduino.h>

#include "IUUtilities.h"
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

/**
 * Mixing class for IUABCFeature and IUABCProducer
 */
class IUFeature : public IUABCProducer, public IUABCFeature
{
  public:
    enum dataSendOption : uint8_t {value = 0,
                                   state = 1,
                                   valueArray = 99,
                                   optionCount = 3};
    IUFeature(uint8_t id, char *name);
    virtual ~IUFeature() {}
    // Feature computation, source and sending queue
    virtual void sendToReceivers();
    virtual bool addArrayReceiver(uint8_t receiverSourceIndex, IUABCFeature *receiver);
    virtual bool addReceiver(uint8_t sendOption, uint8_t receiverSourceIndex, IUABCFeature *receiver);
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
    virtual bool setSource(uint8_t sourceIndex, uint16_t valueCount, q15_t *values);
    virtual void setcomputeScalarFunction(float (*computeScalarFunction) (uint8_t sourceCount, const uint16_t *sourceSize, q15_t **source))
                    { m_computeScalarFunction = computeScalarFunction; }
    virtual bool receiveScalar(uint8_t sourceIndex, q15_t value);

  protected:
    q15_t *m_source[2][ABCSourceCount];
    float (*m_computeScalarFunction) (uint8_t sourceCount, const uint16_t *sourceSize, q15_t **source);
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
    virtual bool setSource(uint8_t sourceIndex, uint16_t valueCount, float *values);
    virtual void setcomputeScalarFunction(float (*computeScalarFunction) (uint8_t sourceCount, const uint16_t *sourceSize, float **source))
                    { m_computeScalarFunction = computeScalarFunction; }
    virtual bool receiveScalar(uint8_t sourceIndex, float value);

  protected:
    float *m_source[2][ABCSourceCount];
    float (*m_computeScalarFunction) (uint8_t sourceCount, const uint16_t *sourceSize, float **source);
    virtual bool newSource();
};


/* ========================== Specialized Feature Classes ============================= */

/**
 * Signal Energy along a single axis, over 128 values + FFT
 * Excepted producers:
 * 1. Sensor data (eg: Accelerometer on 1 axis)
 * Optionnal expected receivers:
 * 1. operationState in q15_t format
 */
class IUAccelPreComputationFeature128: public IUQ15Feature
{
  public:
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
    static const uint16_t destinationSize = 128;
    virtual uint8_t getSourceCount() { return sourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    virtual uint16_t getDestinationSize() { return destinationSize; }
    virtual uint16_t const* getSourceSize() { return sourceSize; }

    IUAccelPreComputationFeature128(uint8_t id, char *name);
    virtual ~IUAccelPreComputationFeature128() {}

  protected:
    q15_t *m_source[2][sourceCount];
    uint16_t m_sourceCounter[sourceCount];
    bool m_computeNow[2][sourceCount];
    bool m_recordNow[2][sourceCount];
    q15_t *m_destination[destinationSize];
};


/**
 * Signal Energy along a single axis, over 128 values + FFT
 * Excepted producers:
 * 1. Sensor data (eg: Accelerometer on 1 axis)
 * Optionnal expected receivers:
 * 1. operationState in q15_t format
 */
class IUAccelPreComputationFeature512: public IUQ15Feature
{
  public:
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
    static const uint16_t destinationSize = 512;
    virtual uint8_t getSourceCount() { return sourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    virtual uint16_t getDestinationSize() { return destinationSize; }
    virtual uint16_t const* getSourceSize() { return sourceSize; }

    IUAccelPreComputationFeature512(uint8_t id, char *name);
    virtual ~IUAccelPreComputationFeature512() {}

  protected:
    q15_t *m_source[2][sourceCount];
    uint16_t m_sourceCounter[sourceCount];
    bool m_computeNow[2][sourceCount];
    bool m_recordNow[2][sourceCount];
    q15_t *m_destination[destinationSize];
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
    q15_t *m_source[2][sourceCount];
    uint16_t m_sourceCounter[sourceCount];
    bool m_computeNow[2][sourceCount];
    bool m_recordNow[2][sourceCount];
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
    q15_t *m_source[2][sourceCount];
    uint16_t m_sourceCounter[sourceCount];
    bool m_computeNow[2][sourceCount];
    bool m_recordNow[2][sourceCount];
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
    q15_t *m_source[2][sourceCount];
    uint16_t m_sourceCounter[sourceCount];
    bool m_computeNow[2][sourceCount];
    bool m_recordNow[2][sourceCount];
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
    q15_t *m_source[2][sourceCount];
    uint16_t m_sourceCounter[sourceCount];
    bool m_computeNow[2][sourceCount];
    bool m_recordNow[2][sourceCount];
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
    float *m_source[2][sourceCount];
    uint16_t m_sourceCounter[sourceCount];
    bool m_computeNow[2][sourceCount];
    bool m_recordNow[2][sourceCount];
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
    static const uint8_t sourceCount = 2;
    static const uint16_t sourceSize[sourceCount];
    virtual uint8_t getSourceCount() { return sourceCount; }
    virtual uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    virtual uint16_t const* getSourceSize() { return sourceSize; }

    IUVelocityFeature512(uint8_t id, char *name);
    virtual ~IUVelocityFeature512() {}

  protected:
    q15_t *m_source[2][sourceCount];
    uint16_t m_sourceCounter[sourceCount];
    bool m_computeNow[2][sourceCount];
    bool m_recordNow[2][sourceCount];
    virtual bool newSource();
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
    float *m_source[2][sourceCount];
    uint16_t m_sourceCounter[sourceCount];
    bool m_computeNow[2][sourceCount];
    bool m_recordNow[2][sourceCount];
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
    q15_t *m_source[2][sourceCount];
    uint16_t m_sourceCounter[sourceCount];
    bool m_computeNow[2][sourceCount];
    bool m_recordNow[2][sourceCount];
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
    q15_t *m_source[2][sourceCount];
    uint16_t m_sourceCounter[sourceCount];
    bool m_computeNow[2][sourceCount];
    bool m_recordNow[2][sourceCount];
};




#endif // IUFEATURE_H
