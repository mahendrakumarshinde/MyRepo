#ifndef IURAWDATAHANDLER_H
#define IURAWDATAHANDLER_H

#include "Utilities.h"

/**
 *
 */
class IURawDataHandler
{
    public:
        /***** Public constants *****/
        static const uint16_t MAX_PAYLOAD_LENGTH = 10000;
        // After timeout, payload is cleared even if it has not been sent
        static const uint32_t TIMEOUT = 30000;  // 30s
        //Max number of distinct part to be added to payload
        sattic const uint8_t MAX_PART_COUNT = 3;
        /***** Core *****/
        IURawDataHandler(uint8_t partcount, );
        virtual ~IURawDataHandler() {}
        void resetPayload();
        void


    protected:
        char m_payload[MAX_PAYLOAD_LENGTH];
        uint16_t m_payloadCounter;
        uint32_t m_payloadStartTime;  // For timing out
        uint8_t m_partCount;
        bool m_partAdded[MAX_PART_COUNT]
};

#endif // IURAWDATAHANDLER_H
