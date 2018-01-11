#ifndef CONDUCTOR_H
#define CONDUCTOR_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include "FeatureClass.h"
#include "FeatureComputer.h"
#include "FeatureGroup.h"
#include "IUI2C.h"
#include "IUSerial.h"
#include "IUBMD350.h"
#include "IURGBLed.h"
#include "IUBattery.h"
#include "IUBMP280.h"
#include "IUBMX055Acc.h"
#include "IUBMX055Gyro.h"
#include "IUBMX055Mag.h"
#include "IUCAMM8Q.h"
#include "IUI2S.h"

#ifdef INTERNAL_ESP8285  // Wifi options
    #include "IUESP8285.h"
#endif

#ifdef RTD_DAUGHTER_BOARD  // Optionnal hardware
    #include "IURTDExtension.h"
#endif



/**
 *
 * A representaton of the relations between data sources, computers and features
 *
 * In the feature, roles are as follow:
 *  - sensors: the data sources
 *  - features: the graph edges
 *  - computers: the graph nodes
 *  - Serial interfaces (or SPI Flash): the "sinks" - They are not included in
 *  the graph for now, but features have functions to be streamed through those
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
        static char START_CONFIRM[11];
        static char END_CONFIRM[9];
        // Default start datetime
        static constexpr double defaultTimestamp = 1492144654.00;
        // Operation state shown on LED every X ms
        static const uint16_t showOpStateTimer = 500;
        /***** Constructors and destructor *****/
        Conductor(const char* macAddress);
        virtual ~Conductor() {}
        char* getMacAddress() { return m_macAddress; }
        /***** Hardware & power management *****/
        void sleep(uint32_t duration);
        void suspend(uint32_t duration);
        void manageSleepCycles();
        sleepMode getSleepMode() { return m_sleepMode; }
        uint32_t getAutoSleepDelay() { return m_autoSleepDelay; }
        uint32_t getSleepDuration() { return m_sleepDuration; }
        uint32_t getCycleTime() { return m_cycleTime; }
        /***** Serial Reading & command processing*****/
        void readFromSerial(IUSerial *iuSerial);
        bool processConfiguration(char *json);
        bool configureMainOptions(JsonVariant &config);
        void configureAllSensors(JsonVariant &config);
        void configureAllFeatures(JsonVariant &config);
        void processLegacyUSBCommands(char *buff);
        void processLegacyBLECommands(char *buff);
        void processWIFICommands(char *buff);

        /***** Features and groups Management *****/
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
        /***** Mode management *****/
        void changeAcquisitionMode(AcquisitionMode::option mode);
        void changeStreamingMode(StreamingMode::option mode);
        void changeUsageMode(UsageMode::option usage);
        /***** Operations *****/
        void setCallback(void (*callback)()) { m_callback = callback; };
        bool beginDataAcquisition();
        void endDataAcquisition();
        bool resetDataAcquisition();
        void acquireData(bool inCallback);
        void computeFeatures();
        void updateOperationState();
        void streamFeatures();
        void storeData() {}  // TODO => implement
        /***** Debugging *****/
        void exposeAllConfigurations();

    protected:
        char m_macAddress[18];
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
        /***** WiFi *****/
        bool m_wifiConnected;
        /***** Configuration and Mode management *****/
        UsageMode::option m_usageMode;
        AcquisitionMode::option m_acquisitionMode;
        StreamingMode::option m_streamingMode;
        // Static JSON buffer to parse config
        StaticJsonBuffer<1600> jsonBuffer;
        // eg: can hold the following config (remove the space and line breaks)
//        {
//          "features": {
//            "A93": {
//              "STREAM": 1,
//              "OPS": 1,
//              "TRH": [100, 110, 120]
//            },
//            "VAX": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [0.5, 1.2, 1.5]
//            },
//            "VAY": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [0.5, 1.2, 1.5]
//            },
//            "VAZ": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [0.5, 1.2, 1.5]
//            },
//           "TMP": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [40, 80, 120]
//            },
//            "S12": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [500, 100, 1500]
//            },
//            "XXX": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [500, 100, 1500]
//            },
//            "XXX": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [500, 100, 1500]
//            },
//            "XXX": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [500, 100, 1500]
//            },
//            "XXX": {
//              "STREAM": 1,
//              "OPS": 0,
//              "TRH": [500, 100, 1500]
//            }
//          }
//        }
};


/***** Instanciation *****/

extern Conductor conductor;


#endif // CONDUCTOR_H

