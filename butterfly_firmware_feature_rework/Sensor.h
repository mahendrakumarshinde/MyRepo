#ifndef SENSOR_H
#define SENSOR_H

#include <ArduinoJson.h>

#include "Component.h"
#include "FeatureClass.h"


/**
 * Generic Sensor class
 *
 * AcquireData calls readData by taking into account the desired sampling
 * rate and the callbackRate: acquireData is called at m_callbackRate, but
 * we only want to sample data at m_samplingRate.
 */
class Sensor : public Component
{
    public:
        /***** Preset values and default settings *****/
        enum usagePreset : uint8_t {P_LOW      = 0,
                                    P_REGULAR  = 1,
                                    P_ENHANCED = 2,
                                    P_HIGH     = 3,
                                    COUNT    = 4};
        static const uint8_t maxDestinationCount = 3;
        /***** Constructors and destructors *****/
        Sensor(const char* name, uint8_t destinationCount=0,
               Feature *destination0=NULL, Feature *destination1=NULL,
               Feature *destination2=NULL);
        virtual ~Sensor() {}
        /***** Core *****/
        virtual char* getName() { return m_name; }
        virtual bool isNamed(const char* name)
            { return strcmp(m_name, name) == 0; }
        virtual uint8_t getDestinationCount()
            { return m_destinationCount; }
        virtual uint8_t getMaxDestinationCount()
            { return maxDestinationCount; }
        /***** Configuration *****/
        virtual bool configure(JsonVariant &config) { return true; }
        /***** Sampling and resolution *****/
        virtual bool isAsynchronous() { return false; }
        virtual void setCallbackRate(uint16_t callbackRate) {} // For asynchronous sensors
        virtual void setResolution(uint16_t resolution);
        virtual uint16_t getResolution() { return m_resolution; }
        /***** Data acquisition *****/
        virtual void acquireData() {}
        virtual void readData() {}  // May be defined in Child class
        /***** Communication *****/
        void sendData(HardwareSerial *port) { }
        /***** Debugging *****/
        virtual void expose();
        virtual void exposeCalibration() {}

    protected:
        /***** Core *****/
        char m_name[4];
        uint8_t m_destinationCount;
        Feature *m_destinations[maxDestinationCount];
        /***** Sampling and resolution *****/
        uint16_t m_resolution;
};


/**
 * Asynchronous Sensor class
 *
 * Asynchronous sensors acquire data based on an interrupt, so their sampling
 * rate is fairly accurate. This class should used for:
 * - sensors that can read data in a short time.
 * - data that needs to be regularly sampled.
 */
class AsynchronousSensor : public Sensor
{
    public:
        /***** Preset values and default settings *****/
        static const uint16_t defaultSamplingRate = 2; // Hz
        static const uint16_t defaultCallbackRate = 1000; // Hz
        /***** Constructors and destructors *****/
        AsynchronousSensor(const char* name, uint8_t destinationCount=0,
                           Feature *destination0=NULL,
                           Feature *destination1=NULL,
                           Feature *destination2=NULL);
        virtual ~AsynchronousSensor() {}
        /***** Configuration *****/
        virtual bool configure(JsonVariant &config);
        /***** Sampling and resolution *****/
        virtual bool isAsynchronous() { return true; }
        virtual void setCallbackRate(uint16_t callbackRate);
        virtual uint16_t getCallbackRate() { return m_callbackRate; }
        virtual void setSamplingRate(uint16_t samplingRate);
        virtual uint16_t getSamplingRate() { return m_samplingRate; }
        virtual void computeDownclockingRate();
        /***** Data acquisition *****/
        virtual void acquireData();

    protected:
        /***** Sampling and resolution *****/
        uint16_t m_callbackRate;
        uint16_t m_samplingRate;
        uint16_t m_downclocking;
        uint16_t m_downclockingCount;
};


/**
 * Synchronous Sensor class
 *
 * Synchronous sensor acquires data in the main process loop, and respect at
 * least samplingPeriod between each sampling. This class should used for:
 * - sensors that can take a fairly long time to read data.
 * - data that doesn't need to be regularly sampled.
 * The samplingPeriod would typically be > 500ms and could be several hours.
 */
class SynchronousSensor : public Sensor
{
    public:
        /***** Preset values and default settings *****/
        static const usagePreset defaultUsagePreset = P_REGULAR;
        static const uint32_t defaultSamplingPeriod = 1000; // ms
        /***** Constructors and destructors *****/
        SynchronousSensor(const char* name, uint8_t destinationCount=0,
                           Feature *destination0=NULL,
                           Feature *destination1=NULL,
                           Feature *destination2=NULL);
        virtual ~SynchronousSensor() {}
        /***** Configuration *****/
        virtual bool configure(JsonVariant &config);
        virtual void changeUsagePreset(Sensor::usagePreset usage);
        virtual Sensor::usagePreset getUsagePreset() { return m_usagePreset; }
        virtual void switchToLowUsage() { m_usagePreset = Sensor::P_LOW; }
        virtual void switchToRegularUsage()
            { m_usagePreset = Sensor::P_REGULAR; }
        virtual void switchToEnhancedUsage()
            { m_usagePreset = Sensor::P_ENHANCED; }
        virtual void switchToHighUsage() { m_usagePreset = Sensor::P_HIGH; }
        /***** Sampling and resolution *****/
        virtual bool isAsynchronous() { return false; }
        virtual void setSamplingPeriod(uint32_t samplingPeriod)
            { m_samplingPeriod = samplingPeriod; }
        virtual uint32_t getSamplingPeriod() { return m_samplingPeriod; }
        virtual float getSamplingRate()
            { return 1.0f / (float) m_samplingPeriod; }
        /***** Data acquisition *****/
        virtual void acquireData();

    protected:
        /***** Configuration *****/
        usagePreset m_usagePreset;
        /***** Sampling and resolution *****/
        uint32_t m_samplingPeriod;
        uint32_t m_lastAcquisitionTime;
};


#endif // SENSOR_H
