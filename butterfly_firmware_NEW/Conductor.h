#ifndef CONDUCTOR_H
#define CONDUCTOR_H

#include <Arduino.h>

#include "FeatureClass.h"
#include "FeatureComputer.h"
#include "FeatureGroup.h"
#include "IUI2C.h"
#include "IUSerial.h"
#include "IUBMD350.h"
#include "IUESP8285.h"
#include "IUSPIFlash.h"
#include "IURGBLed.h"
#include "IUBattery.h"
#include "IUBMP280.h"
#include "IUBMX055Acc.h"
#include "IUBMX055Gyro.h"
#include "IUBMX055Mag.h"
#include "IUCAMM8Q.h"
#include "IUI2S.h"
#include "IURTDExtension.h"

/**
 *
 * A representaton of the relations between data sources, computers and features
 *
 * In the feature, roles are as follow:
 *  - sensors: the data sources
 *  - features: the graph edges
 *  - computers: the graph nodes
 *  - (USB, BLE or WiFi interfaces): the "sinks" - They are not included in the
 *  graph for now, but features have functions to be streamed through those
 *  sinks.
 * FeatureGraph class packs method to activate / deactivate part of the graph,
 * that means enabling / disabling feature computations.
 * The FeatureGraph instance also has a collection of FeatureGroups (which are
 * a collection of features). Activating a FeatureGroup is equivalent to
 * activating the corresponding part of the graph + the required dependencies.
 */
class Conductor
{
    public:
        /***** Preset values and default settings *****/
        enum sleepMode : uint8_t {NONE     = 0,
                                  AUTO     = 1,
                                  PERIODIC = 2,
                                  COUNT    = 3};
        // Default start datetime
        static constexpr double defaultTimestamp = 1492144654.00;
        // Operation state shown on LED every X ms
        static const uint16_t showOpStateTimer = 500;
        /***** Constructors and destructor *****/
        Conductor(const char* version, const char* macAddress);
        virtual ~Conductor() {}
        char* getVersion() { return m_version; }
        char* getMacAddress() { return m_macAddress; }
        /***** Hardware & power management *****/
        void sleep(uint32_t duration);
        void suspend(uint32_t duration);
        void manageSleepCycles();
        /***** Configuration *****/
        bool configure(JsonVariant &my_config);
        void activateFeature(Feature* feature);
        bool isFeatureDeactivatable(Feature* feature);
        void deactivateFeature(Feature* feature);
        void deactivateAllFeatures();
        void activateGroup(FeatureGroup *group);
        void deactivateGroup(FeatureGroup *group);
        void deactivateAllGroups();
        /***** Time management *****/
        void setRefDatetime(double refDatetime);
        double getDatetime();
        /***** Operations *****/
        void setCallback(void (*callback)()) { m_callback = callback; };
        bool beginDataAcquisition();
        void endDataAcquisition();
        bool resetDataAcquisition();
        void acquireData(bool asynchronous);
        void computeFeatures();
        void updateOperationState();
        void streamFeatures();
        void storeData() {}  // TODO => implement
        /***** Mode management *****/
        void changeAcquisitionMode(AcquisitionMode::option mode);
        AcquisitionMode::option getAcquisitionMode()
            { return m_acquisitionMode; }
        void changeStreamingMode(StreamingMode::option mode);
        StreamingMode::option getStreamingMode() { return m_streamingMode; }
        void changeUsageMode(UsageMode::option usage);
        UsageMode::option getUsageMode() { return m_usageMode; }

    protected:
        char m_macAddress[18];
        char m_version[6];  // eg: "1.0.0"
        /***** Hardware & power management *****/
        sleepMode m_sleepMode;
        // Timestamp at which idle phase (or cycle) started for AUTO (or
        // PERIODIC) sleep mode
        uint32_t m_startTime;
        // Duration in which the device must be "IDLE" before entering
        // auto_sleep => Used with "AUTO" sleep mode only
        uint32_t m_autoSleepDelay;
        // Duration of sleep phase => Used both with "PERIODIC" and
        // "AUTO" sleep modes
        uint32_t m_sleepDuration;
        // Duration of total cycle (sleep + active) => Used with "PERIODIC"
        // sleep mode only
        uint32_t m_cycleTime;
        /***** Time management *****/
        uint32_t m_lastSynchroTime;
        double m_refDatetime;  // last datetime received from bluetooth or wifi
        /***** Operations *****/
        OperationState::option m_operationState;
        void (*m_callback)();
        bool m_inDataAcquistion;
        // The last time the LED was lit to reflect the OP state.
        uint32_t m_lastLitLedTime;
        /***** Configuration and Mode management *****/
        UsageMode::option m_usageMode;
        AcquisitionMode::option m_acquisitionMode;
        StreamingMode::option m_streamingMode;
};


/***** Instanciation *****/

extern Conductor conductor;


#endif // CONDUCTOR_H

