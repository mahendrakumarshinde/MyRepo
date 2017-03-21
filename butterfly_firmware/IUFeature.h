/*
  Infinite Uptime Firmware

*/


#ifndef IUFEATURE_H
#define IUFEATURE_H

#include <Arduino.h>

#include "IUUtilities.h"
#include "IUABCFeature.h"
#include "IUABCProducer.h"


/* ========================== Feature classes ============================= */

/**
 * Mixing class for IUABCFeature and IUABCProducer
 */
class IUFeature : public IUABCFeature, public IUABCProducer
{
  public:
    enum dataSendOption : uint8_t {value = 0,
                                   state = 1,
                                   optionCount = 2};
    IUFeature(uint8_t id, String name="", String fullName = "", bool fromSecondaryConfig = false);
    virtual ~IUFeature() {}
    bool isFromSecondaryConfig() { return m_fromSecondaryConfig; }
    virtual void sendToReceivers();


  protected:
    bool m_fromSecondaryConfig;
};


/**
* Feature with possibly multiple sources of which number format is Q15
*/
class IUQ15Feature : public IUFeature
{
  public:
    IUQ15Feature(uint8_t id, String name="", String fullName = "", bool fromSecondaryConfig = false);
    virtual ~IUQ15Feature() {}
    // Feature computation, source and sending queue
    virtual void setComputeFunction(float (*computeFunction) (uint8_t sourceCount, const uint16_t *sourceSize, q15_t **source))
                    { m_computeFunction = computeFunction; }
    virtual void setDefaultComputeFunction() { m_computeFunction = computeDefaultQ15; }
    virtual bool receive(uint8_t sourceIndex, q15_t value);

  protected:
    q15_t *m_source[2][ABCSourceCount];
    float (*m_computeFunction) (uint8_t sourceCount, const uint16_t *sourceSize, q15_t **source);
    virtual bool newSource();
};


/**
* Feature with possibly multiple sources of which number format is float
*/
class IUFloatFeature : public IUFeature
{
  public:
    IUFloatFeature(uint8_t id, String name="", String fullName = "", bool fromSecondaryConfig = false);
    virtual ~IUFloatFeature() {}
    // Feature computation, source and sending queue
    virtual void setComputeFunction(float (*computeFunction) (uint8_t sourceCount, const uint16_t *sourceSize, float **source))
                    { m_computeFunction = computeFunction; }
    virtual void setDefaultComputeFunction() { m_computeFunction = computeDefaultFloat; }
    virtual bool receive(uint8_t sourceIndex, float value);

  protected:
    float *m_source[2][ABCSourceCount];
    float (*m_computeFunction) (uint8_t sourceCount, const uint16_t *sourceSize, float **source);
    virtual bool newSource();
};


/* ========================== Specialized Feature Classes ============================= */

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
    uint8_t getSourceCount() { return sourceCount; }
    uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    uint16_t const* getSourceSize() { return sourceSize; }
    
    IUSingleAxisEnergyFeature128(uint8_t id, String name="", String fullName = "", bool fromSecondaryConfig = false);
    virtual ~IUSingleAxisEnergyFeature128() {}
    virtual void setDefaultComputeFunction() { m_computeFunction = computeSignalEnergy; }
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
    uint8_t getSourceCount() { return sourceCount; }
    uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    uint16_t const* getSourceSize() { return sourceSize; }
    
    IUSingleAxisEnergyFeature512(uint8_t id, String name="", String fullName = "", bool fromSecondaryConfig = false);
    virtual ~IUSingleAxisEnergyFeature512() {}
    virtual void setDefaultComputeFunction() { m_computeFunction = computeSignalEnergy; }
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
    uint8_t getSourceCount() { return sourceCount; }
    uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    uint16_t const* getSourceSize() { return sourceSize; }
    
    IUTriAxisEnergyFeature128(uint8_t id, String name="", String fullName = "", bool fromSecondaryConfig = false);
    virtual ~IUTriAxisEnergyFeature128() {}
    virtual void setDefaultComputeFunction() { m_computeFunction = computeSignalEnergy; }
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
    uint8_t getSourceCount() { return sourceCount; }
    uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    uint16_t const* getSourceSize() { return sourceSize; }
    
    IUTriAxisEnergyFeature512(uint8_t id, String name="", String fullName = "", bool fromSecondaryConfig = false);
    virtual ~IUTriAxisEnergyFeature512() {}
    virtual void setDefaultComputeFunction() { m_computeFunction = computeSignalEnergy; }
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
    uint8_t getSourceCount() { return sourceCount; }
    uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    uint16_t const* getSourceSize() { return sourceSize; }
    
    IUTriSourceSummingFeature(uint8_t id, String name="", String fullName = "", bool fromSecondaryConfig = false);
    virtual ~IUTriSourceSummingFeature() {}
    virtual void setDefaultComputeFunction() { m_computeFunction = computeSumOf; }
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
    uint8_t getSourceCount() { return sourceCount; }
    uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    uint16_t const* getSourceSize() { return sourceSize; }
    
    IUVelocityFeature512(uint8_t id, String name="", String fullName = "", bool fromSecondaryConfig = false);
    virtual ~IUVelocityFeature512() {}
    virtual void setDefaultComputeFunction() { m_computeFunction = computeVelocity; }
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
    uint8_t getSourceCount() { return sourceCount; }
    uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    uint16_t const* getSourceSize() { return sourceSize; }
    
    IUDefaultFloatFeature(uint8_t id, String name="", String fullName = "", bool fromSecondaryConfig = false);
    virtual ~IUDefaultFloatFeature() {}
    virtual void setDefaultComputeFunction() { m_computeFunction = computeDefaultFloat; }
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
    uint8_t getSourceCount() { return sourceCount; }
    uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    uint16_t const* getSourceSize() { return sourceSize; }
    
    IUAudioDBFeature2048(uint8_t id, String name="", String fullName = "", bool fromSecondaryConfig = false);
    virtual ~IUAudioDBFeature2048() {}
    virtual void setDefaultComputeFunction() { m_computeFunction = computeAcousticDB; }
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
    uint8_t getSourceCount() { return sourceCount; }
    uint16_t getSourceSize(uint8_t index) { return sourceSize[index]; }
    uint16_t const* getSourceSize() { return sourceSize; }
    
    IUAudioDBFeature4096(uint8_t id, String name="", String fullName = "", bool fromSecondaryConfig = false);
    virtual ~IUAudioDBFeature4096() {}
    virtual void setDefaultComputeFunction() { m_computeFunction = computeAcousticDB; }
};




#endif // IUFEATURE_H
