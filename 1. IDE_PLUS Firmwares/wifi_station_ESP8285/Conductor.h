#ifndef CONDUCTOR_H
#define CONDUCTOR_H

#include <Arduino.h>
#include <IUSerial.h>
#include <MacAddress.h>
#include "IUMQTTHelper.h"
#include "IURawDataHelper.h"
#include "IUTimeHelper.h"
#include "MultiMessageValidator.h"
#include "Utilities.h"


/* =============================================================================
    Instanciation
============================================================================= */

extern char hostSerialBuffer[3072];
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
        static const uint32_t wifiConfigReceptionTimeout = 1000;  // ms
        // WiFi will deep-sleep if host doesn't respond before timeout
        static const uint32_t hostResponseTimeout = 60000;  // ms
//        #if IUDEBUG_ANY == 1
//            static const uint32_t hostResponseTimeout = 60000;  // ms
//        #else
//            static const uint32_t hostResponseTimeout = 1000;  // ms
//        #endif
        // Default duration of deep-sleep
        static const uint32_t deepSleepDuration = 2000;  // ms
        /** Connection retry constants **/
        static const uint8_t connectionRetry = 3;
        // Single connection attempt timeout
        static const uint32_t connectionTimeout = 15000;  // ms
        // Delay between 2 connection attemps
        static const uint32_t reconnectionInterval = 1000;  // ms
        // ESP82 will deep-sleep after being disconnected for more than:
        static const uint32_t disconnectionTimeout = 60000;  // ms
        /***** Core *****/
        Conductor();
        virtual ~Conductor() {}
        MacAddress getBleMAC() { return m_bleMAC; }
        void deepsleep(uint32_t duration_ms=deepSleepDuration);
        /***** Communication with host *****/
        void readMessagesFromHost();
        void processMessageFromHost();
        /***** WiFi credentials and config *****/
        void setCredentials(const char *userSSID, const char *userPSK);
        void forgetWiFiCredentials();
        void onNewCredentialsReception();
        void forgetWiFiStaticConfig();
        bool getConfigFromMainBoard();
        /***** Wifi connection and status *****/
        void turnOffRadio(uint32_t duration_us=0);
        void turnOnRadio();
        void disconnectWifi(bool wifiOff=true);
        bool reconnect(bool forceNewCredentials=false);
        uint8_t waitForConnectResult();
        void checkWiFiDisconnectionTimeout();
        /***** MQTT *****/
        void processMessageFromMQTT(char* topic, byte* payload,
                                    uint16_t length);
        /***** Data posting / publication *****/
        bool publishDiagnostic(const char *rawMsg, const uint16_t msgLength,
                               const char *diagnosticType=NULL,
                               const uint16_t diagnosticTypeLength=0);
        bool publishFeature(const char *rawMsg, const uint16_t msgLength,
                            const char *featureType=NULL,
                            const uint16_t featureTypeLength=0);
        /***** Debugging *****/
        void getWifiInfo(char *destination, uint16_t len, bool mqttOn);
        void publishWifiInfo();
        void debugPrintWifiInfo();

    protected:
        /***** Config from Host *****/
        bool m_shouldWakeUp = false;
        MacAddress m_bleMAC;
        MacAddress m_wifiMAC;
        bool m_useMQTT = true;
        /***** Wifi connection and status *****/
        bool m_radioOn = true;
        uint32_t m_lastConnectionAttempt = 0;
        uint32_t m_disconnectionTimerStart = 0;
        uint8_t m_remainingConnectionAttempt = connectionRetry;
        /***** WiFi credentials and config *****/
        MultiMessageValidator<2> m_credentialValidator;
        char m_userSSID[wifiCredentialLength];
        char m_userPassword[wifiCredentialLength];
        MultiMessageValidator<3> m_staticConfigValidator;
        IPAddress m_staticIp;
        IPAddress m_staticGateway;
        IPAddress m_staticSubnet;
        /***** Settable parameters (addresses, credentials, etc) *****/
        MultiMessageValidator<2> m_mqttServerValidator;
        IPAddress m_mqttServerIP = MQTT_DEFAULT_SERVER_IP;
        uint16_t m_mqttServerPort = MQTT_DEFAULT_SERVER_PORT;
        MultiMessageValidator<2> m_mqttCredentialsValidator;
        char m_mqttUsername[MQTT_CREDENTIALS_MAX_LENGTH];
        char m_mqttPassword[MQTT_CREDENTIALS_MAX_LENGTH];
        // HTTP Post endpoints
        char m_featurePostHost[MAX_HOST_LENGTH];
        char m_featurePostRoute[MAX_ROUTE_LENGTH];
        uint16_t m_featurePostPort = DATA_DEFAULT_ENDPOINT_PORT;
        char m_diagnosticPostHost[MAX_HOST_LENGTH];
        char m_diagnosticPostRoute[MAX_ROUTE_LENGTH];
        uint16_t m_diagnosticPostPort = DATA_DEFAULT_ENDPOINT_PORT;

};

#endif // CONDUCTOR_H
