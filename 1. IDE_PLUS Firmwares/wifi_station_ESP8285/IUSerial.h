#ifndef IUSERIAL_H
#define IUSERIAL_H

#include "Utilities.h"

/**
 * 
 */
class IUSerial
{
    public:
        /***** Public constants *****/
        HardwareSerial *port;
        const uint32_t baudRate;
        const uint16_t bufferSize;
        const char stopChar;
        /***** Core *****/
        IUSerial(HardwareSerial *serialPort, uint32_t rate=115200,
                 uint16_t buffSize=256, char stop=';',
                 uint16_t dataReceptionTimeout=200);
        virtual ~IUSerial() { }
        void begin();
        /***** Communication with host *****/
        virtual void resetBuffer();
        virtual bool readToBuffer();
        virtual bool hasTimedOut();
        virtual bool hasNewMessage() { return m_newMessage; }
        virtual char* getBuffer() { return m_buffer; }
        virtual uint16_t getCurrentBufferLength() { return m_bufferIndex; }

    protected:
        /***** Communication *****/
        char m_buffer[256];
        uint16_t m_bufferIndex;
        bool m_newMessage;
        // Data reception robustness variables
        // Buffer is emptied if now - lastReadTime > dataReceptionTimeout
        uint16_t m_dataReceptionTimeout;  // in ms
        uint32_t m_lastReadTime;  // in ms, as outputed by millis()

};


/***** Instanciation *****/

extern IUSerial hostSerial;

#endif // IUSERIAL_H
