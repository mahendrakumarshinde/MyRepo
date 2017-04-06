#ifndef IUABCSENSOR_H
#define IUABCSENSOR_H

#include <Arduino.h>
#include "IUABCProducer.h"

/**
 * Generic Sensor class
 *
 * SendToReceivers is to be defined in child class, and should handle data
 * reading and data sending to receivers.
 * AcquireData calls sendToReceivers by taking into account the desired sampling
 * rate and the callbackRate: acquireData is called at m_callbackRate, but we only
 * want to send data at m_samplingRate.
 */
class IUABCSensor : public IUABCProducer
{
  public:
    static const char sensorType_none = '0';
    static const char sensorType_accelerometer = 'A';
    static const char sensorType_battery = 'B';
    static const char sensorType_gyroscope = 'G';
    static const char sensorType_rgbled = 'I';          // for info
    static const char sensorType_gps = 'L';             // for location
    static const char sensorType_magnetometer = 'M';
    static const char sensorType_barometer = 'P';       // for pressure
    static const char sensorType_microphone = 'S';      // for sound
    static const char sensorType_thermometer = 'T';
    static const uint8_t ABCSensorTypeCount = 1;
    static char ABCSensorTypes[ABCSensorTypeCount];

    static const uint16_t defaultSamplingRate = 2; // Hz
    static const uint16_t defaultCallbackRate = 1000; // Hz
    // Constructor, destructor, getters and setters
    IUABCSensor();
    virtual ~IUABCSensor();
    virtual void setSamplingRate(uint16_t samplingRate);
    virtual uint16_t getSamplingRate() { return m_samplingRate; }
    virtual void setCallbackRate(uint16_t callbackRate);
    virtual uint16_t getCallbackRate() { return m_callbackRate; }
    virtual uint8_t getSensorTypeCount() { return ABCSensorTypeCount; }
    virtual char getSensorType(uint8_t index) { return ABCSensorTypes[index]; }

    // Methods
    virtual void wakeUp() {}                  // May be defined in Child class
    virtual void prepareDataAcquisition();
    virtual bool acquireData();
    virtual void readData() {}                // May be defined in Child class
    virtual void sendToReceivers() {}         // May be defined in Child class
    virtual void dumpDataThroughI2C() {}      // May be defined in Child class
    virtual void dumpDataForDebugging() {}    // May be defined in Child class
    // Diagnostic Functions
    virtual void exposeCalibration() {}       // May be defined in Child class


  protected:
    uint16_t m_samplingRate;
    uint16_t m_callbackRate;
    uint16_t m_downclocking;
    uint16_t m_downclockingCount;

};

#endif // IUABCSENSOR_H
