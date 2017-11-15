#ifndef FEATUREGROUP_H
#define FEATUREGROUP_H

#include <Arduino.h>
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
        bool isDataSendTime();
        /***** Communication *****/
        void stream(HardwareSerial *port, OperationState::option opState,
                    double timestamp);
        void legacyStream(HardwareSerial *port, const char *macAddress,
                          OperationState::option opState, float batteryLoad,
                          double timestamp);

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
        uint32_t m_lastSentTime;

};


/***** Instantiation *****/

extern FeatureGroup healthCheckGroup;
extern FeatureGroup calibrationGroup;
extern FeatureGroup rawAccelGroup;
extern FeatureGroup pressStandardGroup;
extern FeatureGroup motorStandardGroup;


/***** Populate groups *****/

extern void populateFeatureGroups();

#endif // FEATUREGROUP_H
