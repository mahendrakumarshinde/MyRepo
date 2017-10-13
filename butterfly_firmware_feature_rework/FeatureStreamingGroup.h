#ifndef FEATURESTREAMINGGROUP_H
#define FEATURESTREAMINGGROUP_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include "FeatureClass.h"

/**
 * A list of features that must be streamed together
 *
 * The class also handles the streaming itself, including formatting the
 * message. The features are streamed in the order they are listed in the group.
 */
class FeatureStreamingGroup
{
    public:
        /***** Preset values and default settings *****/
        static const uint8_t maxFeatureCount = 10;
        static const uint16_t defaultDataSendPeriod = 512; // ms
        /***** Constructors & destructor *****/
        FeatureStreamingGroup(uint8_t id);
        virtual ~FeatureStreamingGroup() { }
        void reset();
        void add(uint8_t idx, Feature *feature);
        /***** Time management *****/
        void setDataSendPeriod(uint16_t dataSendPeriod);
        bool isDataSendTime();
        /***** Communication *****/
        void stream(HardwareSerial *port, OperationState::option opState,
                    double timestamp);

    protected:
        uint8_t m_id;
        Feature *m_features[maxFeatureCount];
        uint8_t m_featureCount;  // Index of last feature + 1
        /***** Time management *****/
        uint16_t m_dataSendPeriod;
        uint32_t m_lastSentTime;

};

#endif // FEATURESTREAMINGGROUP_H
