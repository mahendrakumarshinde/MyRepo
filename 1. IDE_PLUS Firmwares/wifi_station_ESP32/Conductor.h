#ifndef CONDUCTOR_H
#define CONDUCTOR_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <IUSerial.h>
#include <MacAddress.h>
#include <IUMessage.h>
#include "IUMQTTHelper.h"
#include "IURawDataHelper.h"
#include "IUTimeHelper.h"
#include "MultiMessageValidator.h"
#include "Utilities.h"


/* =============================================================================
    Instanciation
============================================================================= */

extern char hostSerialBuffer[8500];
extern IUSerial hostSerial;

extern IURawDataHelper accelRawDataHelper;

extern IUMQTTHelper mqttHelper;

extern IUTimeHelper timeHelper;


/* =============================================================================
    Conductor
============================================================================= */

class Conductor
{
    public:
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
        static const uint32_t disconnectionTimeout = 100000;  // ms
        // Cyclic publication
        static const uint32_t wifiStatusUpdateDelay = 5000;  // ms
        static const uint32_t wifiInfoPublicationDelay = 300000;  // ms
        // OTA Update in progress, timoue for packet ack from STM
        static const uint32_t otaPktAckTimeout = 15000;  // ms
        static const uint32_t otaPktReadTimeout = 50000; //ms;
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
        bool reconnect(bool forceNewCredentials=false);
        uint8_t waitForConnectResult();
        void checkWiFiDisconnectionTimeout();
        /***** MQTT *****/
        void loopMQTT();
        void processMessageFromMQTT(const char* topic, const char* payload,
                                    uint16_t length);
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
        void updateWiFiStatusCycle();
        /***** Debugging *****/
        void debugPrintWifiInfo();
        /***** get Device Firmware Versions ******/
        void getDeviceFirmwareVersion(char* destination,char* HOST_VERSION, const char* WIFI_VERSION);
        bool getHttpData(bool otaDnldProgress);
        void checkOtaPacketTimeout();       

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
        /***** WiFi credentials and config *****/
        MultiMessageValidator<2> m_credentialValidator;
        char m_userSSID[wifiCredentialLength];
        char m_userPassword[wifiCredentialLength];
        MultiMessageValidator<3> m_staticConfigValidator;
        IPAddress m_staticIp;
        IPAddress m_gateway;
        IPAddress m_subnetMask;
        /***** Cyclic Update *****/
        uint32_t m_lastWifiStatusUpdate;
        uint32_t m_lastWifiInfoPublication;
        /***** Settable parameters (addresses, credentials, etc) *****/
        MultiMessageValidator<2> m_mqttServerValidator;
        IPAddress m_mqttServerIP;
        uint16_t m_mqttServerPort;
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
        char otaStm_uri[512] = "http://iu-firmware.s3.ap-south-1.amazonaws.com/butterfly_firmware.ino.iap";
        char otaEsp_uri[512] = "http://iu-firmware.s3.ap-south-1.amazonaws.com/WiFiClient.ino.bin";
        char ota_uri[512];
        char otaMqttBuff[256];
        HTTPClient http_ota;
        int contentLen = 0;
        int fwdnldLen = 0;
        uint32_t totlen = 0;
        uint32_t pktWaitTimeStr = 0;
        bool otaInProgress = false;
        bool waitingForPktAck = false;
};

#endif // CONDUCTOR_H
