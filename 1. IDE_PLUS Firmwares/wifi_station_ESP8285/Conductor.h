#ifndef CONDUCTOR_H
#define CONDUCTOR_H

#include <Arduino.h>
#include "IUSerial.h"
#include "IUMQTTHelper.h"
#include "IURawDataHandler.h"
#include "MultiMessageValidator.h"
#include "TimeManager.h"


/* =============================================================================
    Instanciation
============================================================================= */

extern char hostSerialBuffer[3072];
extern IUSerial hostSerial;


/* =============================================================================
    Conductor
============================================================================= */

class Conductor
{
    public:
        /***** Public constants *****/
        static const uint8_t wifiCredentialLength = 64;
        static const uint32_t wifiConfigReceptionTimeout = 1000;  // ms
        static const uint32_t reconnectionInterval = 5000;  // ms

        Conductor();
        virtual ~Conductor() {}
        /***** Communication with host *****/
        void readMessagesFromHost();
        void processMessageFromHost();
        /***** WiFi credentials and config *****/
        void forgetWiFiCredentials();
        void forgetWiFiStaticConfig();
        /***** Wifi connection and status *****/
        bool reconnectionTimedOut(bool resetTimer);
        bool reconnect(bool forceNewCredentials=false);
        uint8_t waitForConnectResult();
        /***** MQTT *****/
        void processMessageFromMQTT();
        /***** Debugging *****/
        void getWifiInfo(char *destination, bool mqttOn);
        void debugPrintWifiInfo();

    protected:
        /***** MAC addresses *****/
        bool m_unknownBleMacAddress;
        char m_bleMacAddress[18];
        char m_wifiMacAddress[18];
        /***** WiFi credentials and config *****/
        MultiMessageValidator<2> m_credentialValidator;
        char m_userSSID[wifiCredentialLength];
        char m_userPassword[wifiCredentialLength];
        MultiMessageValidator<3> m_staticConfigValidator;
        IPAddress m_staticIp;
        IPAddress m_staticGateway;
        IPAddress m_staticSubnet;
        /***** Wifi connection and status *****/
        uint32_t m_lastReconnectionAttempt;

};

#endif // CONDUCTOR_H
