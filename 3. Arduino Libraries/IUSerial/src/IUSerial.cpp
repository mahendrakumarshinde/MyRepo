#include "IUSerial.h"


/* =============================================================================
    Core
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
    m_stopChar(stopChar)
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
    m_mspCommand = MSPCommand::NONE;
    m_newMessage = false;
}

/**
 * Read available incoming bytes to fill the buffer
 *
 * If m_newMessage is true, the function will return True immediatly. Else, it
 * will return true as soon as a new message has been received.
 * To read several messages, the function should be called repeatedly.
 * Also, a data reception timeout check is performed before reading.
 * @return True if a full message has been received, else false
 */
bool IUSerial::readToBuffer()
{
    if (m_newMessage)
    {
        return true;
    }
    if (m_bufferIndex > 0 &&
        millis() - m_lastReadTime > m_dataReceptionTimeout)
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
        newChar = 0;
        messageIsComplete = true;
    }
    m_buffer[m_bufferIndex++] = newChar;
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
            m_mspDataSizeByte0 = c;
            m_bufferIndex = 0;
            m_mspChecksumIn = 0 ^ c;
            m_mspState = MSP_HEADER_SIZE_1;
            break;
        case MSP_HEADER_SIZE_1:
            m_mspDataSizeByte1 = c;
            m_mspDataSize = (uint16_t) m_mspDataSizeByte0 << 8 |
                            (uint16_t) m_mspDataSizeByte1;
            m_mspChecksumIn ^= c;
            m_mspState = MSP_HEADER_SIZE_2;
            if (m_mspDataSize > m_bufferSize)
            {
                m_mspState = MSP_IDLE;
                if (debugMode)
                {
                    debugPrint(F("MSP command asks to receive too"
                                 " long a msg: "), false);
                    debugPrint(m_mspDataSize);
                }
            }
            break;
        case MSP_HEADER_SIZE_2:
            m_mspCommand = (MSPCommand::command) c;
            m_mspChecksumIn ^= c;
            m_mspState = MSP_HEADER_CMD;
            break;
        case MSP_HEADER_CMD:
            if (m_bufferIndex < m_mspDataSize)
            {
                m_mspChecksumIn ^= c;
                m_buffer[m_bufferIndex++] = (char) c;
            }
            else
            {
                // Compare calculated and transferred checksum
                if (m_mspChecksumIn == c)
                {
                    m_buffer[m_bufferIndex++] = 0; // Terminate string
                    messageIsComplete = true;
                    if (debugMode)
                    {
                        debugPrint(F("Received MSP Command #"), false);
                        debugPrint(m_mspCommand);
                        if (m_mspDataSize > 0)
                        {
                            debugPrint(F("Message is: "), false);
                            debugPrint(m_buffer);
                        }
                    }
                }
                else
                {
                    if (debugMode)
                    {
                        debugPrint("Invalid MSP checksum");
                    }
                    resetBuffer();
                }
                m_mspState = MSP_IDLE;
            }
            break;
    }
    return messageIsComplete;
}

/**
 *
 */
void IUSerial::sendMSPCommand(MSPCommand::command cmd)
{
    if (debugMode)
    {
        debugPrint("Sending MSP Command #", false);
        debugPrint((uint8_t) cmd);
    }
    sendMspCommandHeader(0, cmd);
    sendMspCommandTail();
}

/**
 *
 */
void IUSerial::sendMSPCommand(MSPCommand::command cmd, const char* cmdMsg,
                              uint16_t cmdSize)
{
    if (debugMode)
    {
        debugPrint("Sending MSP Command #", false);
        debugPrint((uint8_t) cmd);
        debugPrint("MSP message is: ", false);
        for (uint16_t i = 0; i < cmdSize; i++)
        {
            debugPrint(cmdMsg[i], false);
        }
        debugPrint("");
    }
    sendMspCommandHeader(cmdSize, cmd);
    for (uint16_t i = 0; i < cmdSize; ++i)
    {
        mspChecksumAndSend(cmdMsg[i]);
    }
    sendMspCommandTail();
}


/**
 *
 */
void IUSerial::sendMSPCommand(MSPCommand::command cmd, const char* cmdMsg)
{
    if (cmdMsg != NULL)
    {
        uint16_t cmdSize = (uint16_t) strlen(cmdMsg);
        sendMSPCommand(cmd, cmdMsg, cmdSize);
    }
    else
    {
        sendMSPCommand(cmd);
    }
}

/**
 * Send long MSP command while making sure that the TX buffer doesn't overflow.
 *
 * The timeout parameter (in microseconds) allows to specify a max allowed
 * duration to send the whole command.
 */
void IUSerial::sendLongMSPCommand(MSPCommand::command cmd,
                                  uint32_t microTimeout, const char* cmdMsg,
                                  uint16_t cmdSize)
{
    if (debugMode)
    {
        debugPrint("Sending Long MSP Command #", false);
        debugPrint((uint8_t) cmd);
        debugPrint("MSP message is: ", false);
        for (uint16_t i = 0; i < cmdSize; i++)
        {
            debugPrint(cmdMsg[i], false);
        }
        debugPrint("");
    }
    uint32_t startT = micros();
    while (port->availableForWrite() < 6)
    {
        delayMicroseconds(5);
        if (micros() - startT > microTimeout)
        {
            if (debugMode) { debugPrint("Timeout exceeded"); }
            return;
        }
    }
    sendMspCommandHeader(cmdSize, cmd);
    uint16_t i = 0;
    for (uint16_t i = 0; i < cmdSize; ++i)
    {
        while (port->availableForWrite() < 1)
        {
            delayMicroseconds(1);
            if (micros() - startT > microTimeout)
            {
                if (debugMode) { debugPrint("Timeout exceeded"); }
                return;
            }
        }
        mspChecksumAndSend(cmdMsg[i]);
    }
    while (port->availableForWrite() < 1)
    {
        delayMicroseconds(1);
        if (micros() - startT > microTimeout)
        {
            if (debugMode) { debugPrint("Timeout exceeded"); }
            return;
        }
    }
    sendMspCommandTail();
}

/**
 *
 */
void IUSerial::startLongMSPCommand(MSPCommand::command cmd, uint16_t cmdSize)
{
    m_expectedLongMspCmdSize = cmdSize;
    sendMspCommandHeader(cmdSize, cmd);
}

/**
 *
 */
void IUSerial::streamLongMSPMessage(char c)
{
    m_actualLongMspCmdSize += mspChecksumAndSend(c);;
}

/**
 *
 */
void IUSerial::streamLongMSPMessage(const char* msg, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        streamLongMSPMessage(msg[i]);
    }
}

/**
 *
 */
bool IUSerial::endLongMSPCommand()
{
    sendMspCommandTail();
    bool success = (m_expectedLongMspCmdSize == m_actualLongMspCmdSize);
    m_expectedLongMspCmdSize = 0;
    m_actualLongMspCmdSize = 0;
    return success;
}

/**
 *
 */
size_t IUSerial::sendMspCommandHeader(uint16_t cmdSize, MSPCommand::command cmd)
{
    m_mspChecksumOut = 0;
    size_t n = 0;
    n += port->write('$');
    n += port->write('M');
    n += port->write('<');
    n += mspChecksumAndSend((uint8_t) (cmdSize >> 8));
    n += mspChecksumAndSend((uint8_t) cmdSize);
    n += mspChecksumAndSend((uint8_t) cmd);
    return n;
}

/**
 *
 */
size_t IUSerial::sendMspCommandTail()
{
    return port->write(m_mspChecksumOut);
}

/**
 *
 */
size_t IUSerial::mspChecksumAndSend(uint8_t b)
{
    m_mspChecksumOut ^= b;
    return port->write(b);
}


/*==============================================================================
    Convenience MSP functions
============================================================================= */

/**
 *
 */
void IUSerial::mspSendMacAddress(MSPCommand::command cmd, MacAddress mac)
{
    if (debugMode)
    {
        debugPrint("Sending MAC Address via MSP: ", false);
        debugPrint(mac);
    }
    sendMspCommandHeader(6, cmd);
    for (uint8_t i = 0; i < 6; ++i)
    {
        mspChecksumAndSend(mac[i]);
    }
    sendMspCommandTail();
}

/**
 *
 */
void IUSerial::mspSendIPAddress(MSPCommand::command cmd, IPAddress ip)
{
    if (debugMode)
    {
        debugPrint("Sending IP Address via MSP: ", false);
        debugPrint(ip);
    }
    sendMspCommandHeader(4, cmd);
    for (uint8_t i = 0; i < 4; ++i)
    {
        mspChecksumAndSend(ip[i]);
    }
    sendMspCommandTail();
}

/**
 *
 */
MacAddress IUSerial::mspReadMacAddress()
{
    return MacAddress(m_buffer[0], m_buffer[1], m_buffer[2],
                      m_buffer[3], m_buffer[4], m_buffer[5]);
}

/**
 *
 */
IPAddress IUSerial::mspReadIPAddress()
{
    return IPAddress(m_buffer[0], m_buffer[1], m_buffer[2], m_buffer[3]);
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
    if (debugMode)
    {
        debugPrint(F("Custom protocol is not implemented: "), false);
        debugPrint(c);
    }
    return false;
}
