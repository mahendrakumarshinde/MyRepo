#ifndef HOSTSERIAL_H
#define HOSTSERIAL_H

#include "IUSerial.h"


class HostSerial : public IUSerial
{
    public:
        HostSerial(HardwareSerial *serialPort, char *charBuffer,
                   uint16_t bufferSize, PROTOCOL_OPTIONS protocol,
                   uint32_t rate=115200, char stopChar=';',
                   uint16_t dataReceptionTimeout=100) :
            IUSerial(serialPort, charBuffer, bufferSize, protocol, rate,
                     stopChar, dataReceptionTimeout) { }
        virtual ~HostSerial() { }

    protected:
        void askBleMacAddress()
            { sendMSPCommand(MSPCommands::ASK_BLE_MAC); }
        void alertNoSavedCredentials()
            { sendMSPCommand(MSPCommands::WIFI_ALERT_NO_SAVED_CREDENTIALS); }
        void alertConnected()
            { sendMSPCommand(MSPCommands::WIFI_ALERT_CONNECTED); }
        void alertDisconnected()
            { sendMSPCommand(MSPCommands::WIFI_ALERT_DISCONNECTED); }
        void alertAwake()
            { sendMSPCommand(MSPCommands::WIFI_ALERT_AWAKE); }
        void AlertSleeping()
            { sendMSPCommand(MSPCommands::WIFI_ALERT_SLEEPING); }
        void shouldContinueSleeping()
            { sendMSPCommand(MSPCommands::WIFI_REQUEST_ACTION); }
        void forwardCloudCommand(char *msg)
            { sendMSPCommand(MSPCommands::CONFIG_FORWARD_CMD, msg); }
        void initiateOTAUpdate()
            { sendMSPCommand(MSPCommands::OTA_INITIATE_UPDATE); }
        void sendOTAData()
            { sendMSPCommand(MSPCommands::OTA_RECEIVE_DATA); }

};

#endif // HOSTSERIAL_H
