#include "Sensor.h"

using namespace IUComponent;


/* =============================================================================
    Constructors and destructors
============================================================================= */

Sensor::Sensor(uint8_t id, uint8_t destinationCount,
               FeatureBuffer *destination0, FeatureBuffer *destination1,
               FeatureBuffer *destination2) :
    ABCComponent(),
    m_id(id),
    m_destinationCount(destinationCount)
{
    m_destinations[0] = destination0;
    m_destinations[1] = destination1;
    m_destinations[2] = destination2;
}


/* =============================================================================
    Sampling and resolution
============================================================================= */

/**
 * Rate cannot be set to zero + call prepareDataAcquisition at the end
 */
void Sensor::setCallbackRate(uint16_t callbackRate)
{
    if (callbackRate == 0)
    {
        m_callbackRate = defaultCallbackRate;
    }
    else
    {
        m_callbackRate = callbackRate;
    }
    prepareDataAcquisition();
}

/**
 * Rate cannot be set to zero + call prepareDataAcquisition at the end
 */
void Sensor::setSamplingRate(uint16_t samplingRate)
{
    if (samplingRate == 0)
    {
        m_samplingRate = defaultSamplingRate;
    }
    else
    {
        m_samplingRate = samplingRate;
    }
    for (uint8_t i = 0; i < getDestinationCount(); ++i)
    {
        m_destinations[i]->setSamplingRate(m_samplingRate);
    }
    prepareDataAcquisition();
}

void Sensor::setResolution(uint16_t resolution)
{
    m_resolution = resolution;
    for (uint8_t i = 0; i < getDestinationCount(); ++i)
    {
        m_destinations[i]->setResolution(m_resolution);
    }
}


/* =============================================================================
    Data acquisition
============================================================================= */


/**
 * Prepare data acquisition by setting up the sampling rates
 *
 * Should be called every time the callback or sampling rates are modified.
 */
void Sensor::prepareDataAcquisition()
{
    m_downclocking = m_callbackRate / m_samplingRate;
    m_downclockingCount = 0;
}

/**
 * Acquire new data and transmit it to receivers, while handling down-clocking
 *
 * @return true if new data were acquired, else false
 */
bool Sensor::acquireData()
{
    m_downclockingCount++;
    if (m_downclocking != m_downclockingCount)
    {
        return false;
    }
    m_downclockingCount = 0;
    readData();
    for (uint8_t i = 0; i < m_destinationCount; ++i)
    {
        m_destinations[i]->setSamplingRate(m_samplingRate);
        m_destinations[i]->setResolution(m_resolution);
    }
    return true;
}
