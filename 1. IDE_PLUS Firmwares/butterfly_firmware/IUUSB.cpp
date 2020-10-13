#include "IUUSB.h"


/* =============================================================================
    Core
============================================================================= */

IUUSB::IUUSB(HardwareSerial *serialPort, char *charBuffer,
             uint16_t bufferSize, IUSerial::PROTOCOL_OPTIONS protocol,
             uint32_t rate, char stopChar, uint16_t dataReceptionTimeout) :
    IUSerial(serialPort, charBuffer, bufferSize, protocol, rate,
             stopChar, dataReceptionTimeout)
{

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
        if (((m_bufferIndex == 11 && strncmp(m_buffer, "IUCMD_START", 11) == 0) || (m_bufferIndex == 9 && strncmp(m_buffer, "IUCMD_END", 9) == 0))  ||
            ((m_bufferIndex == 10 && strncmp(m_buffer, "IUGET_DATA", 10) == 0)  || (m_bufferIndex == 10 && strncmp(m_buffer, "IUEND_DATA", 10) == 0))||
            (m_bufferIndex == 14 && strncmp(m_buffer, "IUGET_DEVICEID", 14) == 0)||
            (m_bufferIndex == 22 && strncmp(m_buffer, "IUGET_FIRMWARE_VERSION", 22) == 0)||
            (m_bufferIndex == 17 && strncmp(m_buffer, "IUGET_DEVICE_TYPE", 17) == 0)||
            (m_bufferIndex == 17 && strncmp(m_buffer, "IUGET_HTTP_CONFIG", 17) == 0)||
            (m_bufferIndex == 17 && strncmp(m_buffer, "IUGET_MQTT_CONFIG", 17) == 0)||
            (m_bufferIndex == 16 && strncmp(m_buffer, "IUGET_TCP_CONFIG", 16) == 0) ||
            (m_bufferIndex == 16 && strncmp(m_buffer, "IUGET_FFT_CONFIG", 16) == 0) ||
            (m_bufferIndex == 17 && strncmp(m_buffer, "IUGET_DEVICE_CONF", 17)== 0) ||
            (m_bufferIndex == 19 && strncmp(m_buffer, "IUGET_OTAFLAG_VALUE", 19)== 0) ||
            (m_bufferIndex == 16 && strncmp(m_buffer, "IUSET_OTAFLAG_00", 16)== 0) || 
            (m_bufferIndex == 16 && strncmp(m_buffer, "IUSET_OTAFLAG_01", 16)== 0) || 
            (m_bufferIndex == 16 && strncmp(m_buffer, "IUSET_OTAFLAG_02", 16)== 0) ||
            (m_bufferIndex == 16 && strncmp(m_buffer, "IUSET_OTAFLAG_03", 16)== 0) ||         
            (m_bufferIndex == 21 && strncmp(m_buffer, "IUSET_ERASE_EXT_FLASH", 21)== 0) ||
            (m_bufferIndex == 16 && strncmp(m_buffer, "IUGET_WIFI_TXPWR", 16)== 0) ||
            (m_bufferIndex == 19 && strncmp(m_buffer, "IUGET_MODBUS_CONFIG", 19)== 0) ||
            (m_bufferIndex == 16 && strncmp(m_buffer, "REMOVE_ESP_FILES", 16)== 0) ||  
            (m_bufferIndex == 12 && strncmp(m_buffer, "CERT-UPGRADE", 12)== 0) ||
            (m_bufferIndex == 17 && strncmp(m_buffer, "IUGET_WIFI_CONFIG", 17)== 0) ||
            (m_bufferIndex == 17 && strncmp(m_buffer, "IUGET_CERT_CONFIG", 17)== 0) || 
            (m_bufferIndex == 21 && strncmp(m_buffer, "IUSET_ERASE_INT_FLASH", 21)== 0) ||
            (m_bufferIndex == 9 && strncmp(m_buffer, "FLASH_ESP", 9)== 0) ||
            (m_bufferIndex == 16 && strncmp(m_buffer, "IUGET_DIG_CONFIG", 16)== 0) ||
            (m_bufferIndex == 18 && strncmp(m_buffer, "IUGET_PHASE_CONFIG", 18)== 0) ||
            (m_bufferIndex == 16 && strncmp(m_buffer, "IUGET_RPM_CONFIG", 16)== 0) ||
            (m_bufferIndex == 16 && strncmp(m_buffer, "IUGET_WIFI_MACID", 16)== 0) ) 
        {
            m_buffer[m_bufferIndex++] = 0;
            messageIsComplete = true;
        }
    }
    return messageIsComplete;
}
