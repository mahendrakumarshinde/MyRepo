#ifndef IUESP8285_H
#define IUESP8285_H

#include <Arduino.h>

#include <IUSerial.h>
#include <MultiMessageValidator.h>
#include "Component.h"

/**
 * Wifi chip
 *
 * Component:
 *   Name:
 *   ESP-8285
 * Description:
 *
 */
class IUESP8285 : public IUSerial, public Component
{
    public:
        /***** Preset values and default settings *****/
        // Max expected length of WiFi SSID or password
        static const uint8_t wifiCredentialLength = 64;
        // WiFi config (credentials or Static IP) MultiMessageValidator timeout
        static const uint32_t wifiConfigReceptionTimeout = 1000;  // ms
        // Sleep management
        static const uint32_t defaultAutoSleepDelay = 45000;  // ms
        static const uint32_t defaultAutoSleepDuration = 60000;  // ms
        /***** Core *****/
        IUESP8285(HardwareSerial *serialPort, char *charBuffer,
                  uint16_t bufferSize, PROTOCOL_OPTIONS protocol, uint32_t rate,
                  char stopChar, uint16_t dataReceptionTimeout);
        virtual ~IUESP8285() {}
        bool isConnected() { return m_connected; }
        bool isAvailable() { return !m_sleeping; }
        bool isSleeping() { return m_sleeping; }
        bool isWorking() { return m_working; }
        MacAddress getMacAddress() { return m_macAddress; }
        /***** Hardware and power management *****/
        virtual void setupHardware();
        virtual void setPowerMode(PowerMode::option pMode);
        void setAutoSleepDelay(uint32_t deltaT) { m_autoSleepDelay = deltaT; }
        void manageAutoSleep();
        /***** Network Configuration *****/
        void setSSID(char *ssid, uint8_t length);
        void setPassword(char *psk, uint8_t length);
        /***** Inbound communication *****/
        bool processMessage();
        /***** Outbound communication *****/
        void hardReset() { sendMSPCommand(MSPCommand::WIFI_HARD_RESET); }
        void sendBleMacAddress(MacAddress bleMac)
            { mspSendMacAddress(MSPCommand::RECEIVE_BLE_MAC, bleMac); }
        void sendWiFiCredentials();
        void forgetCredentials();
        void sendStaticConfig();
        void forgetStaticConfig();
        void connect();
        void disconnect();
        void publishRawData(char *rawData)
            { sendMSPCommand(MSPCommand::PUBLISH_RAW_DATA, rawData); }
        void publishFeature(char *features)
            { sendMSPCommand(MSPCommand::PUBLISH_FEATURE, features); }
        void publishDiagnostic(char *diagnotic)
            { sendMSPCommand(MSPCommand::PUBLISH_DIAGNOSTIC, diagnotic); }

    private:
        /***** Configuring the WiFi *****/
        char m_ssid[wifiCredentialLength];
        char m_psk[wifiCredentialLength];
        MultiMessageValidator<2> m_credentialValidator;
        bool m_credentialSent = false;
        bool m_credentialReceptionConfirmed = false;
        IPAddress m_staticIp;
        IPAddress m_staticGateway;
        IPAddress m_staticSubnet;
        bool m_staticConfigSent = false;
        bool m_staticConfigReceptionConfirmed = false;
        MultiMessageValidator<3> m_staticConfigValidator;
        /***** Connection and auto-sleep *****/
        bool m_connected = false;
        bool m_sleeping = false;
        uint32_t m_awakeTimerStart = 0;
        uint32_t m_sleepTimerStart = 0;
        uint32_t m_autoSleepDelay = defaultAutoSleepDelay;
        uint32_t m_autoSleepDuration = defaultAutoSleepDuration;
        /***** Informative variables *****/
        MacAddress m_macAddress;
        bool m_working = false;
};

#endif // IUESP8285_H
