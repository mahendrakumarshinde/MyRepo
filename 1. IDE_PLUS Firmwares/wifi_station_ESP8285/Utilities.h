#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>
#include <time.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "Logger.h"


/* =============================================================================
    Device designation
============================================================================= */

// TODO Find a better place for this
const uint8_t DEVICE_TYPE_LENGTH = 9;
const char DEVICE_TYPE[DEVICE_TYPE_LENGTH] = "ide_plus";


/* =============================================================================
    PubSub topic names
============================================================================= */

// Define TEST_TOPICS to use the PubSub test topics (instead of prod ones)
//#define TEST_TOPICS

#ifdef TEST_TOPICS
    const uint8_t FEATURE_TOPIC_LENGTH = 20;
    const uint8_t DIAGNOSTIC_TOPIC_LENGTH = 14;
//    const uint8_t RAW_DATA_TOPIC_LENGTH = 17;
    const char FEATURE_TOPIC[FEATURE_TOPIC_LENGTH] = "iu_device_data_test";
    const char DIAGNOSTIC_TOPIC[DIAGNOSTIC_TOPIC_LENGTH] = "iu_error_test";
//    const char RAW_DATA_TOPIC[RAW_DATA_TOPIC_LENGTH] = "iu_raw_data_test";
#else
    const uint8_t FEATURE_TOPIC_LENGTH = 15;
    const uint8_t DIAGNOSTIC_TOPIC_LENGTH = 9;
//    const uint8_t RAW_DATA_TOPIC_LENGTH = 12;
    const char FEATURE_TOPIC[FEATURE_TOPIC_LENGTH] = "iu_device_data";
    const char DIAGNOSTIC_TOPIC[DIAGNOSTIC_TOPIC_LENGTH] = "iu_error";
//    const char RAW_DATA_TOPIC[RAW_DATA_TOPIC_LENGTH] = "iu_raw_data";
#endif  // TEST_TOPICS

// TODO Move this in IUMQTTHelper
const uint8_t CUSTOMER_PLACEHOLDER_LENGTH = 9;
const char CUSTOMER_PLACEHOLDER[CUSTOMER_PLACEHOLDER_LENGTH] = "XXXAdmin";

const char DEFAULT_WILL_MESSAGE[44] =
    "XXXAdmin;;;00:00:00:00:00:00;;;disconnected";


/* =============================================================================
    Multi part message validator
============================================================================= */

template<uint8_t N>
class MultiMessageValidator
{
    public:
        /***** *****/
        MultiMessageValidator() : MultiMessageValidator(0) { }
        MultiMessageValidator(uint32_t timeout) :
            m_timeout(timeout), m_startTime(0) {
            for (uint8_t i = 0; i < N; ++i)
            {
                m_messageReceived[i] = false;
            }
        }
        virtual ~MultiMessageValidator() { }
        /***** *****/
        void setTimeout(uint32_t timeout) { m_timeout = timeout; }
        void reset();
        bool started();
        bool completed();
        void receivedMessage(uint8_t index);
        bool hasTimedOut();

    protected:
        bool m_messageReceived[N];
        uint32_t m_timeout;
        uint32_t m_startTime;

};

template<uint8_t N>
void MultiMessageValidator<N>::reset()
{
    for (uint8_t i = 0; i < N; ++i)
    {
        m_messageReceived[i] = false;
    }
}

template<uint8_t N>
bool MultiMessageValidator<N>::started()
{
    for (uint8_t i = 0; i < N; ++i)
    {
        if (m_messageReceived[i])
        {
            return true;
        }
    }
    return false;
}

template<uint8_t N>
bool MultiMessageValidator<N>::completed()
{
    for (uint8_t i = 0; i < N; ++i)
    {
        if (!m_messageReceived[i])
        {
            return false;
        }
    }
    return true;
}

template<uint8_t N>
void MultiMessageValidator<N>::receivedMessage(uint8_t index)
{
    if (index >= N)
    {
        if (debugMode)
        {
            raiseException("Index out of bound: N=", false);
            debugPrint(N, false);
            debugPrint(", k=", false);
            debugPrint(index);
        }
        return;
    }
    if (!started())
    {
        m_startTime = millis();
    }
    m_messageReceived[index] = true;
}

template<uint8_t N>
bool MultiMessageValidator<N>::hasTimedOut()
{
    uint32_t current = millis();
    return (m_timeout > 0 && current - m_startTime > m_timeout);
}


/* =============================================================================
    HTTP functions
============================================================================= */

/**
 * Sends an HTTP GET request - HTTPS is used if fingerprint is given.
 *
 * @param url
 * @param responseBody
 * @param maxResponseLength
 * @param httpsFingerprint
 */
inline int httpGetRequest(const char *url, char* responseBody,
                          uint16_t maxResponseLength,
                          const char *httpsFingerprint=NULL)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        if (debugMode)
        {
            debugPrint("WiFi disconnected: GET request failed");
        }
        return 0;
    }
    HTTPClient http;
    if (httpsFingerprint)
    {
        http.begin(String(url), String(httpsFingerprint));
    }
    else
    {
        http.begin(String(url));
    }
    int httpCode = http.GET();
    if (httpCode > 0)
    {
        http.getString().toCharArray(responseBody, maxResponseLength);
        if (debugMode)
        {
            debugPrint(responseBody);
        }
    }
    http.end();
    return httpCode;
}

/**
 * Sends an HTTP POST request - HTTPS is used if fingerprint is given.
 *
 * @param url
 * @param payload
 * @param payloadLength
 * @param responseBody
 * @param maxResponseLength
 * @param httpsFingerprint
 */
inline int httpPostJsonRequest(const char *url, char *payload,
                               uint16_t payloadLength, char* responseBody,
                               uint16_t maxResponseLength,
                               const char *httpsFingerprint=NULL)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        if (debugMode)
        {
            debugPrint("WiFi disconnected: POST request failed");
        }
        return 0;
    }
    HTTPClient http;
    if (httpsFingerprint)
    {
        http.begin(String(url), String(httpsFingerprint));
    }
    else
    {
        http.begin(String(url));
    }
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST((uint8_t*) payload, (size_t) payloadLength);
    if (httpCode > 0)
    {
        http.getString().toCharArray(responseBody, maxResponseLength);
        if (debugMode)
        {
            debugPrint(responseBody);
        }
    }
    http.end();
    return httpCode;
}


/**
 * Sends an HTTP POST request with a big JSON
 *
 * "Big" JSON means that its size is above the max packet size which is
 * around 3KB (2920 bytes in my experience).
 *
 * @param endpointHost
 * @param endpointURL
 * @param payload
 * @param payloadLength
 * @param chunkSize should at most WIFICLIENT_MAX_PACKET_SIZE
 *  (= HTTP_TCP_BUFFER_SIZE), currently 1460 bytes.
 * @param tcpTimeout
 * @return the HTTP code, or 0 if connection couldn't be established,
 * or a negative number for HTTPClient errors (see HTTPC_ERROR in
 * ESP8266HTTPClient.h)
 */
inline int httpPostBigJsonRequest(
    const char *endpointHost, const char *endpointURL,
    uint16_t endpointPort, uint8_t *payload, uint16_t payloadLength,
    size_t chunkSize=WIFICLIENT_MAX_PACKET_SIZE,
    uint16_t tcpTimeout=HTTPCLIENT_DEFAULT_TCP_TIMEOUT)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        if (debugMode)
        {
            debugPrint("WiFi disconnected: POST request failed");
        }
        return 0;
    }
    // create the request and headers
    String request = "POST " + String(endpointURL) + " HTTP/1.1\r\n" +
        "Host: " + String(endpointHost) + "\r\n" +
        "Accept: application/json" + "\r\n" +
        "Content-Type: application/json\r\n" +
        "Content-Length: " + String(payloadLength) + "\r\n\r\n";
    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    int connectResult = client.connect(endpointHost, endpointPort);
    if (connectResult == 0)
    {
        if (debugMode)
        {
            debugPrint("Couldn't connect to host: ", false);
            debugPrint(endpointHost, false);
            debugPrint(":", false);
            debugPrint(endpointPort);
            debugPrint("\nHEADERS:");
            debugPrint(request);
        }
        return connectResult;  // 0 means no connection
    }

    // This will send the request and headers to the server
    client.print(request);
    // now we need to chunk the payload into 1000 byte chunks
    size_t cIndex;
    size_t retSize;
    for (cIndex = 0; cIndex < payloadLength - chunkSize;
         cIndex = cIndex + chunkSize)
    {
        retSize = client.write(&payload[cIndex], chunkSize);
        if(retSize != chunkSize)
        {
            if (client.connected() || (client.available() > 0))
            {
                client.stop();
            }
            return HTTPC_ERROR_SEND_PAYLOAD_FAILED;  // -3
        }
    }
    retSize = client.write(&payload[cIndex], payloadLength - cIndex);
    if(retSize != payloadLength - cIndex)
    {
        if (client.connected() || (client.available() > 0))
        {
            client.stop();
        }
        return HTTPC_ERROR_SEND_PAYLOAD_FAILED;  // -3
    }
    // Handle response
    uint32_t lastDataTime = millis();
    int returnCode;
    while(client.connected() || client.available())
    {
        size_t len = client.available();
        if(len > 0)
        {
            String headerLine = client.readStringUntil('\n');
            headerLine.trim(); // remove \r
            lastDataTime = millis();

            if(headerLine.startsWith("HTTP/1.")) {
                returnCode = headerLine.substring(
                    9, headerLine.indexOf(' ', 9)).toInt();
            }
            if(headerLine == "")
            {
                if(returnCode)
                {
                    return returnCode;
                }
                else
                {
                    return HTTPC_ERROR_NO_HTTP_SERVER;  // -7
                }
            }
        }
        else
        {
            if((millis() - lastDataTime) > tcpTimeout) {
                return HTTPC_ERROR_READ_TIMEOUT;  // -11
            }
            delay(0);
        }
    }
    return HTTPC_ERROR_CONNECTION_LOST;  // -5
}


#endif // UTILITIES_H
