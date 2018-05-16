#ifndef CONDUCTOR_H
#define CONDUCTOR_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <MD5.h>

#include "BoardDefinition.h"
#ifdef DRAGONFLY_V03
    #include "InstancesDragonfly.h"
#else
    #include "InstancesButterfly.h"
#endif


/* =============================================================================
    Operation Mode
============================================================================= */

/**
 * Define what type of data we want to acquire / compute
 */
namespace AcquisitionMode
{
    enum option : uint8_t {RAWDATA = 0,
                           FEATURE = 1,
                           NONE    = 2,
                           COUNT   = 3};
}

/**
 * Define the channel through which data will be sent
 */
namespace StreamingMode
{
    enum option : uint8_t {
        WIRED        = 0,       // Send over Serial
        BLE          = 1,       // Send over Bluetooth Low Energy
        WIFI         = 2,       // Send over WiFi
        WIFI_AND_BLE = 3,       // Send over both WiFi and BLE
        STORE        = 4,       // Store in SPI Flash to stream later
        COUNT        = 5};
}


/* =============================================================================
    Operation Presets
============================================================================= */

/**
 * Usage Mode are user controlled, they describe how the device is being used
 */
namespace UsageMode
{
    enum option : uint8_t {CALIBRATION     = 0,
                           EXPERIMENT      = 1,
                           OPERATION       = 2,
                           OPERATION_BIS   = 3,
                           COUNT           = 4};
    // Related default config
    const AcquisitionMode::option acquisitionModeDetails[COUNT] =
    {
        AcquisitionMode::FEATURE,
        AcquisitionMode::RAWDATA,
        AcquisitionMode::FEATURE,
        AcquisitionMode::FEATURE,
    };
}


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
        static const uint32_t defaultAutoSleepDelay = 60000;
        static const uint32_t defaultSleepDuration = 10000;
        static const uint32_t defaultCycleTime = 20000;
        // Raw data publication once per hour by default
        static const uint32_t defaultRawDataPublicationTimer = 3600000;
        static char START_CONFIRM[11];
        static char END_CONFIRM[9];
        // Config handler
        static const uint8_t CONFIG_TYPE_COUNT = 3;
        static IUFlash::storedConfig CONFIG_TYPES[CONFIG_TYPE_COUNT];
        static const uint32_t SEND_CONFIG_CHECKSUM_TIMER = 30000;
        // Default start datetime
        static constexpr double defaultTimestamp = 1524017173.00;
        // Size of Jsn buffr (to parse json)
        static const uint16_t JSON_BUFFER_SIZE = 1600;
        /***** Constructors and destructor *****/
        Conductor(MacAddress macAddress) : m_macAddress(macAddress) { }
        Conductor(const char *macAddress)
            { m_macAddress.fromString(macAddress); }
        virtual ~Conductor() {}
        MacAddress getMacAddress() { return m_macAddress; }
        /***** Hardware & power management *****/
        void sleep(uint32_t duration);
        void suspend(uint32_t duration);
        void manageSleepCycles();
        sleepMode getSleepMode() { return m_sleepMode; }
        uint32_t getAutoSleepDelay() { return m_autoSleepDelay; }
        uint32_t getSleepDuration() { return m_sleepDuration; }
        uint32_t getCycleTime() { return m_cycleTime; }
        /***** Led colors *****/
        void resetLed();
        void overrideLedColor(RGBColor color);
        void showOperationStateOnLed();
        void showStatusOnLed(RGBColor color);
        /***** Local storage (flash) management *****/
        bool configureFromFlash(IUFlash::storedConfig configType);
        void sendConfigChecksum(IUFlash::storedConfig configType);
        void periodicSendConfigChecksum();
        /***** Serial Reading & command processing*****/
        void readFromSerial(StreamingMode::option interfaceType,
                            IUSerial *iuSerial);
        bool processConfiguration(char *json, bool saveToFlash);
        void configureMainOptions(JsonVariant &config);
        void configureAllSensors(JsonVariant &config);
        void configureAllFeatures(JsonVariant &config);
        void processCommands(char *buff);
        void processLegacyCommands(char *buff);
        void processUSBMessages(char *buff);
        void processBLEMessages(char *buff);
        void processUserMessageForWiFi(char *buff,
                                       HardwareSerial *feedbackPort);
        void processWIFIMessages(char *buff);
        /***** Features and groups Management *****/
        void activateFeature(Feature* feature);
        bool isFeatureDeactivatable(Feature* feature);
        void deactivateFeature(Feature* feature);
        void deactivateAllFeatures();
        void activateGroup(FeatureGroup *group);
        void deactivateGroup(FeatureGroup *group);
        void deactivateAllGroups();
        void configureGroupsForOperation();
        void configureGroupsForCalibration();
        /***** Time management *****/
        void setRefDatetime(double refDatetime);
        void setRefDatetime(const char* timestamp);
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
        void sendAccelRawData(uint8_t axisIdx);
        void periodicSendAccelRawData();
        void storeData() {}  // TODO => implement
        /***** Debugging *****/
        void getMCUInfo(char *destination);
        void  streamMCUUInfo(HardwareSerial *port);
        void exposeAllConfigurations();

    protected:
        MacAddress m_macAddress;
        /***** Hardware & power management *****/
        sleepMode m_sleepMode = sleepMode::NONE;
        // Timestamp at which idle phase (or cycle) started for AUTO (or
        // PERIODIC) sleep mode
        uint32_t m_startTime = 0;
        // Duration in which the device must be "IDLE" before entering
        // auto_sleep => Used with "AUTO" sleep mode only
        uint32_t m_autoSleepDelay = defaultAutoSleepDelay;
        // Duration of sleep phase => Used both with "PERIODIC" and
        // "AUTO" sleep modes
        uint32_t m_sleepDuration = defaultSleepDuration;
        // Duration of total cycle (sleep + active) => Used with "PERIODIC"
        // sleep mode only
        uint32_t m_cycleTime = defaultCycleTime;
        /***** Feature management *****/
        FeatureGroup *m_mainFeatureGroup = DEFAULT_FEATURE_GROUP;
        /***** Led colors *****/
        RGBColor m_colorSequence[2];  // Main color, secondary color
        uint32_t m_colorFadeIns[2];   // Main color, secondary color
        uint32_t m_colorDurations[2];   // Main color, secondary color
        /***** Time management *****/
        uint32_t m_lastSynchroTime = 0;
        // last datetime received from bluetooth or wifi
        double m_refDatetime = defaultTimestamp;
        /***** Operations *****/
        OperationState::option m_operationState = OperationState::IDLE;
        void (*m_callback)() = NULL;
        bool m_inDataAcquistion = false;
        uint32_t m_rawDataPublicationTimer = defaultRawDataPublicationTimer;
        uint32_t m_rawDataPublicationStart = 0;
        /***** Configuration and Mode management *****/
        uint32_t m_configTimerStart = 0;
        uint8_t m_nextConfigToSend = 0;
        UsageMode::option m_usageMode = UsageMode::COUNT;
        AcquisitionMode::option m_acquisitionMode = AcquisitionMode::NONE;
        StreamingMode::option m_streamingMode = StreamingMode::COUNT;
};


/* =============================================================================
    Default Thresholds for Accel Energy
============================================================================= */

extern float DEFAULT_ACCEL_ENERGY_NORMAL_TH;
extern float DEFAULT_ACCEL_ENERGY_WARNING_TH;
extern float DEFAULT_ACCEL_ENERGY_HIGH_TH;


/* =============================================================================
    Instanciation
============================================================================= */

extern Conductor conductor;

#endif // CONDUCTOR_H
