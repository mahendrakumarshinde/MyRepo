#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <WiFiUdp.h>

#include "Utilities.h"


class TimeManager
{
    public:
        // NTP time stamp is in the first 48 bytes of the message
        static const uint8_t NTP_PACKET_SIZE = 48;
        static const uint32_t NTP_REQUEST_TIMEOUT = 10000; //ms
        static const uint32_t TIME_UPDATE_INTERVAL = 300000;  // ms
        static const uint32_t SEVENTY_YEARS = 2208988800UL;
        /***** Core *****/
        TimeManager(uint16_t udpPort, const char* serverName);
        virtual ~TimeManager() { }
        void begin();
        void end();
        bool active() { return m_active; }
        void updateTimeReferenceFromNTP();
        void updateTimeReferenceFromIU(byte *payload,
                                       uint16_t payloadLength);
        time_t getCurrentTime();

    protected:
        /***** NTP request *****/
        bool m_active;
        uint16_t m_port;
        char  m_serverName[50];
        IPAddress m_serverIP;
        WiFiUDP m_udp;
        // buffer to hold incoming and outgoing packets
        byte m_packetBuffer[NTP_PACKET_SIZE];
        bool m_requestSent;
        uint32_t m_requestSentTime;
        /***** Time tracking *****/
        time_t m_timeReference;
        uint32_t m_lastTimeUpdate;
        /***** Internal methods *****/
        void sendNTPpacket();
        bool readNTPpacket();
};


/***** Instanciation *****/

extern TimeManager timeManager;

#endif // TIMEMANAGER_H
