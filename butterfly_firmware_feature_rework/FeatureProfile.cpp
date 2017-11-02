#include "FeatureProfile.h"


/* =============================================================================
    Constructors & destructor
============================================================================= */

FeatureProfile::FeatureProfile(const char *name, uint16_t dataSendPeriod) :
    m_dataSendPeriod(dataSendPeriod),
    m_lastSentTime(0)
{
    strcpy(m_name, name);
    reset();
}

/**
 * Remove all the features from the group
 */
void FeatureProfile::reset()
{
    m_featureCount = 0;
    for (uint8_t i = 0; i < maxFeatureCount; ++i)
    {
        m_features[i] = NULL;
    }
}

/**
 * Append a feature to the profile
 */
void FeatureProfile::addFeature(Feature *feature)
{
    if (m_featureCount >= maxFeatureCount)
    {
        if (debugMode)
        {
            debugPrint(F("FeatureProfile: too many features"));
        }
    }
    m_features[m_featureCount] = feature;
    m_featureCount++;
    feature->enableStreaming();
}


/* =============================================================================
    Time management
============================================================================= */

/**
 * Set the data send interval.
 */
void FeatureProfile::setDataSendPeriod(uint16_t dataSendPeriod)
{
    m_dataSendPeriod = dataSendPeriod;
    if (debugMode)
    {
        debugPrint(F("Group "), false);
        debugPrint(m_name, false);
        debugPrint(F(": data send period set to "), false);
        debugPrint(m_dataSendPeriod);
    }
}

/**
 * Check if it is time to send data based on m_dataSendPeriod.
 *
 * NB: Data streaming is always disabled when in "sleep" operation mode.
 */
bool FeatureProfile::isDataSendTime()
{
    // Timer to send data regularly
    uint32_t now = millis();
    if (m_lastSentTime > now)
    {
        m_lastSentTime = 0; // Manage millis uint32_t overflow
    }
    if (m_lastSentTime + m_dataSendPeriod >= now)
    {
        m_lastSentTime = now;
        return true;
    }
    return false;
}


/* =============================================================================
    Communication
============================================================================= */

/**
 * Sends the values of the group features through given serial.
 */
void FeatureProfile::stream(HardwareSerial *port,
                            OperationState::option opState,
                            double timestamp)
{
    if (!m_active)
    {
        return;  // Only stream if profile is active
    }
    if (m_featureCount == 0 || !isDataSendTime())
    {
        return;
    }
    port->print(m_name);
    port->print(",");
    port->print(timestamp);
    port->print(",");
    port->print(opState);
    for (uint8_t i = 0; i < m_featureCount; ++i)
    {
        if (m_features[i] != NULL)
        {
            m_features[i]->stream(port);
        }
    }
    port->print(";");
    if (loopDebugMode)
    {
        // Add a line break to ease readability in debug mode
        port->println("");
    }
    port->flush();
}
