#include "IURawDataHelper.h"


/* =============================================================================
    Core
============================================================================= */

char IURawDataHelper::EXPECTED_KEYS[IURawDataHelper::EXPECTED_KEY_COUNT + 1] =
    "XYZ";


IURawDataHelper::IURawDataHelper(
    uint32_t inputTimeout, uint32_t postTimeout, const char *endpointHost,
    const char *endpointRoute, uint16_t enpointPort) :
    m_endpointPort(enpointPort),
    m_payloadCounter(0),
    m_payloadStartTime(0),
    m_inputTimeout(inputTimeout),
    m_firstPostTime(0),
    m_postTimeout(postTimeout)
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
    m_firstPostTime = 0;
    for (uint8_t i = 0; i < EXPECTED_KEY_COUNT; ++i)
    {
        m_keyAdded[i] = false;
    }
}

/**
 * Tells whether too much time passed since starting it receiving inputs.
 *
 * @return true if a timeout happened and that the payload needs to be
 * reset, else false.
 */
bool IURawDataHelper::inputHasTimedOut()
{
    if (m_payloadStartTime == 0)
    {
        return false;
    }
    if (m_payloadCounter > 0 && (millis() -
            m_payloadStartTime > m_inputTimeout))
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
 * Tells whether too much time passed since starting it receiving inputs.
 *
 * @return true if a timeout happened and that the payload needs to be
 * reset, else false.
 */
bool IURawDataHelper::postPayloadHasTimedOut()
{
    if (m_firstPostTime == 0)
    {
        return false;
    }
    if (millis() - m_firstPostTime > m_postTimeout)
    {
        if (debugMode)
        {
            debugPrint("Raw data post payload has expired.");
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
    m_payloadCounter += 2;
    m_payload[m_payloadCounter++] = key;
    m_payload[m_payloadCounter++] = '"';
    m_payload[m_payloadCounter++] = ':';
    m_payload[m_payloadCounter++] = '"';
//    strncat(m_payload, &key, 1);
//    strncat(m_payload, "\":\"", 3);
    strncat(m_payload, value, valueLength);
    m_payloadCounter += valueLength;
    m_payload[m_payloadCounter++] = '"';
//    char quote = '\"';
//    strncat(m_payload, &quote, 1);
//    m_payloadCounter += valueLength + 7;
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
    if (inputHasTimedOut() || postPayloadHasTimedOut())
    {
        resetPayload();
        return false;
    }
    if (!areAllKeyPresent()) // Raw Data payload is not ready
    {
        return false;
    }
    m_firstPostTime = millis();
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
    if (m_payload[m_payloadCounter - 1] != '}')
    {
        m_payload[m_payloadCounter++] = '}';
    }
    if (debugMode)
    {
        debugPrint("Post payload: ", false);
        debugPrint(m_payload);
    }
    char fullUrl[strlen(m_endpointRoute) + 18];
    strcpy(fullUrl, m_endpointRoute);
    strncat(fullUrl, macAddress.toString().c_str(), 18);
    if (debugMode)
    {
        debugPrint("Host: ", false);
        debugPrint(m_endpointHost);
        debugPrint("Port: ", false);
        debugPrint(m_endpointPort);
        debugPrint("URL: ", false);
        debugPrint(fullUrl);
    }
    return httpPostBigJsonRequest(m_endpointHost, fullUrl, m_endpointPort,
                                  (uint8_t*) m_payload, m_payloadCounter);
}
