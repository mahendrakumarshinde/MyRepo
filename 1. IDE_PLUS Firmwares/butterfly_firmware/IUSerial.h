#ifndef IUSERIAL_H
#define IUSERIAL_H

#include <Arduino.h>

#include "Logger.h"


/**
 * Custom implementation of Serial port (UART)
 *
 * Class implements function to read data from Serial using a protocol.
 * Protocol are implemented by default and child class can define their custom
 * protocol.
 */
class IUSerial
{
    public:
        /***** Public constants *****/
        HardwareSerial *port;
        const uint32_t baudRate;
        enum PROTOCOL_OPTIONS : uint8_t {LEGACY_PROTOCOL,
                                         MS_PROTOCOL,
                                         CUSTOM_PROTOCOL};
        enum MSP_STATE : uint8_t {MSP_IDLE,
                                  MSP_HEADER_START,
                                  MSP_HEADER_M,
                                  MSP_HEADER_ARROW,
                                  MSP_HEADER_SIZE,
                                  MSP_HEADER_CMD};
        /***** Core *****/
        IUSerial(HardwareSerial *serialPort, char *charBuffer,
                 uint16_t bufferSize, PROTOCOL_OPTIONS protocol,
                 uint32_t rate=57600, char stopChar=';',
                 uint16_t dataReceptionTimeout=2000);
        virtual ~IUSerial() {}
        virtual void begin();
        /***** Communication *****/
        virtual void resetBuffer();
        virtual bool readToBuffer();
        virtual bool hasTimedOut();
        virtual bool hasNewMessage() { return m_newMessage; }
        virtual char* getBuffer() { return m_buffer; }
        virtual uint16_t getCurrentBufferLength() { return m_bufferIndex; }

    protected:
        /***** Communication *****/
        char *m_buffer;
        uint16_t m_bufferSize;
        bool m_newMessage;
        uint16_t m_bufferIndex;
        /***** Data reception robustness variables *****/
        // Buffer is emptied if now - lastReadTime > dataReceptionTimeout
        uint16_t m_dataReceptionTimeout;  // in ms
        uint32_t m_lastReadTime;  // in ms, as outputed by millis()
        /***** Protocol selection *****/
        PROTOCOL_OPTIONS m_protocol;
        /***** Legacy Protocol *****/
        virtual bool readCharLegacyProtocol();
        char m_stopChar;
        /***** MSP (Multiwii Serial Protocol) *****/
        virtual bool readCharMsp();
        MSP_STATE m_mspState;
        uint8_t m_mspCommand;
        uint8_t m_mspDataSize;
        uint8_t m_mspChecksum;
        /***** Custom Protocol *****/
        virtual bool readCharCustomProtocol();
};

#endif // IUSERIAL_H
