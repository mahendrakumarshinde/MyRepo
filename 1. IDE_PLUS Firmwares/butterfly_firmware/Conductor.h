#ifndef CONDUCTOR_H
#define CONDUCTOR_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <MD5.h>
#include <IUMessage.h>

#include "BoardDefinition.h"
#ifdef DRAGONFLY_V03
    #include "InstancesDragonfly.h"
#else
    #include "InstancesButterfly.h"
#endif

#include "SegmentedMessage.h"
#define MAX_SEGMENTED_MESSAGES 5
#define FLASH_ERROR        1
#define FLASH_SUCCESS      0

#define DEVICE_DIAG_DOSFS_ERR1    0x0001
#define DEVICE_DIAG_DOSFS_ERR2    0x0002
#define DEVICE_DIAG_DOSFS_ERR3    0x0004
#define DEVICE_DIAG_DOSFS_ERR4    0x0008
#define DEVICE_DIAG_DOSFS_ERR5    0x0010
#define DEVICE_DIAG_DOSFS_ERR6    0x0020
#define DEVICE_DIAG_DOSFS_ERR7    0x0040
#define DEVICE_DIAG_MQTT_ERR1     0x0080
#define DEVICE_DIAG_MQTT_ERR2     0x0100
#define DEVICE_DIAG_HTTP_ERR1     0x0200
#define DEVICE_DIAG_HTTP_ERR2     0x0400
#define DEVICE_DIAG_PHASE_ERR1    0x0800
#define DEVICE_DIAG_SENS_ERR1     0x1000
#define DEVICE_DIAG_FING_ERR1     0x2000
#define DEVICE_DIAG_DIAG_ERR1     0x4000
#define DEVICE_DIAG_VIBR_ERR1     0x8000

//#define DEV_DIAG_MSG_RTRY   3

// #define DEVID_MODE1         1  // Device ID:BMD,APP:BMD
// #define DEVID_MODE2         2  // Device ID:WIFI,APP:NO
// #define DEVID_MODE3         3  // Device ID:WIFI,APP:BMD

#define DEVICE_DIAG_BMD_OK        0
#define DEVICE_DIAG_GET_OK        100

#define DEVICE_DIAG_BMD_ERR1      1
#define DEVICE_DIAG_BMD_ERR2      2
#define DEVICE_DIAG_BMD_ERR3      3
#define DEVICE_DIAG_BMD_ERR4      4
#define DEVICE_DIAG_BMD_ERR5      5
#define DEVICE_DIAG_BMD_ERR6      6
#define DEVICE_DIAG_BMD_ERR7      7
#define DEVICE_DIAG_SETUP_ERR1    8
#define DEVICE_DIAG_STMMEM_ERR    9

#define DEVICE_DIAG_WIFI_ERR1      0x0001
#define DEVICE_DIAG_WIFI_ERR2      0x0002
#define DEVICE_DIAG_WIFI_ERR3      0x0004
#define DEVICE_DIAG_WIFI_ERR4      0x0008
#define DEVICE_DIAG_WIFI_ERR5      0x0010
#define DEVICE_DIAG_WIFI_ERR6      0x0020
#define DEVICE_DIAG_WIFI_ERR7      0x0040
#define DEVICE_DIAG_WIFI_ERR8      0x0080

#define DEVICE_DIAG_WIFI_STS1      0x0100
#define DEVICE_DIAG_WIFI_STS2      0x0200
#define DEVICE_DIAG_WIFI_STS3      0x0400
#define DEVICE_DIAG_WIFI_STS4      0x0800
#define DEVICE_DIAG_WIFI_STS5      0x1000
#define DEVICE_DIAG_WIFI_STS6      0x2000
#define DEVICE_DIAG_WIFI_STS7      0x4000
#define DEVICE_DIAG_WIFI_STS8      0x8000

#define MAX_SYNC_COUNT            20

/* Timeout for MQTT_DISCONNECTED */
#define MQTT_DISCONNECTION_TIMEOUT 900000               //900000 //15min

#define CONFIG_REQUEST_TIMEOUT   14400000      // 4 Hrs

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
        NONE         = 0,
        WIRED        = 1,       // Send over Serial
        BLE          = 2,       // Send over Bluetooth Low Energy
        WIFI         = 3,       // Send over WiFi
        WIFI_AND_BLE = 4,       // Send over both WiFi and BLE
        STORE        = 5,       // Store in SPI Flash to stream later
        ETHERNET     = 6,       // Send over ETHERNET
        COUNT        = 7};
}


/* =============================================================================
    Operation Presets
============================================================================= */

/**
 * Usage Mode are user controlled, they describe how the device is being used
 */
namespace UsageMode
{
   enum option :  uint8_t  {CALIBRATION    = 0,
                           EXPERIMENT      = 1,
                           OPERATION       = 2,
                           OPERATION_BIS   = 3,
                           CUSTOM          = 4,
                           OTA             = 5,
                           COUNT           = 6};
    // Related default config
    const AcquisitionMode::option acquisitionModeDetails[COUNT] =
    {
        AcquisitionMode::FEATURE,
        AcquisitionMode::RAWDATA,   // for Experiment
        AcquisitionMode::FEATURE,
        AcquisitionMode::FEATURE,
        AcquisitionMode::RAWDATA     // for CUSTOM 
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
        uint32_t lastTimeSync = 0;
        uint32_t lastConfigRequest = 0;
        char ack_config[200];
        MacAddress m_macAddress;
        /***** Preset values and default settings *****/
        enum sleepMode : uint8_t {NONE     = 0,
                                  AUTO     = 1,
                                  PERIODIC = 2,
                                  COUNT    = 3};
        enum SensorStatusCode : uint8_t {LSM_SET = 0,
                                        KNX_SET = 1,
                                        LSM_ABS = 2,
                                        KNX_ABS = 3,
                                        KNX_DEFAULT = 4,
                                        LSM_DEFAULT = 5,
                                        SEN_ABS = 6,
                                        LSM_AUTO_GRANGE = 7,
                                        KNX_AUTO_GRANGE = 8
                                        };


        enum publish : uint8_t {
                ALERT_POLICY        = 0,
                STREAM              = 1      // Send over Serial
                };
        static const uint32_t defaultAutoSleepDelay = 60000;
        static const uint32_t defaultSleepDuration = 10000;
        static const uint32_t defaultCycleTime = 20000;
        // Raw data publication once per hour by default
        static const uint32_t defaultRawDataPublicationTimer = 1800000;
        // Usage Modes
        static char START_CONFIRM[11];
        static char END_CONFIRM[9];
        // Config handler
        static const uint8_t CONFIG_TYPE_COUNT = 2;
        static IUFlash::storedConfig CONFIG_TYPES[CONFIG_TYPE_COUNT];
        static const uint32_t SEND_CONFIG_CHECKSUM_TIMER = 30000;
        // Default start datetime
        static constexpr double defaultTimestamp = 1524017173.00;
        // Size of Jsn buffr (to parse json)
        static const uint16_t JSON_BUFFER_SIZE = 1024; //Changed from 1600
        // static const uint32_t BLEconnectionTimeout = 60000;
        static const uint32_t BLEconnectionTimeout = 15000;
        static const uint32_t connectedStatusTimeout = 60000;   // 1 min for ETHERNET connectedStatusTimeout
        uint32_t m_connectionTimeout = 150000;   // 2 min 30s
        uint32_t configRequestTimeout = CONFIG_REQUEST_TIMEOUT;//1800000;   //timeout 30min
        uint32_t m_upgradeMessageTimeout = 30*1000;
        uint32_t m_certDownloadInitTimeout = 60*1000;
        uint32_t m_certDownloadConfigTimeout = 30*1000;
        // Modbus Connection Timeouts 
        const uint16_t modbusConnectionTimeout = 5000;   // ms 
        // Diagnostic Rule Engine published buffers
        static const uint32_t DIG_PUBLISHED_BUFFER_SIZE = 2000;
        char m_diagnosticPublishedBuffer[DIG_PUBLISHED_BUFFER_SIZE+70]; // 70 bytes for MACID, TIMESTMP, DIGRES
        char m_diagnosticResult[DIG_PUBLISHED_BUFFER_SIZE];
         // Spectral Features buffers and Timeouts
        bool m_spectralFeatureBackupComsumed = true;
        static const int MAX_SPECTRAL_FEATURE_COUNT = 10;
        char* spectralFeaturesKeys[MAX_SPECTRAL_FEATURE_COUNT];
        String m_spectralFeatureResult;
        int spectralState;
        int checkFingerprintsState();
        bool spectralStateSuccess;
        uint32_t lastUpdated = 0;
        uint32_t digLastPublish = 0;
        uint32_t fresLastPublish = 0;
        uint16_t reportableDIGLength = 0;
        //timer ISR period
        uint16_t timerISRPeriod = 300; // default 3.3KHz
        String availableFingerprints;
        bool sendConfigReqOnBoot = true;
        bool modbusStreamingMode =false;
        bool ready_to_publish_to_modbus = false;
        bool certDownloadInProgress = false;
        bool certDownloadMode = false;
        bool sendCertInitAck = false;
        bool requestConfig = false;
        bool spectralFeatures_ready_to_publish = false;
        uint8_t getm_id(char* did, int totalConfiguredDiag);
        static const uint8_t maxDiagnosticStates = 10;
        float modbus_reportable_m_id[maxDiagnosticStates];
        uint32_t currTime=0;
        /***** Core *****/
        Conductor() {};
        Conductor(MacAddress macAddress) : m_macAddress(macAddress) { }
        Conductor(const char *macAddress)
            { m_macAddress.fromString(macAddress); }
        virtual ~Conductor() {}
        MacAddress getMacAddress() { return m_macAddress; }
        void setMacAddress(MacAddress macAddress) { m_macAddress = macAddress; }
        /***** Hardware & power management *****/
        void sleep(uint32_t duration);
        void suspend(uint32_t duration);
        void manageSleepCycles();
        sleepMode getSleepMode() { return m_sleepMode; }
        uint32_t getAutoSleepDelay() { return m_autoSleepDelay; }
        uint32_t getSleepDuration() { return m_sleepDuration; }
        uint32_t getCycleTime() { return m_cycleTime; }
        /***** Local storage (flash) management *****/
        bool configureFromFlash(IUFlash::storedConfig configType);
        String sendConfigChecksum(IUFlash::storedConfig configType, JsonObject &inputConfig);
        void periodicSendConfigChecksum();
        bool setThresholdsFromFile();
        /***** Serial Reading & command processing*****/
        bool processConfiguration(char *json, bool saveToFlash);
        void configureMainOptions(JsonVariant &config);
        void configureAllSensors(JsonVariant &config);
        void configureAllFeatures(JsonVariant &config);
        void processCommand(char *buff);
        void processUserCommandForWiFi(char *buff,
                                       IUSerial *feedbackSerial);
        void processLegacyCommand(char *buff);
        void processUSBMessage(IUSerial *iuSerial);
        void processBLEMessage(IUSerial *iuSerial);
        void processWiFiMessage(IUSerial *iuSerial);
        void processJSONmessage(const char * buff);
        /***** Features and groups Management *****/
        void activateFeature(Feature* feature);
        bool isFeatureDeactivatable(Feature* feature);
        void deactivateFeature(Feature* feature);
        void activateGroup(FeatureGroup *group);
        void deactivateGroup(FeatureGroup *group);
        void deactivateAllGroups();
        void configureGroupsForOperation();
        void configureGroupsForCalibration();
        void changeMainFeatureGroup(FeatureGroup *group);
        /***** Time management *****/
        void setRefDatetime(double refDatetime);
        void setRefDatetime(const char* timestamp);
        double getDatetime();
        /***** Mode management *****/
        bool isBLEConnected();
        void changeAcquisitionMode(AcquisitionMode::option mode);
        void updateStreamingMode();
        void changeUsageMode(UsageMode::option usage);
        UsageMode::option getUsageMode() { return m_usageMode ;}
        /***** Operations *****/
        void setCallback(void (*callback)()) { m_callback = callback; };
        bool beginDataAcquisition();
        void endDataAcquisition();
        bool resetDataAcquisition();
        void acquireData(bool inCallback);
        void acquireAudioData(bool inCallback);
        void acquireTemperatureData();
        void computeFeatures();
        /**** Diagnostic Rule Engine ******/
        void computeTriggers();
        void streamDiagnostics();
        void constructPayload(const char* dId,JsonObject& desc);
        void addFTR(const char* dId,JsonArray& FTR,uint8_t id );
        void addFTR(JsonArray& FTR,uint8_t id );
        /*********************************/
        void streamFeatures();
        void sendAccelRawData(uint8_t axisIdx);
        void periodicSendAccelRawData();
        void storeData() {}  // TODO => implement
        bool setFFTParams();
        bool configureRPM(JsonVariant &config);
        /***** Debugging *****/
        void getMCUInfo(char *destination);
        void  streamMCUUInfo(HardwareSerial *port);
        void exposeAllConfigurations();
        // mqtt / http configuration
        // void fastSwap (const char **i, const char **d);
        void updateDeviceInfo(uint8_t devIdMode);
        bool checkdeviceIdFlash();
        bool getdeviceIdFlash(uint8_t *idMode);
        void setDeviceIdMode(bool status);
        void checkDeviceDiagMsg();
        void sendDeviceDiagMsg(int diagErrCode, char *statusMsg);
        void readMQTTendpoints();
        bool readHTTPendpoints();
        // void configureMQTTServer(String filename);
        bool configureBoardFromFlash(String filename,bool isSet);
        JsonObject& configureJsonFromFlash(String filename,bool isSet);
        void sendDiagnosticFingerPrints();
        void resetBLEonTimeout();
        void setConductorMacAddress();
        void printConductorMac();
        /***** Segmented Messages *****/
        void extractPayloadFromSegmentedMessage(const char* segment, char* payload);
        bool checkMessageActive(int messageID);
        void processSegmentedMessage(const char* buff);
        bool checkAllSegmentsReceived(int messageID);
        void compileSegmentedMessage(int messageID);
        void computeSegmentedMessageHash(int messageID);
        bool consumeReadySegmentedMessage(char* returnMessage);
        void cleanSegmentedMessage(int messageID);
        void cleanTimedoutSegmentedMessages();
        void cleanConsumedSegmentedMessages();
        void cleanFailedSegmentedMessage(int messageID);
        void sendSegmentedMessageResponse(int messageID);

        //bool setSensorConfig(char* filename);
        bool setSensorConfig(JsonVariant &config);
        bool setEthernetConfig(char* filename);
        uint8_t processWiFiRadioModes(char* buff);
        /***** HTTP raw data sending 
         * problems encountered: 
         * 1. Raw buffers are of datatype char, need to handle copying memory
         * with care.
         * (Ideal solution)change datatype of raw buffers to q15's? or 
         * (second solution) handle moving memory explicitly?
         * Tried 1st solution - getting compilation errors, implemented for second solution
         * TODO : Implement first solution in optimization pass
        */
        void rawDataRequest();
        void manageRawDataSending();
        // void startRawDataSendingSession();
        void prepareRawDataPacketAndSend(char axis);       // to send to ESP
        int httpsStatusCodeX, httpsStatusCodeY, httpsStatusCodeZ;   
        int httpsOEMStatusCodeX, httpsOEMStatusCodeY, httpsOEMStatusCodeZ;   
        bool sendNextAxis = false;      
        bool XSentToWifi, YsentToWifi, ZsentToWifi;     // TODO optimize using bit vector
        bool XrecByWifi,YrecByWifi,ZrecByWifi;
        uint32_t RawDataTimeout = 0;
        uint32_t RawDataTotalTimeout = 0;
        double rawDataRecordedAt, lastPacketSentToESP;
        IUMessageFormat::rawDataPacket rawData;
        void prepareFFTMetaData();      //prepareFFTMetadata()
        int gRange_metaData;      //currentgRange_metaData
        //Send Sensor error codes
        void setSensorStatus(SensorStatusCode errorCode);
        void sendSensorStatus();
        
        void otaChkFwdnldTmout();
        uint32_t firmwareValidation();
        uint8_t firmwareConfigValidation(File *ValidationFile);
        uint8_t firmwareDeviceValidation(File *ValidationFile);
        uint8_t firmwareWifiValidation(File *ValidationFile);
        void sendOtaStatusMsg(MSPCommand::command type, char *msg, const char *errMsg);
        void readOtaConfig();
        void readForceOtaConfig();
        void getOtaStatus();
        void sendOtaStatus();
        void otaFWValidation();
        static const uint32_t fwDnldStartTmout = 60000;
        uint32_t otaFwdnldTmout = 0;
        bool waitingDnldStrart = false;
        void sendFlashStatusMsg(int flashStatus, char *deviceStatus);
        bool flashStatusFlag = false;
        void periodicFlashTest();
        void onBootFlashTest();
        float* getFingerprintsforModbus();
        void getFingerprintsforFRES(const char* input);
        bool checkforModbusSlaveConfigurations();
        bool updateModbusStatus();
        void checkforWiFiConfigurations();
        void removeChar(char * New_BLE_MAC_Address, int charToRemove);
        void setDefaultMQTT();
        void setDefaultHTTP();
        void updateWiFiHash();
        void updateMQTTHash();
        void updateHTTPHash();
        void updateCertHash();
        void updateDiagCertHash();
        void sendConfigRequest();
        /**** Diagnostic Rule Engine *****/
        void computeDiagnoticState(String *diagInput, int totalConfiguredDiag);
        void configureAlertPolicy();
        void clearDiagStateBuffers();
        void clearDiagResultArray();
        int getTotalDigCount(const char* diagName);
        int getActiveDigCount(const char* diagName);
        
        char* GetStoredMD5(IUFlash::storedConfig configType, JsonObject &inputConfig);
        JsonObject& createFeatureGroupjson();
        void mergeJson(JsonObject& dest, const JsonObject& src);
        bool validTimeStamp();
        void checkPhaseConfig();
        void computeAdvanceFeature();
        void addAdvanceFeature(JsonObject& destJson, uint8_t index , String* id, float* value);
        bool isWifiConnected() { return m_wifiConnected; }
        bool isJsonKeyPresent(JsonObject &config,char* key);
        static const uint8_t max_IDs = 10;
        String m_phase_ids[max_IDs];
        uint32_t m_devDiagErrCode = 0;        
        bool devIdbmdWifi = false;
        uint8_t syncLostCount = 0;
        void selfFwUpgradeInit();
        void mqttReset(bool timerflag);
        uint32_t devResetTime;
        uint16_t m_wifiDiagErrCode = 0;
    #ifdef DEVIDFIX_TESTSTUB
        uint8_t flagval2 = 0;
    #endif
    
    protected:
        //MacAddress m_macAddress;
        MacAddress m_macAddressBle;
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
        /***** Time management *****/
        uint32_t m_lastSynchroTime = 0;
        // last datetime received from bluetooth or wifi
        double m_refDatetime = defaultTimestamp;
        uint32_t m_lastBLEmessage = 0;
        /***** Operations *****/
        void (*m_callback)() = NULL;
        bool m_inDataAcquistion = false;
        uint32_t m_rawDataPublicationTimer = defaultRawDataPublicationTimer;
        uint32_t m_rawDataPublicationStart = 0;
        /***** Configuration and Mode management *****/
        uint32_t m_configTimerStart = 0;
        uint8_t m_nextConfigToSend = 0;
        UsageMode::option m_usageMode = UsageMode::COUNT;
        AcquisitionMode::option m_acquisitionMode = AcquisitionMode::NONE;
        StreamingMode::option m_streamingMode = StreamingMode::NONE;
        const char* m_mqttServerIp = MQTT_DEFAULT_SERVER_IP;
        uint16_t m_mqttServerPort = MQTT_DEFAULT_SERVER_PORT;
        const char* m_mqttUserName = MQTT_DEFAULT_USERNAME;
        const char* m_mqttPassword = MQTT_DEFAULT_PASSWORD;
        bool m_connectionType = false;
        // bool m_tls_enabled = MQTT_DEFAULT_TLS_FLAG;
        //httpendpoint configuration
        const char* m_httpHost  = HTTP_DEFAULT_HOST;
        uint16_t  m_httpPort  = HTTP_DEFAULT_PORT;
        const char* m_httpPath = HTTP_DEFAULT_PATH;
        const char* m_httpUsername = HTTP_DEFAULT_USERNAME;
        const char* m_httpPassword = HTTP_DEFAULT_PASSWORD;
        const char* m_httpOauth = HTTP_DEFAULT_OUTH;

        const char* m_httpHost_oem;
        uint16_t  m_httpPort_oem;
        const char* m_httpPath_oem;
        const char* m_httpUsername_oem;
        const char* m_httpPassword_oem;
        const char* m_httpOauth_oem;
        bool httpOEMConfigPresent = false;
        
        const char* m_accountId;
        bool httpOtaValidation = false;
        double last_fingerprint_timestamp = 0;
        bool computed_first_fingerprint_timestamp = false;
        SegmentedMessage segmentedMessages[MAX_SEGMENTED_MESSAGES]; // atmost MAX_SEGMENTED_MESSAGES can be captured in interleaved manner
        char status[50];
        SensorStatusCode statusCode;
        char m_otaStmUri[512];
        char m_otaEspUri[512];
        char stmHash[34];
        char espHash[34];
        char m_type1[8];
        char m_type2[8];
        char m_otaMsgId[32];
        char m_otaMsgType[16];
        char m_otaFwVer[16];
        char m_deviceType[16];
        char m_deviceId[32];
        char m_rlbkMsgId[32];
        char m_rlbkFwVer[16];
        char fwBinFileName[32];
        MacAddress m_rlbkDevId;
        bool m_rlbkDowngrade = false;
        bool otaSendMsg = false;
        bool doOnceFWValid = false;
        bool otaConnectionMode = false;
        int FWValidCnt = 0;
        char FW_Valid_State = 0;
        uint32_t otaInitWaitTimeout = 0;
        bool otaInitTimeoutFlag = false;
        char WiFiDisconnect_OTAErr[16];
        //char ack_config[200];
        uint32_t certDownloadInitWaitTimeout =0;
        uint32_t certDownloadConfigTimeout = 0;
        uint32_t m_downloadSuccessStartTime = 0;
        bool m_getDownloadConfig = false;
        bool m_certDownloadStarted  = false;
        bool m_downloadSuccess = false;
        bool m_upgradeSuccess = false;
        bool m_mqttConnected = false;
        uint32_t dataSendingPeriod;
        // Certificates buffers
        char m_certType[15];
        char m_keyType[15];
        char m_certHash[34];
        char m_keyHash[34];
        bool m_wifiConnected = false;
        uint32_t last_active[maxDiagnosticStates];
        uint32_t first_active[maxDiagnosticStates];
        uint32_t last_alert[maxDiagnosticStates];
        bool first_active_flag[maxDiagnosticStates];
        bool last_active_flag[maxDiagnosticStates];
        bool last_alert_flag[maxDiagnosticStates];
        bool reset_alert_flag[maxDiagnosticStates];
        bool alert_repeat_state[maxDiagnosticStates];
        uint16_t m_minSpan[maxDiagnosticStates];
        uint16_t m_aleartRepeat[maxDiagnosticStates];
        uint16_t m_maxGap[maxDiagnosticStates];
        uint16_t m_totalDigCount[maxDiagnosticStates];
        uint16_t m_activeDigCount[maxDiagnosticStates];
        uint8_t reportableDIGID[maxDiagnosticStates];
        uint8_t reportableIndexCounter;
        char* diagAlertResults[maxDiagnosticStates];
        uint32_t diagStreamingPeriod = 5000; // in milli seconds
        uint32_t fresPublishPeriod = 5000;
        bool digStream = true;
        bool fresStream = true;
        
        size_t totalIDs;
        char m_ax1[max_IDs];
        char  m_ax2[max_IDs];
        uint8_t m_trh[max_IDs];
       // float phase_output[max_IDs];
        uint8_t m_id[maxDiagnosticStates];
        const char* d_id[maxDiagnosticStates];
        uint8_t reportable_m_id[maxDiagnosticStates];
        
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
