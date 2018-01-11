#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <time.h>


/* =============================================================================
    Debugging and testing definitions
============================================================================= */

// Define TEST_TOPICS to use the PubSub test topics (instead of prod ones)
//#define TEST_TOPICS

// Define DEBUGMODE to enable debug level messages from the whole firmware
//#define DEBUGMODE

#ifdef DEBUGMODE
    const bool debugMode = true;
#else
    const bool debugMode = false;
#endif


/* =============================================================================
    PubSub topic names
============================================================================= */

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


/* =============================================================================
    Debugging utilities
============================================================================= */


template <typename T>
inline void debugPrint(T msg, bool endline = true)
{
    #ifdef DEBUGMODE
    if (endline) { Serial.println(msg); }
    else { Serial.print(msg); }
    #endif
}

//Specialization for float printing with fixed decimal count
template <>
inline void debugPrint(float msg, bool endline)
{
    #ifdef DEBUGMODE
    if (endline) { Serial.println(msg, 6); }
    else { Serial.print(msg, 6); }
    #endif
}

template <typename T>
inline void raiseException(T msg)
{
    #ifdef DEBUGMODE
    debugPrint(F("Error: "), false);
    debugPrint(msg);
    #endif
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
inline int httpPostRequest(const char *url, char *payload,
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


/* =============================================================================
    OTA functions
============================================================================= */



#endif // UTILITIES_H
