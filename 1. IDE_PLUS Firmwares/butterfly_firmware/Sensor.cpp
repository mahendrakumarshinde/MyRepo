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
               Feature *destination2, Feature *destination3,
               Feature *destination4, Feature *destination5) :
    Component(),
    m_destinationCount(destinationCount)
{
    strcpy(m_name, name);
    m_destinations[0] = destination0;
    m_destinations[1] = destination1;
    m_destinations[2] = destination2;
    m_destinations[3] = destination3;
    m_destinations[4] = destination4;
    m_destinations[5] = destination5;
    setResolution(1); // Default resolution
    // Instance registration
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
 * Shows the sensor and its destination in IUDEBUG_ANY
 */
void Sensor::expose()
{
    #ifdef IUDEBUG_ANY
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
    Driven Sensor
============================================================================= */

HighFreqSensor::HighFreqSensor(const char* name, uint8_t destinationCount,
                               Feature *destination0, Feature *destination1,
                               Feature *destination2, Feature *destination3,
                               Feature *destination4, Feature *destination5) :
    Sensor(name, destinationCount, destination0, destination1, destination2,
           destination3, destination4, destination5)
{
    m_callbackRate = defaultCallbackRate;
    setSamplingRate(defaultSamplingRate);
}


/***** Configuration *****/

/**
 * Read and apply the config
 *
 * @param config A reference to a JsonVariant, ie a parsed JSON
 */
void HighFreqSensor::configure(JsonVariant &config)
{
    Sensor::configure(config);
    uint16_t samplingRate = config["FREQ"];
    if (samplingRate)
    {
        setSamplingRate(samplingRate);
    }
}



/***** Sampling and resolution *****/

/**
 * Rate cannot be set to zero + call computeDownclockingRate at the end
 */
void HighFreqSensor::setCallbackRate(uint16_t callbackRate)
{
    m_callbackRate = callbackRate;
    computeDownclockingRate();
}

/**
 * Rate cannot be set to zero + call computeDownclockingRate at the end
 */
void HighFreqSensor::setSamplingRate(uint16_t samplingRate)
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
void HighFreqSensor::computeDownclockingRate()
{
    m_downclocking = m_callbackRate / m_samplingRate;
    m_downclockingCount = 0;
}


/***** Data acquisition *****/

/**
 * Acquire new data, while handling down-clocking
 *
 * @param inCallback  set to true if the function is called from a callback.
 */
void HighFreqSensor::acquireData(bool inCallback, bool force)
{
    if (inCallback)
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
            if(!force && !m_destinations[i]->isReadyToRecord())
            {
                return;
            }
        }
        readData();
    }
}


/* =============================================================================
    Synchronous Sensor
============================================================================= */

LowFreqSensor::LowFreqSensor(const char* name, uint8_t destinationCount,
                             Feature *destination0, Feature *destination1,
                             Feature *destination2, Feature *destination3,
                             Feature *destination4, Feature *destination5) :
    Sensor(name, destinationCount, destination0, destination1, destination2,
           destination3, destination4, destination5),
    m_lastAcquisitionTime(0)
{
    setSamplingPeriod(defaultSamplingPeriod);
}


/***** Configuration *****/

/**
 * Read and apply the config
 *
 * @param config A reference to a JsonVariant, ie a parsed JSON
 */
void LowFreqSensor::configure(JsonVariant &config)
{
    Sensor::configure(config);
    JsonVariant value = config["POW"];
    if (value.success())
    {
        setPowerMode((PowerMode::option) (value.as<int>()));
    }
}

void LowFreqSensor::setSamplingPeriod(uint32_t samplingPeriod)
{
    m_samplingPeriod = samplingPeriod;
    for (uint8_t i = 0; i < getDestinationCount(); ++i)
    {
        /* TODO For now low freq sensors sends 0 as sampling rates to their
        receivers. Change this? Is it worth it? */
        m_destinations[i]->setSamplingRate(0);
    }
}


/***** Data acquisition *****/

/**
 * Acquire new data, while handling sampling period
 *
 * @param inCallback  set to true if the function is called from a callback.
 *  Note that in that case, data acquisition will be skipped as low freq sensors
 *  are not meant to be read in callbacks.
 */
void LowFreqSensor::acquireData(bool inCallback, bool force)
{
    if (inCallback)
    {
        return;  // Do not read low freq sensors in callback
    }
    uint32_t now = millis();
    if (now - m_lastAcquisitionTime > m_samplingPeriod ||
        m_lastAcquisitionTime == 0)  // Aquire at device start up
    {
        // Check if destinations are ready
        for (uint8_t i = 0; i < m_destinationCount; ++i)
        {
            if(!force && !m_destinations[i]->isReadyToRecord())
            {
                return ;
            }
        }
        readData();
        m_lastAcquisitionTime = now;
    }
}
