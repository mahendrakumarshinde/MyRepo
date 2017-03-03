#include "IUFeatureConfig.h"

/* ================== Definitions for IUFeature class ==================== */
IUFeature::IUFeature() : m_name(""), m_value(0), m_state(0), m_highestDangerLevel(0)
{
  //ctor
  m_computeFunction = &default_compute;
  setThresholds(0, 0, 0);
}

IUFeature::IUFeature(String name, float (*computeFunction) ()) :  m_name(name), m_value(0), m_state(0), m_highestDangerLevel(0)
{
    m_computeFunction = computeFunction;
    setThresholds(0, 0, 0);
}

void IUFeature::setThresholds(float normal, float warning, float danger)
{
    m_normalThreshold = normal;
    m_warningThreshold = warning;
    m_dangerThreshold = danger;
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
  if (m_value < m_normalThreshold) {
    m_state = 0; // level 0: not cutting
  }
  else if (m_value < m_warningThreshold) {
    m_state = 1; // level 1: normal cutting
  }
  else if (m_value < m_dangerThreshold) {
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

