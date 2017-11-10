#include "Sensor.h"


/* =============================================================================
    Generic Sensor class
============================================================================= */

uint8_t Sensor::instanceCount = 0;

Sensor *Sensor::instances[Sensor::MAX_INSTANCE_COUNT] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/***** Core *****/

Sensor::Sensor(const char* name, uint8_t destinationCount,
               Feature *destination0, Feature *destination1,
               Feature *destination2) :
    Component(),
    m_destinationCount(destinationCount)
{
    strcpy(m_name, name);
    m_destinations[0] = destination0;
    m_destinations[1] = destination1;
    m_destinations[2] = destination2;
    setResolution(1); // Default resolution
    // Instance registration
    if (debugMode)
    {
        Sensor *existing = getInstanceByName(name);
        if (existing != NULL)
        {
            debugPrint(F("WARNING - Duplicate sensor name "), false);
            debugPrint(name);
        }
        if (instanceCount >= MAX_INSTANCE_COUNT)
        {
            raiseException("Max sensor count exceeded");
        }
    }
    m_instanceIdx = instanceCount;
    instances[m_instanceIdx] = this;
    instanceCount++;
}

Sensor::~Sensor()
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

Sensor *Sensor::getInstanceByName(const char *name)
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


/***** Sampling and resolution *****/

void Sensor::setResolution(float resolution)
{
    m_resolution = resolution;
    for (uint8_t i = 0; i < getDestinationCount(); ++i)
    {
        m_destinations[i]->setResolution(m_resolution);
    }
}


/***** Debugging *****/

/**
 * Shows the sensor and its destination in DEBUGMODE
 */
void Sensor::expose()
{
    #ifdef DEBUGMODE
    debugPrint(F("Sensor "), false);
    debugPrint(getName(), false);
    debugPrint(F(" has "), false);
    debugPrint(m_destinationCount, false);
    debugPrint(F(" receivers:"));
    for (uint8_t i = 0; i < m_destinationCount; ++i)
    {
        debugPrint(F("  "), false);
        debugPrint(m_destinations[i]->getName());
    }
    #endif
}


/* =============================================================================
    Asynchronous Sensor
============================================================================= */

AsynchronousSensor::AsynchronousSensor(const char* name,
                                       uint8_t destinationCount,
                                       Feature *destination0,
                                       Feature *destination1,
                                       Feature *destination2) :
    Sensor(name, destinationCount, destination0, destination1, destination2)
{
    m_callbackRate = defaultCallbackRate;
    setSamplingRate(defaultSamplingRate);
}


/***** Configuration *****/

/**
 * Read and apply the config
 *
 * @param config A reference to a JsonVariant, ie a parsed JSON
 * @return True if a config was found and applied for the sensor, else false
 */
bool AsynchronousSensor::configure(JsonVariant &config)
{

    JsonVariant my_config = config[m_name];
    if (!my_config)
    {
        return false;
    }
    uint16_t samplingRate = my_config["FREQ"];
    if (samplingRate)
    {
        setSamplingRate(samplingRate);
    }
    return Sensor::configure(config);
}



/***** Sampling and resolution *****/

/**
 * Rate cannot be set to zero + call computeDownclockingRate at the end
 */
void AsynchronousSensor::setCallbackRate(uint16_t callbackRate)
{
    m_callbackRate = callbackRate;
    computeDownclockingRate();
}

/**
 * Rate cannot be set to zero + call computeDownclockingRate at the end
 */
void AsynchronousSensor::setSamplingRate(uint16_t samplingRate)
{
    m_samplingRate = samplingRate;
    for (uint8_t i = 0; i < getDestinationCount(); ++i)
    {
        m_destinations[i]->setSamplingRate(m_samplingRate);
    }
    computeDownclockingRate();
}

/**
 * Compute the downclocking rate from the callback and sampling rates
 *
 * Should be called every time the callback or sampling rates are modified.
 */
void AsynchronousSensor::computeDownclockingRate()
{
    m_downclocking = m_callbackRate / m_samplingRate;
    m_downclockingCount = 0;
}


/***** Data acquisition *****/

/**
 * Acquire new data, while handling down-clocking
 */
void AsynchronousSensor::acquireData()
{
    m_downclockingCount++;
    if (m_downclockingCount < m_downclocking)
    {
        return;
    }
    m_downclockingCount = 0;
    // Check if destinations are ready
    for (uint8_t i = 0; i < m_destinationCount; ++i)
    {
        if(!m_destinations[i]->isReadyToRecord())
        {
            return;
        }
    }
    readData();
}


/* =============================================================================
    Synchronous Sensor
============================================================================= */

SynchronousSensor::SynchronousSensor(const char* name,
                                     uint8_t destinationCount,
                                     Feature *destination0,
                                     Feature *destination1,
                                     Feature *destination2) :
    Sensor(name, destinationCount, destination0, destination1, destination2),
    m_usagePreset(SynchronousSensor::defaultUsagePreset),
    m_lastAcquisitionTime(0)
{
    setSamplingPeriod(defaultSamplingPeriod);
}


/***** Configuration *****/

/**
 * Read and apply the config
 *
 * @param config A reference to a JsonVariant, ie a parsed JSON
 * @return True if a config was found and applied for the sensor, else false
 */
bool SynchronousSensor::configure(JsonVariant &config)
{

    JsonVariant my_config = config[m_name];
    if (!my_config)
    {
        return false;
    }
    JsonVariant value = my_config["USG"];
    if (value.success())
    {
        changeUsagePreset((usagePreset) (value.as<int>()));
    }
    return Sensor::configure(config);
}

/**
 * Change the usagePreset
 *
 * @param usage A usagePreset (Low, Regular, Enhanced, High)
 */
void SynchronousSensor::changeUsagePreset(Sensor::usagePreset usage)
{
    switch (m_usagePreset)
    {
    case Sensor::P_LOW:
        switchToLowUsage();
        break;
    case Sensor::P_REGULAR:
        switchToRegularUsage();
        break;
    case Sensor::P_ENHANCED:
        switchToEnhancedUsage();
        break;
    case Sensor::P_HIGH:
        switchToHighUsage();
        break;
    default:
        if (debugMode)
        {
            debugPrint(F("Unknown usagePreset "), false);
            debugPrint(usage);
        }
    }
}


void SynchronousSensor::setSamplingPeriod(uint32_t samplingPeriod)
{
    m_samplingPeriod = samplingPeriod;
    for (uint8_t i = 0; i < getDestinationCount(); ++i)
    {
        /* TODO For now synchronous sensors sends 0 as sampling rates to their
        receivers. Change this? Is it worth it? */
        m_destinations[i]->setSamplingRate(0);
    }
}


/***** Data acquisition *****/

/**
 * Acquire new data, while handling sampling period
 */
void SynchronousSensor::acquireData()
{
    uint32_t now = millis();
    if (m_lastAcquisitionTime + m_samplingPeriod > now
            && m_lastAcquisitionTime < now )  // Handle millis() overflow
    {
        return;
    }
    // Check if destinations are ready
    for (uint8_t i = 0; i < m_destinationCount; ++i)
    {
        if(!m_destinations[i]->isReadyToRecord())
        {
            return ;
        }
    }
    readData();
    m_lastAcquisitionTime = now;
}
