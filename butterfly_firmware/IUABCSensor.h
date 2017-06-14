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
    enum sensorTypeOptions : char {NONE         = '0',
                                   ACCELERATION = 'A',
                                   ATMOSPHERE   = 'T', // Temperature + pressure
                                   BATTERY      = 'B',
                                   SATELLITE    = 'G',
                                   ORIENTATION  = 'O', // Gyroscope
                                   MAGNETISM    = 'M',
                                   SOUND        = 'S',
                                   ULTRASOUND   = 'U'};
    static const sensorTypeOptions ABCSensorType = sensorTypeOptions::NONE;

    static const uint16_t defaultSamplingRate = 2; // Hz
    static const uint16_t defaultCallbackRate = 1000; // Hz
    // Constructor, destructor, getters and setters
    IUABCSensor();
    virtual ~IUABCSensor();
    virtual bool isAsynchronous() { return m_asynchronous; }
    virtual void setSamplingRate(uint16_t samplingRate);
    virtual uint16_t getSamplingRate() { return m_samplingRate; }
    virtual void setCallbackRate(uint16_t callbackRate);
    virtual uint16_t getCallbackRate() { return m_callbackRate; }
    virtual char getSensorType() { return (char) ABCSensorType; }

    // Hardware & power management methods
    virtual void switchToPowerMode(powerMode::option pMode);
    virtual void wakeUp() {}                 // May be defined in Child class
    virtual void sleep() {}                  // May be defined in Child class
    virtual void suspend() {}                // May be defined in Child class
    // Data acquisition methods
    virtual void prepareDataAcquisition();
    virtual bool acquireData();
    virtual void readData() {}                // May be defined in Child class
    // Communication methods
    virtual void sendToReceivers() {}         // May be defined in Child class
    virtual void dumpDataThroughI2C() {}      // May be defined in Child class
    virtual void dumpDataForDebugging() {}    // May be defined in Child class
    // Diagnostic Functions
    virtual void exposeCalibration() {}       // May be defined in Child class


  protected:
    powerMode::option m_powerMode;
    bool m_asynchronous;
    uint16_t m_samplingRate;
    uint16_t m_callbackRate;
    uint16_t m_downclocking;
    uint16_t m_downclockingCount;

};

#endif // IUABCSENSOR_H
