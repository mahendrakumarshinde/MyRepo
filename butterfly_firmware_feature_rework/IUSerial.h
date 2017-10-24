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
        IUSerial(InterfaceType::option interType, HardwareSerial *serialPort,
                 uint32_t rate=57600, uint16_t buffSize=20, char stop=';',
                 uint16_t dataReceptionTimeout=2000);
        virtual ~IUSerial() {}
        /***** Public constants *****/
        const InterfaceType::option interface;
        HardwareSerial *port;
        const uint32_t baudRate;
        const uint16_t bufferSize;
        const char stopChar;
        /***** Hardware and power management *****/
        virtual void setupHardware();
        /***** Communication *****/
        virtual void resetBuffer();
        virtual bool readToBuffer();
        virtual bool hasTimedOut();
        virtual bool hasNewMessage() { return m_newMessage; }
        virtual char* getBuffer() { return m_buffer; }
        virtual uint16_t getCurrentBufferLength() { return m_bufferIndex; }

    protected:
        /***** Communication *****/
        char m_buffer[20];
        uint16_t m_bufferIndex;
        bool m_newMessage;
        // Data reception robustness variables
        // Buffer is emptied if now - lastReadTime > dataReceptionTimeout
        uint16_t m_dataReceptionTimeout;  // in ms
        uint32_t m_lastReadTime;  // in ms, as outputed by millis()
};


/***** Instanciation *****/

extern IUSerial iuUSB;

#endif // IUSERIAL_H
