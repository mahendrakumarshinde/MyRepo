#include "FeatureStreamingGroup.h"


/* =============================================================================
    Constructors & destructor
============================================================================= */

FeatureStreamingGroup::FeatureStreamingGroup(uint8_t id) :
    m_id(id),
    m_dataSendPeriod(FeatureStreamingGroup::defaultDataSendPeriod),
    m_lastSentTime(0)
{
    reset();
}

/**
 * Remove all the features from the group
 */
void FeatureStreamingGroup::reset()
{
    m_featureCount = 0;
    for (uint8_t i = 0; i < maxFeatureCount; ++i)
    {
        m_features[i] = NULL;
    }
}

/**
 * Add a feature in the group at the given index
 */
void FeatureStreamingGroup::add(uint8_t idx, Feature *feature)
{
    if (idx >= maxFeatureCount)
    {
        if (debugMode)
        {
            debugPrint(F("FeatureGroup: index out of range "), false);
            debugPrint(idx);
        }
    }
    m_features[idx] = feature;
    if (idx >= m_featureCount)
    {
        m_featureCount = idx + 1;
    }
    feature->enableStreaming();
}


/* =============================================================================
    Time management
============================================================================= */

/**
 * Set the data send interval.
 */
void FeatureStreamingGroup::setDataSendPeriod(uint16_t dataSendPeriod)
{
    m_dataSendPeriod = dataSendPeriod;
    if (debugMode)
    {
        debugPrint(F("Group "), false);
        debugPrint(m_id, false);
        debugPrint(F(": data send period set to "), false);
        debugPrint(m_dataSendPeriod);
    }
}

/**
 * Check if it is time to send data based on m_dataSendPeriod.
 *
 * NB: Data streaming is always disabled when in "sleep" operation mode.
 */
bool FeatureStreamingGroup::isDataSendTime()
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
void FeatureStreamingGroup::stream(HardwareSerial *port, OperationState::option opState,
                                   double timestamp)
{
    if (m_featureCount == 0 || !isDataSendTime())
    {
        return;
    }
    port->print(m_id);
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
