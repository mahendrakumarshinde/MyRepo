#include "IURawDataHandler.h"


/* =============================================================================
    Core
============================================================================= */

char IURawDataHandler::EXPECTED_KEYS[
    IURawDataHandler::EXPECTED_KEY_COUNT + 1] = "XYZ";

char IURawDataHandler::ENDPOINT_HOST[45] =
    "ideplus-dot-infinite-uptime-1232.appspot.com";

char IURawDataHandler::ENDPOINT_URL[15] = "/raw_data?mac=";

IURawDataHandler::IURawDataHandler(uint32_t timeout) :
    m_payloadCounter(0),
    m_payloadStartTime(0),
    m_timeout(timeout)
{
    strcpy(m_payload, "");
    for (uint8_t i = 0; i < EXPECTED_KEY_COUNT; ++i)
    {
        m_keyAdded[i] = false;
    }
}


/* =============================================================================
    Payload construction
============================================================================= */

/**
 * Empty the payload and reset the associated counters and booleans
 */
void IURawDataHandler::resetPayload()
{
    strcpy(m_payload, "");
    m_payloadCounter = 0;
    m_payloadStartTime = 0;
    for (uint8_t i = 0; i < EXPECTED_KEY_COUNT; ++i)
    {
        m_keyAdded[i] = false;
    }
}

/**
 * Reset the payload if too much time passed since starting it.
 *
 * @return true if a timeout happened and that the payload has been reset,
 * else false.
 */
bool IURawDataHandler::hasTimedOut()
{
    if (m_payloadStartTime == 0)
    {
        return false;
    }
    if (m_payloadCounter > 0 && (millis() -
            m_payloadStartTime > m_timeout))
    {
        if (debugMode)
        {
            debugPrint("Raw data payload has timed out.");
        }
        return true;
    }
    return false;
}

/**
 * Add a key-value pair to the JSON payload.
 *
 * Function checks that:
 *  - given key is part of the expected keys
 *  - given keyhas not already been added.
 *  - the payload is large enough to add the key-value pair.
 *
 * @return true if the key, value pair was succesfully added, else false.
 */
bool IURawDataHandler::addKeyValuePair(char key, const char *value,
                                       uint16_t valueLength)
{
    char *foundKey = strchr(EXPECTED_KEYS, key);
    if (foundKey == NULL)
    {
        if (debugMode)
        {
            debugPrint("Raw data handler received unexpected key: ", false);
            debugPrint(key);
        }
        return false;
    }
    uint8_t idx = (uint8_t) (foundKey - EXPECTED_KEYS);
    if (m_keyAdded[idx])
    {
        if (debugMode)
        {
            debugPrint("Raw data handler received twice the key: ", false);
            debugPrint(key, false);
            debugPrint(" at idx: ", false);
            debugPrint(idx);
        }
        return false;
    }
    uint16_t available = MAX_PAYLOAD_LENGTH - m_payloadCounter - 1;
    if (valueLength + 7 > available)
    {
        if (debugMode)
        {
            debugPrint("Raw data handler - not enough available space in "
                       "payload", false);
            debugPrint(key);
        }
        return false;
    }
    if (m_payloadCounter == 0)
    {
        strcpy(m_payload, "{\"");
    }
    else
    {
        strncat(m_payload, ",\"", 2);
    }
    strncat(m_payload, &key, 1);
    strncat(m_payload, "\":\"", 3);
    strncat(m_payload, value, valueLength);
    char quote = '\"';
    strncat(m_payload, &quote, 1);
    m_payloadCounter += valueLength + 7;
    // Handle key duplication and timeout
    if (m_payloadStartTime == 0)
    {
        m_payloadStartTime = millis();
    }
    m_keyAdded[idx] = true;
    return true;
}

/**
 * Check whether all expected keys have been added to the payload.
 */
bool IURawDataHandler::areAllKeyPresent()
{
    for (uint8_t i = 0; i < EXPECTED_KEY_COUNT; ++i)
    {
        if (!m_keyAdded[i])
        {
            return false;
        }
    }
    return true;
}


/* =============================================================================
    HTTP Post request
============================================================================= */

/**
 * Post the payload if ready
 */
int IURawDataHandler::publishIfReady(const char *macAddress)
{
    if (hasTimedOut())
    {
        resetPayload();
        return false;
    }
    if (!areAllKeyPresent()) // Raw Data payload is not ready
    {
        return false;
    }
    int httpCode = httpPostPayload(macAddress);
    if (debugMode)
    {
        debugPrint("Post raw data: ", false);
        debugPrint(httpCode);
    }
    if (httpCode == 200)
    {
        resetPayload();
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * Post the payload
 */
int IURawDataHandler::httpPostPayload(const char *macAddress)
{
    // Close JSON first (last curled brace) if not closed yet
    char closingBrace = '}';
    if (m_payload[m_payloadCounter - 1] != closingBrace)
    {
        strncat(m_payload, &closingBrace, 1);
        m_payloadCounter++;
    }
    char fullUrl[strlen(ENDPOINT_URL) + 18];
    strcpy(fullUrl, ENDPOINT_URL);
    strncat(fullUrl, macAddress, 18);
    return httpPostBigJsonRequest(ENDPOINT_HOST, fullUrl, ENDPOINT_PORT,
                                  (uint8_t*) m_payload, m_payloadCounter);

//    const char *endpointHost, const char *endpointURL,
//    uint16_t endpointPort, uint8_t *payload, uint16_t payloadLength,
//    size_t chunkSize, uint16_t tcpTimeout=HTTPCLIENT_DEFAULT_TCP_TIMEOUT)
}


/* =============================================================================
    Instanciation
============================================================================= */


IURawDataHandler accelRawDataHandler(10000);  // 10s timeout

