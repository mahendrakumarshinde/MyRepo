#include "FeatureGroup.h"


/* =============================================================================
    Constructors & destructor
============================================================================= */

uint8_t FeatureGroup::instanceCount = 0;

FeatureGroup *FeatureGroup::instances[FeatureGroup::MAX_INSTANCE_COUNT] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

FeatureGroup::FeatureGroup(const char *name, uint16_t dataSendPeriod) :
    m_active(false),
    m_dataSendPeriod(dataSendPeriod),
    m_lastSentTime(0)
{
    strcpy(m_name, name);
    reset();
    // Instance registration
    m_instanceIdx = instanceCount;
    instances[m_instanceIdx] = this;
    instanceCount++;
}

FeatureGroup::~FeatureGroup()
{
    instances[m_instanceIdx] = NULL;
    for (uint8_t i = m_instanceIdx + 1; i < instanceCount; ++i)
    {
        instances[i]->m_instanceIdx--;
        instances[i -1] = instances[i];
    }
    instances[instanceCount] = NULL;
    instanceCount--;
}

FeatureGroup *FeatureGroup::getInstanceByName(const char *name)
{
    for (uint8_t i = 0; i < instanceCount; ++i)
    {
        if (instances[i] != NULL)
        {
            if(instances[i]->isNamed(name))
            {
                return instances[i];
            }
        }
    }
    return NULL;
}


/***** Core *****/

/**
 * Remove all the features from the group
 */
void FeatureGroup::reset()
{
    m_featureCount = 0;
    for (uint8_t i = 0; i < maxFeatureCount; ++i)
    {
        m_features[i] = NULL;
    }
}

/**
 * Append a feature to the group
 */
void FeatureGroup::addFeature(Feature *feature)
{
    if (m_featureCount >= maxFeatureCount)
    {
        if (debugMode)
        {
            debugPrint(F("FeatureGroup: too many features"));
        }
    }
    m_features[m_featureCount] = feature;
    m_featureCount++;
}


/* =============================================================================
    Time management
============================================================================= */

/**
 * Set the data send interval.
 */
void FeatureGroup::setDataSendPeriod(uint16_t dataSendPeriod)
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
bool FeatureGroup::isDataSendTime()
{
    // Timer to send data regularly
    uint32_t now = millis();
    if (m_lastSentTime > now)
    {
        m_lastSentTime = 0; // Manage millis uint32_t overflow
    }
    if (m_lastSentTime + m_dataSendPeriod < now)
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
void FeatureGroup::stream(HardwareSerial *port, const char *macAddress,
                          double timestamp, bool sendMACAddress)
{
    if (!m_active)
    {
        return;  // Only stream if group is active
    }
    if (m_featureCount == 0 || !isDataSendTime())
    {
        return;
    }
    port->print(m_name);
    if (sendMACAddress)
    {
        port->print(",");
        port->print(macAddress);
    }
    for (uint8_t i = 0; i < m_featureCount; ++i)
    {
        if (m_features[i] != NULL)
        {
            port->print(",");
            port->print(m_features[i]->getName());
            m_features[i]->stream(port);
        }
    }
    port->print(",");
    port->print(timestamp);
    port->print(";");
    if (loopDebugMode)
    {
        // Add a line break to ease readability in debug mode
        port->println("");
    }
}


void FeatureGroup::legacyStream(HardwareSerial *port, const char *macAddress,
                                OperationState::option opState,
                                float batteryLoad, double timestamp,
                                bool sendName)
{
    if (!m_active)
    {
        return;  // Only stream if group is active
    }
    if (m_featureCount == 0 || !isDataSendTime())
    {
        return;
    }
    if (sendName)
    {
        port->print(m_name);
        port->print(",");
    }
    port->print(macAddress);
    port->print(",0");
    port->print((uint8_t) opState);
    port->print(",");
    port->print((int) round(batteryLoad));
    for (uint8_t i = 0; i < m_featureCount; ++i)
    {
        port->print(",000");
        port->print(i + 1);
        if (m_features[i] != NULL)
        {
            m_features[i]->stream(port);
        }
    }
    port->print(",");
    port->print(timestamp);
    port->print(";");
    if (loopDebugMode)
    {
        port->println("");
    }
}


/* =============================================================================
    Instantiation
============================================================================= */

// Health Check
FeatureGroup healthCheckGroup("HEALTH", 45000);
// Calibration
FeatureGroup calibrationGroup("CAL001", 512);
// Raw acceleration data
FeatureGroup rawAccelGroup("RAWACC", 512);
// Standard Press Monitoring
FeatureGroup pressStandardGroup("PRSSTD", 512);
// Standard Motor Monitoring
FeatureGroup motorStandardGroup("MOTSTD", 512);


/***** Populate groups *****/

void populateFeatureGroups()
{
    /** Health Check **/
    healthCheckGroup.addFeature(&batteryLoad);
    // TODO => configure HealthCheckGroup

    /** Calibration **/
    calibrationGroup.addFeature(&velRMS512X);
    calibrationGroup.addFeature(&velRMS512Y);
    calibrationGroup.addFeature(&velRMS512Z);
    calibrationGroup.addFeature(&temperature);
    calibrationGroup.addFeature(&accelMainFreqX);
    calibrationGroup.addFeature(&accelMainFreqY);
    calibrationGroup.addFeature(&accelMainFreqZ);
    calibrationGroup.addFeature(&accelRMS512X);
    calibrationGroup.addFeature(&accelRMS512Y);
    calibrationGroup.addFeature(&accelRMS512Z);

    /** Raw acceleration data **/
    rawAccelGroup.addFeature(&accelerationX);
    rawAccelGroup.addFeature(&accelerationY);
    rawAccelGroup.addFeature(&accelerationZ);

    /** Standard Press Monitoring **/
    pressStandardGroup.addFeature(&accelRMS512Total);
    pressStandardGroup.addFeature(&accelRMS512X);
    pressStandardGroup.addFeature(&accelRMS512Y);
    pressStandardGroup.addFeature(&accelRMS512Z);
    pressStandardGroup.addFeature(&temperature);
    pressStandardGroup.addFeature(&audioDB4096);

    /** Standard Motor Monitoring **/
    motorStandardGroup.addFeature(&accelRMS512Total);
    motorStandardGroup.addFeature(&velRMS512X);
    motorStandardGroup.addFeature(&velRMS512Y);
    motorStandardGroup.addFeature(&velRMS512Z);
    motorStandardGroup.addFeature(&temperature);
    motorStandardGroup.addFeature(&audioDB4096);
}
