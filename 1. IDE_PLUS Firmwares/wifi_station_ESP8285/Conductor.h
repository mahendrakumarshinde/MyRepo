#ifndef CONDUCTOR_H
#define CONDUCTOR_H

#include <Arduino.h>
#include "IUSerial.h"
#include "IUMQTTHelper.h"
#include "IURawDataHelper.h"
#include "IUTimeHelper.h"
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
        static const uint32_t hostResponseTimeout = 1000;  // ms
        // Default duration of deep-sleep
        static const uint32_t deepSleepDuration = 1000;  // ms
        /** Connection retry constants **/
        static const uint8_t connectionRetry = 3;
        // Single connection attempt timeout
        static const uint32_t connectionTimeout = 5000;  // ms
        // Delay between 2 connection attemps
        static const uint32_t reconnectionInterval = 1000;  // ms
        Conductor();
        virtual ~Conductor() {}
        /***** Communication with host *****/
        void readMessagesFromHost();
        void processMessageFromHost();
        /***** WiFi credentials and config *****/
        void forgetWiFiCredentials();
        void forgetWiFiStaticConfig();
        /***** Wifi connection and status *****/
        void Conductor::turnOffRadio(uint32_t duration_us=0);
        void Conductor::turnOnRadio();
        void Conductor::disconnectWifi(bool wifiOff=true);
        bool Conductor::reconnect(bool forceNewCredentials);

        bool reconnectionTimedOut(bool resetTimer);
        bool reconnect(bool forceNewCredentials=false);
        uint8_t waitForConnectResult();
        bool disableWifi(uint32_t duration_us);
        /***** MQTT *****/
        void processMessageFromMQTT();
        /***** Main operations *****/
        void setup();
        void loop();
        /***** Debugging *****/
        void getWifiInfo(char *destination, bool mqttOn);
        void debugPrintWifiInfo();

    protected:
        /***** MAC addresses *****/
        bool m_unknownBleMacAddress;
        char m_bleMacAddress[18];
        char m_wifiMacAddress[18];
        /***** Wifi connection and status *****/
        bool m_radioOn;
        uint32_t m_lastConnectionAttempt;
        uint32_t m_lastConnected;
        uint8_t m_remainingConnectionAttempt;
        /***** WiFi credentials and config *****/
        MultiMessageValidator<2> m_credentialValidator;
        char m_userSSID[wifiCredentialLength];
        char m_userPassword[wifiCredentialLength];
        MultiMessageValidator<3> m_staticConfigValidator;
        IPAddress m_staticIp;
        IPAddress m_staticGateway;
        IPAddress m_staticSubnet;
        /***** Main operations *****/
        bool m_shouldWakeUp;

};

#endif // CONDUCTOR_H
