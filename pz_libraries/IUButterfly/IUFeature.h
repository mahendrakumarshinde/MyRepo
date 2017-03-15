#ifndef IUFEATURE_H
#define IUFEATURE_H

#include <Arduino.h>

#include "IUUtilities.h"
#include "IUABCFeature.h"
#include "IUABCProducer.h"


/* ========================== Feature classes ============================= */

/**
* Feature for data collection, with one or several sources with number in Q15 format
*
* Data collection feature is a special sort of feature:
* - the sendingQueue is not usefull => it will be of size 1 and receive only 0
* - it has a destination array instead, with the same shape as source
* - it is not a Producer sub-class
* - the compute feature update the destination array and return 0
* - the stream function stream the destination array
* - Sources can be named, so the name is streamed with the values
*/
class IUQ15DataCollectionFeature : public IUABCFeature
{
  public:
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
    IUQ15DataCollectionFeature(uint8_t id, String name="");
    virtual ~IUQ15DataCollectionFeature() {}
    virtual void setDefaultComputeFunction() { m_computeFunction = computeDefaultDataCollectionQ15; } //Does nothing
    virtual float compute();
    void setSourceNames(String *sourceNames);
    bool stream(Stream *port);

  protected:
    bool m_streamNow;
    float *m_destination[sourceCount];
    float (*m_computeFunction) (uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[], float *m_destination[]);
    String m_sourceNames[sourceCount];
};


/**
* Feature with possibly multiple sources of which number format is Q15
*/
class IUQ15Feature : public IUABCFeature, public IUABCProducer
{
  public:
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
    IUQ15Feature(uint8_t id, String name="");
    virtual ~IUQ15Feature() {}
    // Feature computation, source and sending queue
    virtual void setComputeFunction(float (*computeFunction) (uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[]))
                    { m_computeFunction = computeFunction; }
    virtual void setDefaultComputeFunction() { m_computeFunction = computeDefaultQ15; }
    virtual bool receive(uint8_t sourceIndex, q15_t value);
    virtual float compute();

  protected:
    q15_t *m_source[2][sourceCount];
    float (*m_computeFunction) (uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[]);
    virtual bool newSource();
};


/**
* Feature with possibly multiple sources of which number format is float
*/
class IUFloatFeature : public IUABCFeature, public IUABCProducer
{
  public:
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
    IUFloatFeature(uint8_t id, String name="");
    virtual ~IUFloatFeature();
    // Feature computation, source and sending queue
    virtual void setComputeFunction(float (*computeFunction) (uint8_t sourceCount, const uint16_t *sourceSize, float *source[]))
                    { m_computeFunction = computeFunction; }
    virtual void setDefaultComputeFunction() { m_computeFunction = computeDefaultFloat; }
    virtual bool receive(uint8_t sourceIndex, float value);
    virtual float compute();

  protected:
    float *m_source[2][sourceCount];
    float (*m_computeFunction) (uint8_t sourceCount, const uint16_t *sourceSize, float *source[]);
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
    virtual void setDefaultComputeFunction() { m_computeFunction = computeSignalEnergy; }
    virtual void sendToReceivers();
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
    virtual void setDefaultComputeFunction() { m_computeFunction = computeSignalEnergy; }
    virtual void sendToReceivers();
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
    virtual void setDefaultComputeFunction() { m_computeFunction = computeSignalEnergy; }
    virtual void sendToReceivers();
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
    virtual void setDefaultComputeFunction() { m_computeFunction = computeSignalEnergy; }
    virtual void sendToReceivers();
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
    virtual void setDefaultComputeFunction() { m_computeFunction = computeSumOf; }
    virtual void sendToReceivers();
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
    virtual void setDefaultComputeFunction() { m_computeFunction = computeVelocity; }
    virtual void sendToReceivers() {}
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
    virtual void setDefaultComputeFunction() { m_computeFunction = computeDefaultFloat; }
    virtual void sendToReceivers() {}
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
    virtual void setDefaultComputeFunction() { m_computeFunction = computeAcousticDB; }
    virtual void sendToReceivers() {}
};




#endif // IUFEATURE_H
