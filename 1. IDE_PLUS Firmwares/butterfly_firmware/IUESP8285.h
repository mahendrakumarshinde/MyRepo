#ifndef IUESP8285_H
#define IUESP8285_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include <IUSerial.h>
#include <MultiMessageValidator.h>
#include "Component.h"
#include "IUFlash.h"


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
        static const uint8_t ESP8285_ENABLE_PIN = A2;
        // Max expected length of WiFi SSID or password
        static const uint8_t wifiCredentialLength = 64;
        // WiFi config (credentials or Static IP) MultiMessageValidator timeout
        static const uint32_t wifiConfigReceptionTimeout = 5000;  // ms
        // Sleep management
        static const uint32_t defaultAutoSleepDelay = 130000;  // ms
        static const uint32_t defaultAutoSleepDuration = 90000;  // ms
        // Timeout for "Trying to connect" LED blinking
        static const uint32_t displayConnectAttemptTimeout = 20000;  // ms
        // Timeout for connection status message
        static const uint32_t connectedStatusTimeout = 30000;  // ms
        // Timeout for no response (only when WiFi is awake)
        static const uint32_t noResponseTimeout = 30000; // ms
        // Default Config type for flash storing
        static const IUFlash::storedConfig STORED_CFG_TYPE = IUFlash::CFG_WIFI0;
        // Size of Json buffer (to parse config json)
        static const uint16_t JSON_BUFFER_SIZE = 256;
        /***** Core *****/
        IUESP8285(HardwareSerial *serialPort, char *charBuffer,
                  uint16_t bufferSize, PROTOCOL_OPTIONS protocol, uint32_t rate,
                  char stopChar, uint16_t dataReceptionTimeout);
        virtual ~IUESP8285() {}
        bool isConnected() { return m_connected; }
        bool isAvailable() { return (m_on && !m_sleeping); }
        bool isWorking() { return m_working; }
        MacAddress getMacAddress() { return m_macAddress; }
        /***** Hardware and power management *****/
        virtual void setupHardware();
        void turnOn(bool forceTimerReset=false);
        void turnOff();
        void hardReset();
        virtual void setPowerMode(PowerMode::option pMode);
        void setAutoSleepDelay(uint32_t deltaT) { m_autoSleepDelay = deltaT; }
        void manageAutoSleep(bool wakeUpNow=false);
        virtual bool readToBuffer();
        /***** Local storage (flash) management *****/
        bool loadConfigFromFlash(IUFlash *iuFlashPtr,
                IUFlash::storedConfig configType=STORED_CFG_TYPE);
        void saveConfigToFlash(IUFlash *iuFlashPtr,
                IUFlash::storedConfig configType=STORED_CFG_TYPE);
        /***** Network Configuration *****/
        void setSSID(const char *ssid, uint8_t length);
        void setPassword(const char *psk, uint8_t length);
        void setStaticIP(IPAddress staticIP);
        bool setStaticIP(const char *staticIP, uint8_t len);
        void setGateway(IPAddress gatewayIP);
        bool setGateway(const char *gatewayIP, uint8_t len);
        void setSubnetMask(IPAddress subnetIP);
        bool setSubnetMask(const char *subnetIP, uint8_t len);
        /***** User Inbound communication *****/
        void processUserMessage(char *buff, IUFlash *iuFlashPtr);
        /***** Guest Inbound communication *****/
        bool processChipMessage();
        /***** Guest Outbound communication *****/
        void softReset() { sendMSPCommand(MSPCommand::WIFI_SOFT_RESET); }
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
        IPAddress m_staticIP;
        IPAddress m_staticGateway;
        IPAddress m_staticSubnet;
        bool m_staticConfigSent = false;
        bool m_staticConfigReceptionConfirmed = false;
        MultiMessageValidator<3> m_staticConfigValidator;
        /***** Connection and auto-sleep *****/
        bool m_on = true;
        bool m_connected = false;
        bool m_sleeping = false;
        uint32_t m_awakeTimerStart = 0;
        uint32_t m_sleepTimerStart = 0;
        uint32_t m_autoSleepDelay = defaultAutoSleepDelay;
        uint32_t m_autoSleepDuration = defaultAutoSleepDuration;
        uint32_t m_lastResponseTime = 0;
        uint32_t m_lastConnectedStatusTime = 0;
        /***** Informative variables *****/
        MacAddress m_macAddress;
        bool m_working = false;
        uint32_t m_displayConnectAttemptStart = 0;
};

#endif // IUESP8285_H
