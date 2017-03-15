/*
  Infinite Uptime Firmware


   Feature definitions:
   
1. Acceleration RMS in X
8:12
2. Acceleration RMS in Y
8:12
3. Acceleration RMS in Z
8:13
4. Acceleration Peak-to-Peak in X
5. Acceleration Peak-to-Peak in Y
6. Acceleration Peak-to-Peak in Z
8:13
7. Velocity RMS in X
8. Velocity RMS in Y
9. Velocity RMS in Z
10. Velocity Peak-to-Peak in X
11. Velocity Peak-to-Peak in Y
12. Velocity Peak-to-Peak in Z
8:14
13. Displacement RMS in X
14. Displacement RMS in Y
15. Displacement RMS in Z
16. Displacement Peak-to-Peak in X
17. Displacement Peak-to-Peak in Y
18. Displacement Peak-to-Peak in Z
8:14
19. Signal  Energy (same as in existing firmware)
8:14
20. Latitude
8:14
21. Longitude
8:15
22. Acoustic (dB)
8:15
23. Temperature (C)
8:16
24. Energy in configurable frequency bands for vibration
8:16
25. Energy in configurable frequency bands for acoustics
   
*/


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
    IUQ15DataCollectionFeature(uint8_t id, String name="", String fullName = "");
    virtual ~IUQ15DataCollectionFeature() {}
    virtual void setDefaultComputeFunction() { m_computeFunction = computeDefaultDataCollectionQ15; } //Does nothing
    virtual bool compute();
    void setSourceNames(String *sourceNames);
    bool stream(Stream *port);

  protected:
    bool m_streamNow;
    float *m_destination[sourceCount];
    float (*m_computeFunction) (uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[], float *m_destination[]);
    String m_sourceNames[sourceCount];
};


/**
 * Mixing class for IUABCFeature and IUABCProducer
 */
class IUFeature : public IUABCFeature, public IUABCProducer
{
  public:
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
    IUFeature(uint8_t id, String name="", String fullName = "");
    virtual ~IUFeature() {}
    enum dataSendOption : uint8_t {value = 0,
                                   state = 1,
                                   optionCount = 2};
    virtual void sendToReceivers();
};


/**
* Feature with possibly multiple sources of which number format is Q15
*/
class IUQ15Feature : public IUFeature
{
  public:
    IUQ15Feature(uint8_t id, String name="", String fullName = "");
    virtual ~IUQ15Feature() {}
    // Feature computation, source and sending queue
    virtual void setComputeFunction(float (*computeFunction) (uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[]))
                    { m_computeFunction = computeFunction; }
    virtual void setDefaultComputeFunction() { m_computeFunction = computeDefaultQ15; }
    virtual bool receive(uint8_t sourceIndex, q15_t value);

  protected:
    q15_t *m_source[2][sourceCount];
    float (*m_computeFunction) (uint8_t sourceCount, const uint16_t *sourceSize, q15_t *source[]);
    virtual bool newSource();
};


/**
* Feature with possibly multiple sources of which number format is float
*/
class IUFloatFeature : public IUFeature
{
  public:
    IUFloatFeature(uint8_t id, String name="", String fullName = "");
    virtual ~IUFloatFeature() {}
    // Feature computation, source and sending queue
    virtual void setComputeFunction(float (*computeFunction) (uint8_t sourceCount, const uint16_t *sourceSize, float *source[]))
                    { m_computeFunction = computeFunction; }
    virtual void setDefaultComputeFunction() { m_computeFunction = computeDefaultFloat; }
    virtual bool receive(uint8_t sourceIndex, float value);

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
    IUSingleAxisEnergyFeature128(uint8_t id, String name="", String fullName = "");
    virtual ~IUSingleAxisEnergyFeature128() {}
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
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
    IUSingleAxisEnergyFeature512(uint8_t id, String name="", String fullName = "");
    virtual ~IUSingleAxisEnergyFeature512() {}
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
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
    IUTriAxisEnergyFeature128(uint8_t id, String name="", String fullName = "");
    virtual ~IUTriAxisEnergyFeature128() {}
    static const uint8_t sourceCount = 3;
    static const uint16_t sourceSize[sourceCount];
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
    IUTriAxisEnergyFeature512(uint8_t id, String name="", String fullName = "");
    virtual ~IUTriAxisEnergyFeature512() {}
    static const uint8_t sourceCount = 3;
    static const uint16_t sourceSize[sourceCount];
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
    IUTriSourceSummingFeature(uint8_t id, String name="", String fullName = "");
    virtual ~IUTriSourceSummingFeature() {}
    static const uint8_t sourceCount = 3;
    static const uint16_t sourceSize[sourceCount];
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
    IUVelocityFeature512(uint8_t id, String name="", String fullName = "");
    virtual ~IUVelocityFeature512() {}
    static const uint8_t sourceCount = 2;
    static const uint16_t sourceSize[sourceCount];
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
    IUDefaultFloatFeature(uint8_t id, String name="", String fullName = "");
    virtual ~IUDefaultFloatFeature() {}
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
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
    IUAudioDBFeature2048(uint8_t id, String name="", String fullName = "");
    virtual ~IUAudioDBFeature2048() {}
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
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
    IUAudioDBFeature4096(uint8_t id, String name="", String fullName = "");
    virtual ~IUAudioDBFeature4096() {}
    static const uint8_t sourceCount = 1;
    static const uint16_t sourceSize[sourceCount];
    virtual void setDefaultComputeFunction() { m_computeFunction = computeAcousticDB; }
};




#endif // IUFEATURE_H
