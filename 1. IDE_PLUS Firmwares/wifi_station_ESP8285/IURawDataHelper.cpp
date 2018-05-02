#include "IURawDataHelper.h"


/* =============================================================================
    Core
============================================================================= */

char IURawDataHelper::EXPECTED_KEYS[IURawDataHelper::EXPECTED_KEY_COUNT + 1] =
    "XYZ";


IURawDataHelper::IURawDataHelper(uint32_t timeout, const char *endpointHost,
                                 const char *endpointRoute,
                                 uint16_t enpointPort) :
    m_endpointPort(enpointPort),
    m_payloadCounter(0),
    m_payloadStartTime(0),
    m_timeout(timeout)
{
    strncpy(m_endpointHost, endpointHost, MAX_HOST_LENGTH);
    strncpy(m_endpointRoute, endpointRoute, MAX_ROUTE_LENGTH);
    resetPayload();
}


/* =============================================================================
    Payload construction
============================================================================= */

/**
 * Empty the payload and reset the associated counters and booleans
 */
void IURawDataHelper::resetPayload()
{
    for (uint16_t i = 0; i < MAX_PAYLOAD_LENGTH; i++)
    {
        m_payload[i] = 0;
    }
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
bool IURawDataHelper::hasTimedOut()
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
bool IURawDataHelper::addKeyValuePair(char key, const char *value,
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
bool IURawDataHelper::areAllKeyPresent()
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
int IURawDataHelper::publishIfReady(MacAddress macAddress)
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
int IURawDataHelper::httpPostPayload(MacAddress macAddress)
{
    // Close JSON first (last curled brace) if not closed yet
    char closingBrace = '}';
    if (m_payload[m_payloadCounter - 1] != closingBrace)
    {
        strncat(m_payload, &closingBrace, 1);
        m_payloadCounter++;
    }
    char fullUrl[strlen(m_endpointRoute) + 18];
    strcpy(fullUrl, m_endpointRoute);
    strncat(fullUrl, macAddress.toString().c_str(), 18);
    return httpPostBigJsonRequest(m_endpointHost, fullUrl, m_endpointPort,
                                  (uint8_t*) m_payload, m_payloadCounter);
}
