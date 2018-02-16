#ifndef IUESP8285_H
#define IUESP8285_H

#include <Arduino.h>

#include "IUSerial.h"
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
        static const uint32_t defaultAutoSleepDelay = 45000;  // ms
        static const uint32_t defaultAutoSleepDuration = 60000;  // ms
        /***** Constructors & desctructors *****/
        IUESP8285(HardwareSerial *serialPort, char *charBuffer,
                  uint16_t bufferSize, PROTOCOL_OPTIONS protocol, uint32_t rate,
                  char stopChar, uint16_t dataReceptionTimeout);
        virtual ~IUESP8285() {}
        bool isConnected() { return m_connected; }
        bool isAvailable() { return !m_sleeping; }
        bool isSleeping() { return m_sleeping; }
        bool isWorking() { return m_working; }
        /***** Hardware and power management *****/
        virtual void setupHardware();
        virtual void setPowerMode(PowerMode::option pMode);
        void setAutoSleepDelay(uint32_t deltaT) { m_autoSleepDelay = deltaT; }
        void manageAutoSleep();
        /***** Inbound communication *****/
        bool processMessage();
        /***** Outbound communication *****/
        void hardReset() { sendMSPCommand(MSPCommand::WIFI_HARD_RESET); }
        void sendBleMacAddress(char *macAddress)
            {
                port->print("BLEMAC-");
                port->print(macAddress);
                port->print(';');
            }
        void setBleMacAddress(char *macAddress)
            { sendMSPCommand(MSPCommand::RECEIVE_BLE_MAC, macAddress); }
        void setSSID(char *ssid);
        void setPassword(char *password);
        void forgetCredentials();
        void connect();
        void disconnect();
        void publishRawData(char *rawData)
            { sendMSPCommand(MSPCommand::PUBLISH_RAW_DATA, rawData); }
        void publishFeature(char *features)
            { sendMSPCommand(MSPCommand::PUBLISH_FEATURE, features); }
        void publishDiagnostic(char *diagnotic)
            { sendMSPCommand(MSPCommand::PUBLISH_DIAGNOSTIC, diagnotic); }

    private:
        /***** Connection and auto-sleep *****/
        bool m_connected;
        bool m_sleeping;
        uint32_t m_lastConnectedTime;
        uint32_t m_sleepStartTime;
        uint32_t m_autoSleepDelay;
        uint32_t m_autoSleepDuration;
        /***** Informative variables *****/
        bool m_working;
};

#endif // IUESP8285_H
