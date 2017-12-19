#include "IUSerial.h"


/* =============================================================================
    Constructor & desctructors
============================================================================= */

IUSerial::IUSerial(StreamingMode::option interface, HardwareSerial *serialPort,
                   char *charBuffer, uint16_t bufferSize, uint32_t rate,
                   char stop, uint16_t dataReceptionTimeout,
                   uint16_t forceMessageSize) :
    Component(),
    interfaceType(interface),
    port(serialPort),
    m_bufferSize(bufferSize),
    baudRate(rate),
    stopChar(stop),
    m_dataReceptionTimeout(dataReceptionTimeout),
    m_lastReadTime(0),
    m_forceMessageSize(forceMessageSize)
{
    m_buffer = charBuffer;
    resetBuffer();
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set up the component and finalize the object initialization
 */
void IUSerial::setupHardware()
{
    port->begin(baudRate);
    delay(100);
    wakeUp();
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
 * Buffer fill up until receiving stopChar or being full. To process several
 * messages sequentially, function should be called several times.
 * If m_newMessage is true, the function will return True immediatly.
 * Also, a data reception timeout check is performed before reading (see
 * hasTimedOut).
 * @param processBuffer A function to process buffer
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
    char newChar;
    while (port->available() > 0)
    {
        if (m_bufferIndex >= m_bufferSize) // overflowing
        {
            m_bufferIndex = 0;
            if (debugMode)
            {
                debugPrint(F("Reception buffer is overflowing: "), false);
                debugPrint(m_buffer);
            }
        }
        newChar = port->read();
        m_buffer[m_bufferIndex] = newChar;
        m_bufferIndex++;
        if (newChar == stopChar)
        {
            // Replace stopChar with end of string char.
            m_buffer[m_bufferIndex - 1] = '\0';
            m_newMessage = true;
            return true;
        }
        if (m_forceMessageSize > 0 && m_bufferIndex >= m_forceMessageSize)
        {
            m_buffer[m_bufferIndex] = '\0';
            m_newMessage = true;
            return true;
        }
        if (interfaceType == StreamingMode::WIRED)
        {
            if ((m_bufferIndex == 11 && strncmp(m_buffer, "IUCMD_START", 11) == 0) ||
                (m_bufferIndex == 9 && strncmp(m_buffer, "IUCMD_END", 9) == 0))
            {
                m_buffer[m_bufferIndex] = '\0';
                m_newMessage = true;
                return true;
            }
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


/* =============================================================================
    Instanciation
============================================================================= */

char iuUSBBuffer[20] = "";
IUSerial iuUSB(StreamingMode::WIRED, &Serial, iuUSBBuffer, 20, 115200, '\n',
               2000);

#ifdef EXTERNAL_WIFI
    char iuWiFiBuffer[500] = ""
    IUSerial iuWiFi(StreamingMode::WIFI, &Serial3, iuWiFiBuffer, 500, 
                    115200, ';', 500);
#endif
