#ifndef SENSOR_H
#define SENSOR_H

#include "IUComponent.h"
#include "FeatureBuffer.h"


namespace IUComponent
{
    /**
     * Generic Sensor class
     *
     * AcquireData calls readData by taking into account the desired sampling
     * rate and the callbackRate: acquireData is called at m_callbackRate, but
     * we only want to sample data at m_samplingRate.
     */
    class Sensor : public ABCComponent
    {
        public:
            static const uint8_t maxDestinationCount = 3;
            static const uint16_t defaultSamplingRate = 2; // Hz
            static const uint16_t defaultCallbackRate = 1000; // Hz
            /***** Constructors and destructors *****/
            Sensor(uint8_t id=0, uint8_t destinationCount=0,
                   FeatureBuffer *destination0=NULL,
                   FeatureBuffer *destination1=NULL,
                   FeatureBuffer *destination2=NULL);
            virtual ~Sensor() {}
            /***** Core *****/
            virtual uint8_t getId() { return m_id; }
            virtual uint8_t getDestinationCount()
                { return m_destinationCount; }
            virtual uint8_t getMaxDestinationCount()
                { return maxDestinationCount; }
            /***** Sampling and resolution *****/
            virtual void setCallbackRate(uint16_t callbackRate);
            virtual uint16_t getCallbackRate() { return m_callbackRate; }
            virtual void setSamplingRate(uint16_t samplingRate);
            virtual uint16_t getSamplingRate() { return m_samplingRate; }
            virtual bool isAsynchronous() { return m_asynchronous; }
            virtual void setResolution(uint16_t resolution);
            virtual uint16_t getResolution() { return m_resolution; }
            /***** Data acquisition *****/
            virtual void prepareDataAcquisition();
            virtual bool acquireData();
            virtual void readData() {}  // May be defined in Child class
            /***** Communication *****/
            // May be defined in Child class
            virtual void dumpDataThroughI2C() {}
            virtual void dumpDataForDebugging() {}
            /***** Debugging *****/
            // May be defined in Child class
            virtual void exposeCalibration() {}

        protected:
            /***** Core *****/
            uint8_t m_id;
            uint8_t m_destinationCount;
            FeatureBuffer *m_destinations[maxDestinationCount];
            /***** Sampling and resolution *****/
            uint16_t m_callbackRate;
            uint16_t m_samplingRate;
            bool m_asynchronous;
            uint16_t m_downclocking;
            uint16_t m_downclockingCount;
            uint16_t m_resolution;
            /***** Hardware & power management *****/
            PowerMode::option m_powerMode;
    };
};

#endif // SENSOR_H
