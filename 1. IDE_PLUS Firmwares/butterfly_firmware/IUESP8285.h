#ifndef IUESP8285_H
#define IUESP8285_H

#include <Arduino.h>
#include <ArduinoJson.h>

#include <IPAddress.h>
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
        static const uint8_t ESP32_ENABLE_PIN = 10;  // IDE1.5_PORT_CHANGE Actual Value = A2. Temp. used - 10
        // Max expected length of WiFi SSID or password
        static const uint8_t wifiCredentialLength = 64;
        // WiFi config (credentials or Static IP)
        static const uint32_t wifiConfigReceptionTimeout = 5000;  // ms
        // Sleep management
        static const uint32_t defaultAutoSleepDelay = 130000;  // ms
        static const uint32_t defaultAutoSleepDuration = 5000;  // ms
        // Timeout for "Trying to connect" LED blinking
        static const uint32_t displayConnectAttemptTimeout = 20000;  // ms
        // Timeout for connection status message
        static const uint32_t connectedStatusTimeout = 30000;  // ms
        // Timeout for no response (only when WiFi is awake)
        static const uint32_t noResponseTimeout = 45000; // ms
        // Timeout for failing to confim feature publication
        static const uint32_t confirmPublicationTimeout = 40000; // ms
        // Default Config type for flash storing
        static const IUFlash::storedConfig STORED_CFG_TYPE = IUFlash::CFG_WIFI0;
        // Size of Json buffer (to parse config json)
        static const uint16_t JSON_BUFFER_SIZE = 256;
        // WiFi firmware FirmwareVersion
        char espFirmwareVersion[6];
        bool espFirmwareVersionReceived = false;
        /***** Core *****/
        IUESP8285(HardwareSerial *serialPort, char *charBuffer,
                  uint16_t bufferSize, PROTOCOL_OPTIONS protocol, uint32_t rate,
                  char stopChar, uint16_t dataReceptionTimeout);
        virtual ~IUESP8285() {}
        bool isConnected() { return m_connected; }
        bool isAvailable() { return (m_on && !m_sleeping); }
        bool isWorking() { return m_working; }
        bool arePublicationsFailing();
        MacAddress getMacAddress() { return m_macAddress; }
        void setOnConnect(void (*callback)()) { m_onConnect = callback; }
        void setOnDisconnect(void (*callback)()) { m_onDisconnect = callback; }
        /***** Hardware and power management *****/
        virtual void setupHardware();
        void turnOn(bool forceTimerReset=false);
        void turnOff();
        void hardReset();
        virtual void setPowerMode(PowerMode::option pMode);
        void setAutoSleepDelay(uint32_t deltaT) { m_autoSleepDelay = deltaT; }
        void manageAutoSleep(bool wakeUpNow=false);
        virtual bool readMessages();
        /***** Local storage (flash) management *****/
        void saveConfigToFlash(IUFlash *iuFlashPtr,
                IUFlash::storedConfig configType=STORED_CFG_TYPE);
        /***** Network Configuration *****/
        bool configure(JsonVariant &config);
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
	      void sendHostFirmwareVersion(const char *FirmwareVersion)
            { sendMSPCommand(MSPCommand::RECEIVE_HOST_FIRMWARE_VERSION, FirmwareVersion); } // send Firmware Vr to WIfi
        void sendHostSamplingRate(const int samplingRate)
            { 
                char sampRate[7];                   // increase if samplingRate goes above 6 digits
                itoa(samplingRate, sampRate, 10);
                sendMSPCommand(MSPCommand::RECEIVE_HOST_SAMPLING_RATE, sampRate);
            }
        void sendHostBlockSize(const int blockSize)
            {
                char bs[6];                         // increase if block size goes above 5 digits
                itoa(blockSize, bs, 10);
                sendMSPCommand(MSPCommand::RECEIVE_HOST_BLOCK_SIZE, bs);
            }
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
        void m_setConnectedStatus(bool status);
        bool m_sleeping = false;
        uint32_t m_awakeTimerStart = 0;
        uint32_t m_sleepTimerStart = 0;
        uint32_t m_autoSleepDelay = defaultAutoSleepDelay;
        uint32_t m_autoSleepDuration = defaultAutoSleepDuration;
        uint32_t m_lastResponseTime = 0;
        uint32_t m_lastConnectedStatusTime = 0;
        uint32_t m_lastConfirmedPublication = 0;
        /***** Informative variables *****/
        MacAddress m_macAddress;
        bool m_working = false;
        uint32_t m_displayConnectAttemptStart = 0;
        /***** Callbacks *****/
        void (*m_onConnect)() = NULL;
        void (*m_onDisconnect)() = NULL;
};

#endif // IUESP8285_H
