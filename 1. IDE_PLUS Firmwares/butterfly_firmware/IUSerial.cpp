#include "IUSerial.h"


/* =============================================================================
    Constructor & desctructors
============================================================================= */

IUSerial::IUSerial(HardwareSerial *serialPort, char *charBuffer,
                   uint16_t bufferSize, PROTOCOL_OPTIONS protocol,
                   uint32_t rate, char stopChar,
                   uint16_t dataReceptionTimeout) :
    port(serialPort),
    m_bufferSize(bufferSize),
    baudRate(rate),
    m_dataReceptionTimeout(dataReceptionTimeout),
    m_lastReadTime(0),
    m_protocol(protocol),
    // Legacy Protocol specific variables
    m_stopChar(stopChar),
    // MSP specific variables
    m_mspState(MSP_IDLE),
    m_mspCommand(255),
    m_mspDataSize(0),
    m_mspChecksum(0)
{
    m_buffer = charBuffer;
    resetBuffer();
}

void IUSerial::begin()
{
    port->begin(baudRate);
    delay(10);
}


/* =============================================================================
    Communication
============================================================================= */

/**
 * Empty the buffer and reset the index counter.
 */
void IUSerial::resetBuffer()
{
    m_bufferIndex = 0;
//    m_buffer[0] = '\0';  // End of string at 1st character (ie buffer is empty)
    m_newMessage = false;
}

/**
 * Read available incoming bytes to fill the buffer
 *
 * If m_newMessage is true, the function will return True immediatly. Else, it
 * will return true as soon as a new message has been received.
 * To read several messages, the function should be called repeatedly.
 * Also, a data reception timeout check is performed before reading (see
 * hasTimedOut).
 * @return True if a full message has been received, else false
 */
bool IUSerial::readToBuffer()
{
    if (m_newMessage)
    {
        return true;
    }
    if (hasTimedOut())
    {
        resetBuffer();
    }
    while (port->available() > 0)
    {
        if (m_bufferIndex >= m_bufferSize)
        {
            if (debugMode)
            {
                debugPrint(F("Reception buffer is overflowing: "), false);
                debugPrint(m_buffer);
            }
            resetBuffer();
            return false;
        }
        switch (m_protocol)
        {
            case LEGACY_PROTOCOL:
                m_newMessage = readCharLegacyProtocol();
                break;
            case MS_PROTOCOL:
                m_newMessage = readCharMsp();
                break;
            case CUSTOM_PROTOCOL:
                m_newMessage = readCharCustomProtocol();
                break;
        }
        if (m_newMessage)
        {
            return true;
        }
    }
    m_lastReadTime = millis();
    return false;
}

/**
 * Reset the buffer if too much time passed since last reception
 *
 * Time since last reception is compared to m_dataReceptionTimeout.
 * @return true if a timeout happened and that the buffer needs to be reset,
 * else false.
 */
bool IUSerial::hasTimedOut()
{
    if (m_bufferIndex > 0 && (millis() -
            m_lastReadTime > m_dataReceptionTimeout))
    {
        m_bufferIndex = 0;
        return true;
    }
    return false;
}


/*==============================================================================
    Legacy protocol
============================================================================= */

/**
 * Read a char from Serial using IU legacy protocol
 *
 * Function append the char to reception buffer until receiving stopChar.
*
* @return True if current message has been completed, else false.
 */
bool IUSerial::readCharLegacyProtocol()
{
    bool messageIsComplete = false;
    char newChar = port->read();
    if (newChar == m_stopChar)
    {
        messageIsComplete = true;
        newChar = '\0';
    }
    m_buffer[m_bufferIndex] = newChar;
    m_bufferIndex++;
    return messageIsComplete;
}


/*==============================================================================
    MSP (Multiwii Serial Protocol)
============================================================================= */

/**
* Read a char from Serial using the MultiWii Serial Protocol (MSP)
*
* Function manages port state and packet logistics.
*
* @return True if current message has been completed, else false.
*/
bool IUSerial::readCharMsp()
{
    bool messageIsComplete = false;
    uint8_t c = port->read();
    switch (m_mspState)
    {
        case MSP_IDLE:
            m_mspState = (c == '$') ? MSP_HEADER_START : MSP_IDLE;
            break;
        case MSP_HEADER_START:
            m_mspState = (c == 'M') ? MSP_HEADER_M : MSP_IDLE;
            break;
        case MSP_HEADER_M:
            m_mspState = ( c=='<' ) ? MSP_HEADER_ARROW : MSP_IDLE;
            break;
        case MSP_HEADER_ARROW:
            if (c > m_bufferSize)
            {
                m_mspState = MSP_IDLE;
                if (debugMode)
                {
                    debugPrint(F("MSP command asks to receive too"
                                 " long a msg: "), false);
                    debugPrint(c);
                }
            }
            else
            {
                m_mspDataSize = c;
                m_bufferIndex = 0;
                m_mspChecksum = 0;
                m_mspChecksum ^= c;
                m_mspState = MSP_HEADER_SIZE;
            }
            break;
        case MSP_HEADER_SIZE:
            m_mspCommand = c;
            m_mspChecksum ^= c;
            m_mspState = MSP_HEADER_CMD;
            break;
        case MSP_HEADER_CMD:
            if (m_bufferIndex < m_mspDataSize)
            {
                m_mspChecksum ^= c;
                m_buffer[m_bufferIndex++] = (char) c;
            }
            else
            {
                // Compare calculated and transferred checksum
                if (m_mspChecksum == c)
                {
                    // We got a valid packet
                    messageIsComplete = true;
                }
                m_mspState = MSP_IDLE;
            }
            break;
    }
    return messageIsComplete;
}


/*==============================================================================
    Custom protocol
============================================================================= */

/**
 * Read a char from Serial using a custom protocol
 *
 * Use the custom protocol to implement subclass specific protocol.
 *
 * @return True if current message has been completed, else false.
 */
bool IUSerial::readCharCustomProtocol()
{
    char c = port->read();
    if (loopDebugMode)
    {
        debugPrint(F("Custom protocol is not implemented: "), false);
        debugPrint(c);
    }
    return false;
}
