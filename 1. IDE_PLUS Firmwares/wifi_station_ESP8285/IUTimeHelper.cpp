#include "IUTimeHelper.h"

/* =============================================================================
    Core
============================================================================= */

IUTimeHelper::IUTimeHelper(uint16_t udpPort, const char* serverName) :
    m_active(false),
    m_port(udpPort),
    m_requestSent(false),
    m_requestSentTime(0),
    m_timeReference(0),
    m_lastTimeUpdate(0)
{
    strcpy(m_serverName, serverName);
}

/**
 *
 */
void IUTimeHelper::begin()
{
    if (m_active)
    {
        return;
    }
    m_udp.begin(m_port);
    m_active = true;
    if (debugMode)
    {
        debugPrint("Starting UDP at port: ", false);
        debugPrint(m_udp.localPort());
    }
}

/**
 *
 */
void IUTimeHelper::end()
{
    if (!m_active)
    {
        return;
    }
    m_udp.stop();
    m_active = false;
}

/**
 * Update the Time from an NTP server, while prioritizing IU server time.
 *
 * As long as we keep receiving time from IU server, this function will do
 * nothing (see IME_UPDATE_INTERVAL for max delay before update from NTP
 * server).
 */
void IUTimeHelper::updateTimeReferenceFromNTP()
{
    if (!m_active)
    {
        return;
    }
    uint32_t current = millis();
    if (m_requestSent) // If request sent, always try to read the response
    {
        bool readSuccess = readNTPpacket();
        if (readSuccess)
        {
            m_lastTimeUpdate = current;
            m_requestSent = false;
        }
        else if (current > m_requestSentTime + NTP_REQUEST_TIMEOUT ||
                 current < m_requestSentTime)  // Handle millis overflow
        {
            // Handle request timeout
            if (debugMode)
            {
                debugPrint("NTP request has timed out");
            }
            m_requestSent = false;
        }
        // else request not yet received but not timed out either
    }
    else  // If no request sent, only send a new one when it's time
    {
        if (m_lastTimeUpdate == 0 ||
            current > m_lastTimeUpdate + TIME_UPDATE_INTERVAL ||
            m_lastTimeUpdate > current)  // Handle millis overflow
        {
            sendNTPpacket();
            m_requestSent = true;
        }
    }
}

/**
 *
 */
void IUTimeHelper::updateTimeReferenceFromIU(byte *payload,
                                            uint16_t payloadLength)
{
    const char *buff = reinterpret_cast<const char*>(payload);
    int32_t ts = atol(&buff[2]);
    if (ts > 0)
    {
        m_timeReference = (time_t) ts;
        m_lastTimeUpdate = millis();
        if (debugMode)
        {
            debugPrint("Updated time ref from IU msg: ", false);
            debugPrint(ctime(&m_timeReference));
        }
    }
}

time_t IUTimeHelper::getCurrentTime()
{
    return m_timeReference + (time_t) ((millis() - m_lastTimeUpdate) / 1000);
}


/* =============================================================================
    Internal methods
============================================================================= */

/**
 * Send an NTP request to the time server
 */
void IUTimeHelper::sendNTPpacket()
{
    if (debugMode)
    {
        debugPrint("sending NTP packet... ", false);
    }
    WiFi.hostByName(m_serverName, m_serverIP);
    // set all bytes in the buffer to 0
    memset(m_packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    m_packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    m_packetBuffer[1] = 0;     // Stratum, or type of clock
    m_packetBuffer[2] = 6;     // Polling Interval
    m_packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    m_packetBuffer[12]  = 49;
    m_packetBuffer[13]  = 0x4E;
    m_packetBuffer[14]  = 49;
    m_packetBuffer[15]  = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    m_udp.beginPacket(m_serverIP, 123); // NTP requests are to port 123
    m_udp.write(m_packetBuffer, NTP_PACKET_SIZE);
    m_udp.endPacket();
    if (debugMode)
    {
        debugPrint("Done.");
    }
}

/**
 * Read NTP packet coming from the time server
 */
bool IUTimeHelper::readNTPpacket()
{
    if (debugMode)
    {
        debugPrint("Reading NTP packet... ", false);
    }
    int cb = m_udp.parsePacket();
    if (!cb)  // No response yet
    {
        if (debugMode)
        {
            debugPrint("No packet yet");
        }
        return false;
    }
    else  // Got the response
    {
        if (debugMode)
        {
            debugPrint("Packet received, length=", false);
            debugPrint(cb);
        }
        m_udp.read(m_packetBuffer, NTP_PACKET_SIZE);
        /* Timestamp starts at byte 40 of the received packet and is four
        bytes long. */
        uint32_t ntpTime = ((uint32_t) m_packetBuffer[40]) << 24 |
                           ((uint32_t) m_packetBuffer[41]) << 16 |
                           ((uint32_t) m_packetBuffer[42]) << 8 |
                           ((uint32_t) m_packetBuffer[43]);
        // NTP time is seconds since Jan 1 1900
        m_timeReference = (time_t) (ntpTime - SEVENTY_YEARS);
        if (debugMode)
        {
            debugPrint("NTP time: ", false);
            debugPrint(ntpTime);
            debugPrint("Unix time: ", false);
            debugPrint(m_timeReference);
            debugPrint("UTC datetime: ", false);
            debugPrint(ctime(&m_timeReference));
        }
        return true;
    }
}
