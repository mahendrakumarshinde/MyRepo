#include "IUUSB.h"

IUUSB::IUUSB(HardwareSerial *serialPort, char *charBuffer, uint16_t bufferSize,
             PROTOCOL_OPTIONS protocol, uint32_t rate,
             char stopChar, uint16_t dataReceptionTimeout) :
    IUSerial(serialPort, charBuffer, bufferSize, protocol, rate, stopChar,
             dataReceptionTimeout)
{
    //ctor
}

/* =============================================================================
    Custom protocol
============================================================================= */

/**
 * Read a char from Serial using a custom protocol
 *
 * Use the custom protocol to implement subclass specific protocol.
 *
 * @return True if current message has been completed, else false.
 */
bool IUUSB::readCharCustomProtocol()
{

    bool messageIsComplete = readCharLegacyProtocol();
    if (!messageIsComplete)
    {
        // TODO Remove this additional test once the data collection software is
        // improved
        if ((m_bufferIndex == 11 &&
             strncmp(m_buffer, "IUCMD_START", 11) == 0) ||
            (m_bufferIndex == 9 &&
             strncmp(m_buffer, "IUCMD_END", 9) == 0))
        {
            m_buffer[m_bufferIndex] = '\0';
            m_bufferIndex++;
            messageIsComplete = true;
        }
    }
    return messageIsComplete;
}
