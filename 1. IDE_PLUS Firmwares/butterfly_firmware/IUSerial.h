#ifndef IUSERIAL_H
#define IUSERIAL_H

#include <Arduino.h>

#include "Keywords.h"
#include "Component.h"


/**
 * Custom implementation of Serial port (UART)
 *
 * Component:
 *   Name: -
 * Description:
 *   Serial port, UART
 */
class IUSerial : public Component
{
    public:
        /***** Public constants *****/
        const StreamingMode::option interfaceType;
        HardwareSerial *port;
        const uint32_t baudRate;
        const char stopChar;
        enum WSP_STATE : uint8_t {WSP_IDLE,
                                  WSP_HEADER_START,
                                  WSP_HEADER_M,
                                  WSP_HEADER_ARROW,
                                  WSP_HEADER_SIZE,
                                  WSP_HEADER_CMD};
        /***** Core *****/
        IUSerial(StreamingMode::option interface, HardwareSerial *serialPort,
                 char *charBuffer, uint16_t bufferSize, uint32_t rate=57600,
                 char stop=';', uint16_t dataReceptionTimeout=2000,
                 uint16_t forceMessageSize=0);
        virtual ~IUSerial() {}
        /***** Hardware and power management *****/
        virtual void setupHardware();
        /***** Communication *****/
        virtual void resetBuffer();
        virtual bool readToBuffer();
        virtual bool hasTimedOut();
        virtual bool hasNewMessage() { return m_newMessage; }
        virtual char* getBuffer() { return m_buffer; }
        virtual uint16_t getCurrentBufferLength() { return m_bufferIndex; }
        virtual void setForceMessageSize(uint16_t messageSize)
            { m_forceMessageSize = messageSize; }
        /***** MSP (Multiwii Serial Protocol) *****/
        virtual void readMspMessages();



    protected:
        /***** Communication *****/
        char *m_buffer;
        uint16_t m_bufferSize;
        bool m_newMessage;
        uint16_t m_bufferIndex;
        // Data reception robustness variables
        // Buffer is emptied if now - lastReadTime > dataReceptionTimeout
        uint16_t m_dataReceptionTimeout;  // in ms
        uint32_t m_lastReadTime;  // in ms, as outputed by millis()
        uint16_t m_forceMessageSize;
        /***** MSP (Multiwii Serial Protocol) *****/
        WSP_STATE m_state;
        uint8_t m_command;
        uint8_t m_wspDataSize;
        uint8_t m_checksum;
        uint8_t m_indRX;

};


/***** Instanciation *****/

extern char iuUSBBuffer[20];
extern IUSerial iuUSB;

#ifdef EXTERNAL_WIFI
    extern char iuWiFiBuffer[500];
    extern IUSerial iuWiFi;
#endif

#endif // IUSERIAL_H
