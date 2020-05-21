#ifndef CONDUCTOR_H
#define CONDUCTOR_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <IUSerial.h>
#include <MacAddress.h>
#include <IUMessage.h>
#include <MD5.h>
#include "IUMQTTHelper.h"
#include "IURawDataHelper.h"
#include "IUTimeHelper.h"
#include "MultiMessageValidator.h"
#include "Utilities.h"
//#include "SPIFFS.h"
#include "IUESPFlash.h"


/* =============================================================================
    Instanciation
============================================================================= */

extern char hostSerialBuffer[8500];
extern IUSerial hostSerial;

extern IURawDataHelper accelRawDataHelper;

extern IUMQTTHelper mqttHelper;

extern IUTimeHelper timeHelper;

extern IUESPFlash iuWiFiFlash;

#define OTA_STM_PKT_ACK_TMOUT       1000
#define OTA_DATA_READ_TIMOUT        1001
#define OTA_HTTP_INIT_FAIL          1002
#define OTA_WIFI_DISCONNECT         1003
#define OTA_INVALID_MAIN_FW_SIZE    1004
#define OTA_INVALID_WIFI_FW_SIZE    1005

// Certificate download error Codes
#define CERT_DOWNLOAD_COMPLETE              2000
#define CERT_UPGRADE_COMPLETE               2001
#define CERT_DOWNLOAD_FAILED                2003
#define CERT_FILES_NOT_PRESENT              2004
#define CERT_INVALID_CONFIG_JSON            2005
#define CERT_INVALID_KEY_CHECKSUM           2006
#define CERT_INVALID_CRT_CHECKSUM           2007
#define CERT_INVALID_ROOTCA_CHECKSUM        2008
#define CERT_DOWNLOAD_CRT_FAILED            2009
#define CERT_DOWNLOAD_KEY_FAILED            2010
#define CERT_DOWNLOAD_ROOTCA_FAILED         2011
#define CERT_CRT_FILE_NOT_PRESENT           2012
#define CERT_KEY_FILE_NOT_PRESENT           2013
#define CERT_ROOTCA_FILE_NOT_PRESENT        2014
#define CERT_CONFIG_JSON_FILE_NOT_PRESENT   2015
#define CERT_STATIC_URL_FILE_NOT_PRESENT    2016
#define CERT_FILESYSTEM_READ_WRITE_FAILED   2017
#define CERT_UPGRADE_START                  2018
#define CERT_UPGRADE_FAILED                 2019
#define CERT_NEW_CERT_NOT_AVAILBLE          2020
#define CERT_SAME_UPGRADE_CONFIG_RECEIVED   2021

#define MQTT_CONNECTION_ATTEMPT_FAILED      2022

#define MAX_HTTP_INIT_RETRY     3

#define MAX_MAIN_FW_SIZE        634880 // 0x9B000‬ // 620 KB
#define MAX_WIFI_FW_SIZE        1572864 // 0x180000‬ // 1.5 MB
/* =============================================================================
    Conductor
============================================================================= */

class Conductor
{
    public:
        String IpAddress2String(const IPAddress& ipAddress);
        /***** Public constants *****/
        // Max expected length of WiFi SSID or password
        static const uint8_t wifiCredentialLength = 64;
        // WiFi config (credentials or Static IP) MultiMessageValidator timeout
        static const uint32_t wifiConfigReceptionTimeout = 5000;  // ms
        // WiFi will deep-sleep if host doesn't respond before timeout
        static const uint32_t hostResponseTimeout = 60000;  // ms
        // Default duration of deep-sleep
        static const uint32_t deepSleepDuration = 2000;  // ms
        // MQTT connection info request to host delay
        static const uint32_t mqttInfoRequestDelay = 1000;  // ms
        /** Connection retry constants **/
        // Single connection attempt timeout
        static const uint32_t connectionTimeout = 30000;  // ms
        // Delay between 2 connection attemps
        static const uint32_t reconnectionInterval = 1000;  // ms
        //ESP32 will deep-sleep after being disconnected for more than:
        static const uint32_t disconnectionTimeout = 120000;  // ms
        // Cyclic publication
        static const uint32_t wifiStatusUpdateDelay = 5000;  // ms
        static const uint32_t wifiInfoPublicationDelay = 300000;  // ms
        // OTA Update in progress, timoue for packet ack from STM
        static const uint32_t otaPktAckTimeout = 30000;  // ms
        static const uint32_t otaPktReadTimeout = 50000; //ms;
        static const uint32_t otaHttpTimeout = 60000; //ms;
        static const uint8_t maxMqttClientConnectionCount = 5;
        static const uint8_t maxMqttCertificateDownloadCount = 3;
        static const uint32_t downloadInitRetryTimeout  = 30*1000;   //ms
        /***** Core *****/
        Conductor();
        virtual ~Conductor() {}
        MacAddress getBleMAC() { return m_bleMAC; }
        void setBleMAC(MacAddress hostBLEMac);
        void setBleMAC(const char *hostBLEMac);
        void deepsleep(uint32_t duration_ms=deepSleepDuration);
        /***** Communication with host *****/
        void processHostMessage(IUSerial *iuSerial);
        /***** WiFi credentials and config *****/
        void forceWiFiConfig(const char *userSSID, const char *userPSK,
                             IPAddress staticIp, IPAddress gateway,
                             IPAddress subnetMask);
        void forgetWiFiCredentials();
        void receiveNewCredentials(char *newSSID, char *newPSK);
        void forgetWiFiStaticConfig();
        void receiveNewStaticConfig(IPAddress ip, uint8_t idx);
        bool getConfigFromMainBoard();
        /***** Wifi connection and status *****/
        void disconnectWifi(bool wifiOff=true);
        void disconnectMQTT();
        bool reconnect(bool forceNewCredentials=false);
        uint8_t waitForConnectResult();
        void checkWiFiDisconnectionTimeout();
        /***** Mqtt Connection and status, timeouts ******/
        void checkMqttDisconnectionTimeout();
        void mqttSecureConnect();
        void upgradeSuccess();
        void upgradeFailed();
        /***** MQTT *****/
        void loopMQTT();
        void processMessageFromMQTT(const char* topic, const char* payload,
                                    uint16_t length);
        /***** Autherization ***********/
        String setBasicHTTPAutherization();
        void removeCharacterFromString(char* inputString, int charToRemove);
        void reverseString(char* username);
        /***** Data posting / publication *****/
        bool publishDiagnostic(const char *rawMsg, const uint16_t msgLength,
                               const char *diagnosticType=NULL,
                               const uint16_t diagnosticTypeLength=0);
        bool publishFeature(const char *rawMsg, const uint16_t msgLength,
                            const char *featureType=NULL,
                            const uint16_t featureTypeLength=0);
        /***** Cyclic Update *****/
        void getWifiInfo(char *destination, uint16_t len, bool mqttOn);
        void publishWifiInfo();
        void publishWifiInfoCycle();
        void updateWiFiStatus();
        void updateMQTTStatus();
        void updateWiFiStatusCycle();
        void autoReconncetWifi();
        void publishRSSI();   // 30Sec
        /***** Debugging *****/
        void debugPrintWifiInfo();
        /***** get Device Firmware Versions ******/
        void getDeviceFirmwareVersion(char* destination,char* HOST_VERSION, const char* WIFI_VERSION);
        bool otaDnldFw(bool otaDnldProgress);
        void checkOtaPacketTimeout();
        String getRca(int error);
        /****** Certificate download managment ******/
        bool getDeviceCertificates(IUESPFlash::storedConfig configType, const char* type,const char* url);
        bool writeCertificatesToFlash(IUESPFlash::storedConfig configType,long fileSize,const char* type);
        void readCertificatesFromFlash(IUESPFlash::storedConfig configType,const char* type);
        int download_tls_ssl_certificates();
        char* getConfigChecksum(IUESPFlash::storedConfig configType);
        void updateDiagnosticEndpoint(char* diagnosticEndpoint,int length);
        bool setCommonHttpEndpoint();
        void configureDiagnosticEndpointFromFlash(IUESPFlash::storedConfig configType);
        void publishedDiagnosticMessage(char* buffer,int bufferLength);
        void resetDownloadInitTimer(uint16_t downloadTriggerTime,uint16_t loopTimeout);    //(sec,ms)
        int downloadCertificates(const char* type,const char* url,const char* hash,uint8_t index,uint8_t certToUpdate);
        /******* Validation ********/
        void messageValidation(char* json);
        /***************************/
        char mqtt_client_cert[2048];
        char mqtt_client_key[2048];
        char ssl_rootca_cert[2048];
        char eap_client_cert[2048];
        char eap_client_key[2048];
        char certDownloadResponse[2400];    // stores the cert download json (Actual  -2299)
        char diagnosticEndpointHost[MAX_HOST_LENGTH];
        int  diagnosticEndpointPort;
        char diagnosticEndpointRoute[MAX_ROUTE_LENGTH];
        bool configStatus = false;
        bool certificateDownloadInProgress=false;
        bool certificateDownloadInitInProgress = false;
        bool certDownloadInitAck = false;
        bool newMqttcertificateAvailable = false;
        bool newMqttPrivateKeyAvailable = false;
        bool newRootCACertificateAvailable = false;
        bool newEapCertificateAvailable = false;
        bool newEapPrivateKeyAvailable = false;
        bool downloadInitTimer = true;
        bool downloadAborted = false;
        bool upgradeReceived = false;
        bool initialFileDownload = false;
        uint32_t downloadInitLastTimeSync;
        uint8_t certificateDownloadStatus = 0;
        uint8_t newDownloadConnectonAttempt = 0;
        uint8_t activeCertificates = 0;
        bool m_statementEntry = true;
        bool commomEndpointsuccess = false;
        // Config handler
        static const uint8_t CONFIG_TYPE_COUNT = 10;
        static IUESPFlash::storedConfig CONFIG_TYPES[CONFIG_TYPE_COUNT];
        void connectToWiFi();
        void updateWiFiConfig(char* config,int length);
        void setWiFiConfig();
    protected:
        /***** Config from Host *****/      
        char HOST_FIRMWARE_VERSION[8];      //filled when the ESP starts or when it connects to MQTT broker
        int HOST_SAMPLING_RATE;
        int HOST_BLOCK_SIZE;
        MacAddress m_bleMAC;
        MacAddress m_wifiMAC;
        bool m_useMQTT;
        uint32_t m_lastMQTTInfoRequest;
        /***** Wifi connection *****/
        uint32_t m_lastConnectionAttempt;
        uint32_t m_disconnectionTimerStart;
        /***** MQTT connection *****/
        uint32_t m_lastConnectionAttemptMqtt;
        uint32_t m_disconnectionMqttTimerStart;
        /***** WiFi credentials and config *****/
        MultiMessageValidator<2> m_credentialValidator;
        char m_userSSID[wifiCredentialLength];
        char m_userPassword[wifiCredentialLength];
        char m_username[wifiCredentialLength];
        MultiMessageValidator<3> m_staticConfigValidator;
        IPAddress m_staticIp;
        IPAddress m_gateway;
        IPAddress m_subnetMask;
        IPAddress m_dns1;
        IPAddress m_dns2;
        /***** Cyclic Update *****/
        uint32_t m_lastWifiStatusUpdate;
        uint32_t m_lastWifiStatusCheck;
        uint32_t m_lastWifiInfoPublication;
        /***** Settable parameters (addresses, credentials, etc) *****/
        MultiMessageValidator<2> m_mqttServerValidator;
        // IPAddress m_mqttServerIP;
        char m_mqttServerIP[IUMQTTHelper::credentialMaxLength];
        uint16_t m_mqttServerPort;
        bool m_tls_enabled;
        char httpBuffer[8235];              //maximum possible buffer size (when blockSize = 4096)
        MultiMessageValidator<2> m_mqttCredentialsValidator;
        char m_mqttUsername[IUMQTTHelper::credentialMaxLength];
        char m_mqttPassword[IUMQTTHelper::credentialMaxLength];
        // HTTP Post endpoints
        char m_featurePostHost[MAX_HOST_LENGTH];
        char m_featurePostRoute[MAX_ROUTE_LENGTH];
        uint16_t m_featurePostPort;
        char m_diagnosticPostHost[MAX_HOST_LENGTH];
        char m_diagnosticPostRoute[MAX_ROUTE_LENGTH];
        uint16_t m_diagnosticPostPort;
        //HTTP POST metadata
        const int macIdSize = 18;
        const int hostFirmwareVersionSize = 8;
        const int timestampSize = 8;
        const int blockSizeSize = 4;
        const int samplingRateSize = 4;
        const int axisSize = 1; 
        double timestamp;
        char otaStm_uri[512];
        char otaEsp_uri[512];
        char ota_uri[512];
        HTTPClient http_ota;
        uint32_t contentLen = 0;
        uint32_t fwdnldLen = 0;
        uint32_t totlen = 0;
        uint32_t pktWaitTimeStr = 0;
        bool otaInProgress = false;
        bool waitingForPktAck = false;
        bool otaStsDataSent = false;
        bool otaInitTimeoutFlag = false;
        uint32_t otaInitTimeout = 0;
        char m_wifiAuthType[wifiCredentialLength];
};

#endif // CONDUCTOR_H
