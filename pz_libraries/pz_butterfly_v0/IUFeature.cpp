#include "IUFeatureConfig.h"

/* ================== Definitions for IUFeature class ==================== */
IUFeature::IUFeature(uint8_t id) : m_id(id), m_name(""), m_active(false), m_state(0), m_highestDangerLevel(0)
{
  m_computeFunction = &default_compute;
  m_sourceSize = 0;
  m_source = NULL;
  m_destinationSize = 0;
  m_destination = NULL;
  setThresholds(0, 0, 0);
}

/**
 * Prepare the feature to receive data (receive function can then be used)
 * The feature will be store data and be calculated once the data buffer is full.
 * The data buffer is sourceSize long.
 */
void setSource(uint16_t sourceSize)
{
    m_sourceSize = sourceSize;
    q15_t source[sourceSize];
    m_source = &source;
}

/**
 * Prepare the feature to store its results
 * A feature can store up to destinationSize calculated value
 */
void setDestination(uint16_t destinationSize)
{
    m_destinationSize = destinationSize;
    q15_t destination[destinationSize];
    m_destination = destination;
}

/**
 * Set the feature as active: it will then be calculated and streamed
 * @return true if the feature was correctly activated, else false
 * NB: The feature cannot be activated if the source or destination have
 * not been set.
 */
bool activate()
{
    if (m_source && m_destination)
    {
        m_active = true;
    }
    else
    {
        m_active = false;
    }
    return m_active;
}

void IUFeature::setThresholds(float normal, float warning, float danger)
{
    m_threshold[0] = normal;
    m_threshold[1] = warning;
    m_threshold[2] = danger;
}



/**
 * Compute the feature value, store it and return it
 */
float IUFeature::compute()
{
    m_value = m_computeFunction();
    return m_value;
}

/**
 * Update and return the state of the feature in regards to defined thresholds
 * NB: Also update highestDangerLevel if needed.
 * @return 0 is not cutting, 1 is normal, 2 is warning, 3 is danger
 */
uint8_t IUFeature::getThresholdState() {
  if (m_value < m_threshold[0]) {
    m_state = 0; // level 0: not cutting
  }
  else if (m_value < m_threshold[1]) {
    m_state = 1; // level 1: normal cutting
  }
  else if (m_value < m_threshold[2]) {
    m_state = 2;  // level 2: warning cutting
  }
  else {
    m_state = 3;  // level 3: bad cutting
  }
  m_highestDangerLevel = max(m_highestDangerLevel, m_state);
  return m_state;
}


/* ================ Definitions for IUFeatureCollection class ================= */

/**
 * Collection of features is initially empty
 */
IUFeatureCollection::IUFeatureCollection() : m_size(0)
{
  //ctor
}

/**
 * Attempt to add a feature to the collection
 * @return true if success, else false (because the collection is full)
 */
bool IUFeatureCollection::addFeature(IUFeature feature)
{
    if (m_size == MAX_SIZE)
    {
        return false; //Unable to add a new feature because the collection is full
    }
    m_features[m_size] = feature;
    m_size++;
    return true;
}

/**
 * Create a IUFeature instance and add it to the collection
 * @return true if success, else false (because the collection is full)
 */
bool IUFeatureCollection::addFeature(String name, float (*computeFunction) ())
{
    if (m_size == MAX_SIZE)
    {
        return false; //Unable to add a new feature because the collection is full
    }
    feature = IUFeature(name, computeFunction)
    m_features[m_size] = feature;
    m_size++;
    return true;
}

IUFeature IUFeatureCollection::getFeature(uint8_t index)
{
    if (index < m_size)
    {
        return m_features[index];
    }
    // Else... Unable to throw exception with Arduino
}

