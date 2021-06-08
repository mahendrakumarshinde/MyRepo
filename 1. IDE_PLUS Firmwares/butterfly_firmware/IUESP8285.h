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
    enum staticIP : uint8_t {STATIC_IP,
                        GATEWAY_IP,
                        SUBNET_IP,
                        DNS1_IP,
                        DNS2_IP,
                        IP_COUNT};
    enum credentials : uint8_t {AUTH_TYPE,
                        SSID,
                        PASSWORD,
                        USERNAME,
                        CRED_COUNT};
        /***** Preset values and default settings *****/
        static const uint8_t ESP32_ENABLE_PIN = A2;  // IDE1.5_PORT_CHANGE
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
        static const uint32_t confirmPublicationTimeout = 60000; // ms
        // Default Config type for flash storing
        static const IUFlash::storedConfig STORED_CFG_TYPE = IUFlash::CFG_WIFI0;
        // Size of Json buffer (to parse config json)
        static const uint16_t JSON_BUFFER_SIZE = 256;
        // WiFi firmware FirmwareVersion
        char espFirmwareVersion[6];
        bool espFirmwareVersionReceived = false;
        int current_rssi = 0;
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
        void setWiFiConfig(credentials type, const char *config, uint8_t length);
        void setStaticIP(staticIP type, IPAddress IP);
        bool setStaticIP(staticIP type, const char *IP, uint8_t len);
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
        // void publishRawData(char *rawData)
        //     { sendMSPCommand(MSPCommand::PUBLISH_RAW_DATA, rawData); }
        void publishFeature(char *features)
            { sendMSPCommand(MSPCommand::PUBLISH_FEATURE, features); }
        void publishDiagnostic(char *diagnotic)
            { sendMSPCommand(MSPCommand::PUBLISH_DIAGNOSTIC, diagnotic); }
        // Reset last publication confirmation timer on establishing connection.
        void m_setLastConfirmedPublication(void) { m_lastConfirmedPublication = millis();} 
        bool getConnectionStatus(){return m_wifiConfirmPublication;}
        void setAwakeTimerStart() { m_awakeTimerStart = millis(); }
        void clearSendWifiConfig();
    private:
        /**** Confirm Publication Timeout ****/
        uint32_t m_lastConfirmedPublication = 0;
        /***** Configuring the WiFi *****/
        char m_ssid[wifiCredentialLength];
        char m_psk[wifiCredentialLength];
        char m_username[wifiCredentialLength];
        char m_wifiAuthType[wifiCredentialLength];
        MultiMessageValidator<credentials::CRED_COUNT> m_credentialValidator;
        bool m_credentialSent = false;
        bool m_credentialReceptionConfirmed = false;
        IPAddress m_staticIP;
        IPAddress m_staticGateway;
        IPAddress m_staticSubnet;
        IPAddress m_dns1;
        IPAddress m_dns2;
        bool m_staticConfigSent = false;
        bool m_staticConfigReceptionConfirmed = false;
        MultiMessageValidator<staticIP::IP_COUNT> m_staticConfigValidator;
        /***** Connection and auto-sleep *****/
        bool m_on = true;
        bool m_connected = false;
        void m_setConnectedStatus(bool status);
        bool m_sleeping = false;
        bool m_wifiConfirmPublication = false;
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
        /***** Callbacks *****/
        void (*m_onConnect)() = NULL;
        void (*m_onDisconnect)() = NULL;
        void clearStaticIPBuffers();
         /***** Use Default WIFI Config option *****/
        uint32_t m_retryDefWifiConfTime = 0;
        bool m_defUserWifi = false; // False- use default conf. True use user conf
};

#endif // IUESP8285_H
