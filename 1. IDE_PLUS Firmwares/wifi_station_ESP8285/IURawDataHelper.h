#ifndef IURAWDATAHELPER_H
#define IURAWDATAHELPER_H

#include <MacAddress.h>
#include "Utilities.h"
#include "BoardDefinition.h"

/**
 * A class to create and publish a raw data JSON via an HTTP POST request.
 *
 * The class is typically instanciated with an empty payload, that is then
 * completed with (key (char), value (char array)) pairs.
 * Expected keys are given at instanciation, allowing the class to check that
 * the JSON is complete and that there is no key repetition.
 *
 * @param timeout  duration in ms after which the raw data payload is cleared,
 *  even if it has not been sent
 */
class IURawDataHelper
{
    public:
        /***** Public constants *****/
        static const uint16_t MAX_PAYLOAD_LENGTH = 10000;
        // Expected keys for the JSON creation of keys (at top level).
        static const uint8_t EXPECTED_KEY_COUNT = 3;
        static char EXPECTED_KEYS[EXPECTED_KEY_COUNT + 1];
        /***** Core *****/
        IURawDataHelper(uint32_t inputTimeout, uint32_t postTimeout,
                        const char *endpointHost, const char *endpointRoute,
                        uint16_t enpointPort);
        virtual ~IURawDataHelper() {}
        /***** Endpoint *****/
        void setEndpointHost(char *host)
            { strncpy(m_endpointHost, host, MAX_HOST_LENGTH); }
        void setEndpointRoute(char *route)
            { strncpy(m_endpointRoute, route, MAX_ROUTE_LENGTH); }
        void setEndpointPort(uint16_t port) { m_endpointPort = port; }
        /***** Payload construction *****/
        virtual void resetPayload();
        virtual bool inputHasTimedOut();
        virtual bool postPayloadHasTimedOut();
        virtual bool addKeyValuePair(char key, const char *value,
                                     uint16_t valueLength);
        virtual bool areAllKeyPresent();
        /***** HTTP Post request *****/
        virtual int publishIfReady(MacAddress macAddress);
        virtual int publishJSON(MacAddress macAddress,char* value, uint16_t valueLength);
        //virtual int publishBigJSON(char* values);
        virtual String publishConfigMessage(MacAddress macAddress);

    protected:
        /***** Endpoint *****/
        char m_endpointHost[MAX_HOST_LENGTH];
        char m_endpointRoute[MAX_ROUTE_LENGTH];
        uint16_t m_endpointPort;
        /***** Payload handling *****/
        char m_payload[MAX_PAYLOAD_LENGTH];
        uint16_t m_payloadCounter;
        uint32_t m_payloadStartTime;  // For timing out
        bool m_keyAdded[EXPECTED_KEY_COUNT];
        uint32_t m_inputTimeout;
        uint32_t m_firstPostTime;
        uint32_t m_postTimeout;
        /***** HTTP Post request *****/
        virtual int httpPostPayload(MacAddress macAddress);
        virtual String httpGetPayload(MacAddress macAddress);
};

#endif // IURAWDATAHELPER_H
