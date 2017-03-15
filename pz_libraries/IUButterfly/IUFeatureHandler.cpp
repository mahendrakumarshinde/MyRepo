#include "IUFeatureHandler.h"

IUFeatureHandler::IUFeatureHandler()
{
  resetFeatures()
}

IUFeatureHandler::~IUFeatureHandler()
{
  resetFeatures()
}

void IUFeatureHandler::resetFeatures()
{
  for (int i = 0; i < maxFeatureCount; i++)
  {
    delete m_features[i]; m_features[i] = NULL;
  }
  m_featureCount = 0;
}

/**
 * Attempt to add a feature to the collection
 * @return true if success, else false (because the collection is full)
 */
bool IUFeatureSelection::addFeature(IUABCFeature *feature)
{
    if (m_featureCount == maxFeatureCount)
    {
        return false; //Unable to add a new feature because the collection is full
    }
    m_features[m_featureCount] = feature;
    m_featureCount++;
    return true;
}
