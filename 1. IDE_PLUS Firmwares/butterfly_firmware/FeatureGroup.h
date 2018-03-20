#ifndef FEATUREGROUP_H
#define FEATUREGROUP_H

#include <Arduino.h>
#include <MacAddress.h>
#include <ArduinoJson.h>

#include "FeatureClass.h"

/**
 * A list of features that allows to consistently monitor 1 (part of a) machine.
 *
 * The class also handles the streaming itself, including formatting the
 * message. The features are indexed and streamed in the order of the index.
 */
class FeatureGroup
{
    public:
        /***** Preset values and default settings *****/
        static const uint8_t maxFeatureCount = 10;
        static const uint16_t maxBufferSize = 700;
        static const uint16_t maxBufferMargin = 200;
        static const uint32_t maxBufferDelay = 3000;
        /***** Instance registry *****/
        static const uint8_t MAX_INSTANCE_COUNT = 10;
        static uint8_t instanceCount;
        static FeatureGroup *instances[MAX_INSTANCE_COUNT];
        /***** Constructors & destructor *****/
        FeatureGroup(const char *name, uint16_t dataSendPeriod=512);
        virtual ~FeatureGroup();
        char* getName() { return m_name; }
        bool isNamed(const char* name) { return strcmp(m_name, name) == 0; }
        static FeatureGroup *getInstanceByName(const char *name);
        /***** Core *****/
        void reset();
        void addFeature(Feature *feature);
        uint8_t getFeatureCount() { return m_featureCount; }
        Feature* getFeature(uint8_t idx) { return m_features[idx]; }
        /***** Configuration *****/
        void activate() { m_active = true; }
        void deactivate() { m_active = false; }
        bool isActive() { return m_active; }
        /***** Time management *****/
        void setDataSendPeriod(uint16_t dataSendPeriod);
        bool isDataSendTime(uint8_t idx=0);
        /***** Communication *****/
        void stream(HardwareSerial *port, MacAddress mac,
                    double timestamp, bool sendMACAddress=false);
        void legacyStream(
            HardwareSerial *port, MacAddress mac,
            OperationState::option opState, float batteryLoad, double timestamp,
            bool sendName=false, uint8_t portIdx=0);
        void legacyBufferStream(
            HardwareSerial *port, MacAddress mac,
            OperationState::option opState, float batteryLoad, double timestamp,
            bool sendName=false);

    protected:
        /***** Instance registry *****/
        uint8_t m_instanceIdx;
        /***** Group Designation *****/
        char m_name[7];
        /***** Core *****/
        Feature *m_features[maxFeatureCount];
        uint8_t m_featureCount;  // Index of last feature + 1
        /***** Configuration *****/
        bool m_active;
        /***** Time management *****/
        uint16_t m_dataSendPeriod;  // ms
        uint32_t m_lastSentTime[2];
        /***** Feature Buffering *****/
        char m_featureBuffer[maxBufferSize];
        uint16_t m_bufferIndex;
        uint32_t m_bufferStartTime;
};

#endif // FEATUREGROUP_H
