#include "IUFeatureConfigurator.h"


/* ========================= Feature Configuration ================================ */
/*
Short name coding
char 1: feature type
- A for acceleration
- M for Magnetometer
- G for Gyroscope
- V for Velocity
- T for Temperature
- P for Pressure
- S for Sound
char 2: axis or source count
- X, Y, Z for axis when it makes senses
- 1 for 1 source, 3 for 3 sources when it make senses (eg on 3 axis)
char 3: sample sizes
- 0 for size 1
- N for 64 * 2^N (eg: 1 => 128, 3 => 512, 5 => 2048, 6 => 4096)
*/

IUFeatureConfigurator::FeatureConfig IUFeatureConfigurator::noConfig =
{
  "", "",
  {"", {0, "", ""}, {0, "", ""}},
  {"", {0, "", ""}, {0, "", ""}}
};

IUFeatureConfigurator::FeatureConfig IUFeatureConfigurator::registeredConfigs[IUFeatureConfigurator::registeredCount] =
  {
    { // 0
      "AX1", "accelEnergy_X_128",
      {"IUSingleAxisEnergyFeature128", {1, "A", "0"}, {0, "", ""}},
      {"", {0, "", ""}, {0, "", ""}}
    },
    { // 1
      "AY1", "accelEnergy_Y_128",
      {"IUSingleAxisEnergyFeature128", {1, "A", "1"}, {0, "", ""}},
      {"", {0, "", ""}, {0, "", ""}}
    },
    { // 2
      "AZ1", "accelEnergy_Z_128",
      {"IUSingleAxisEnergyFeature128", {1, "A", "2"}, {0, "", ""}},
      {"", {0, "", ""}, {0, "", ""}}
    },
    { // 3
      "A31", "accelEnergy_3_128",
      {"IUTriAxisEnergyFeature128", {3, "A-A-A", "0-1-2"}, {0, "", ""}},
      {"IUTriSourceSummingFeature", {0, "", ""}, {3, "AX1-AY1-AZ1", "0-0-0"}}
    },
    { // 4
      "AX3", "accelEnergy_X_512",
      {"IUSingleAxisEnergyFeature512", {1, "A", "0"}, {0, "", ""}},
      {"", {0, "", ""}, {0, "", ""}}
    },
    { // 5
      "AY3", "accelEnergy_Y_512",
      {"IUSingleAxisEnergyFeature512", {1, "A", "1"}, {0, "", ""}},
      {"", {0, "", ""}, {0, "", ""}}
    },
    { // 6
      "AZ3", "accelEnergy_Z_512",
      {"IUSingleAxisEnergyFeature512", {1, "A", "2"}, {0, "", ""}},
      {"", {0, "", ""}, {0, "", ""}}
    },
    { // 7
      "A33", "accelEnergy_3_512",
      {"IUTriAxisEnergyFeature512", {3, "A-A-A", "0-1-2"}, {0, "", ""}},
      {"IUTriSourceSummingFeature", {0, "", ""}, {3, "AX3-AY3-AZ3", "0-0-0"}}
    },
    { // 8
      "VX3", "velocity_X_512",
      {"IUVelocityFeature512", {1, "A", "0"}, {1, "AX3", "0"}},
      {"", {0, "", ""}, {0, "", ""}}
    },
    { // 9
      "VY3", "velocity_Y_512",
      {"IUVelocityFeature512", {1, "A", "1"}, {1, "AY3", "0"}},
      {"", {0, "", ""}, {0, "", ""}}
    },
    { // 10
      "VZ3", "velocity_Z_512",
      {"IUVelocityFeature512", {1, "A", "2"}, {1, "AZ3", "0"}},
      {"", {0, "", ""}, {0, "", ""}}
    },
    { // 11
      "T10", "temperature_1_1",
      {"IUDefaultFloatFeature", {1, "T", "0"}, {0, "", ""}},
      {"", {0, "", ""}, {0, "", ""}}
    },
    { // 12
      "S15", "audioDB_1_2048",
      {"IUAudioDBFeature2048", {1, "S", "0"}, {0, "", ""}},
      {"", {0, "", ""}, {0, "", ""}}
    },
    { // 13
      "S16", "audioDB_1_4096",
      {"IUAudioDBFeature4096", {1, "S", "0"}, {0, "", ""}},
      {"", {0, "", ""}, {0, "", ""}}
    },
  };

String IUFeatureConfigurator::standardConfig = "A33-VX3-VY3-VZ3-T10-S16";
float IUFeatureConfigurator::standardThresholds[6][3] = {{30, 600, 1200},
                                                         {0.05, 1.2, 1.8},
                                                         {0.05, 1.2, 1.8},
                                                         {0.05, 1.2, 1.8},
                                                         {200, 205, 210},
                                                         {500, 1000, 1500}};
bool IUFeatureConfigurator::standardFeatureCheck[6] = {true, false, false, false, false, false};

String IUFeatureConfigurator::pressConfig = "A33-A31-T10-S16";
float IUFeatureConfigurator::pressThresholds[4][3] = {{30, 600, 1200},
                                                      {30, 600, 1200},
                                                      {200, 205, 210},
                                                      {500, 1000, 1500}};
bool IUFeatureConfigurator::pressFeatureCheck[4] = {true, false, false, false};

/* ========================= Method definitions  ================================ */

IUFeatureConfigurator::IUFeatureConfigurator() :
  m_featureCount(0),
  m_secondaryFeatureCount(0)
{
  for (int i = 0; i < maxFeatureCount; i++)
  {
    m_features[i] = NULL;
  }
  for (int i = 0; i < maxSecondaryFeatureCount; i++)
  {
    m_secondaryFeatures[i] = NULL;
  }
}

IUFeatureConfigurator::~IUFeatureConfigurator()
{
  resetFeatures();
}

/**
 * Delete all primary and secondary features
 */
void IUFeatureConfigurator::resetFeatures()
{
  for (int i = 0; i < maxFeatureCount; i++)
  {
    if(m_features[i])
    {
      delete m_features[i]; m_features[i] = NULL;
    }
  }
  m_featureCount = 0;
  for (int i = 0; i < maxSecondaryFeatureCount; i++)
  {
    if(m_secondaryFeatures[i])
    {
      delete m_secondaryFeatures[i]; m_secondaryFeatures[i] = NULL;
    }
  }
  m_secondaryFeatureCount = 0;
}

/**
 * Get the feature registered config from the feature's name
 * @param featureName   the feature name
 * @return              the feature config, or IUFeatureConfigurator::noConfig if the name was not found
 */
IUFeatureConfigurator::FeatureConfig IUFeatureConfigurator::getConfigFromName(String featureName)
{
  if (featureName == "") 
  {
    if (debugMode) { debugPrint("Tried to look for an empty config name, returned NoConfig"); }
    return noConfig;
  }
  for (uint8_t i = 0; i < registeredCount; i++)
  {
    if (registeredConfigs[i].name == featureName)
    {
      return registeredConfigs[i];
    }
  }
  if (debugMode) { debugPrint(featureName + " is not a registered config name"); }
  return noConfig;
}

/**
 * Get several feature configs from their names as string
 * @param featureNames   the feature names as a single string, separated by IUFeatureConfigurator::nameSeparator
 * @param config         a pointer to the destination array to get the feature configs
 * @return               the number of registered features found
 */
uint8_t IUFeatureConfigurator::getConfigFromName(String featureNames, IUFeatureConfigurator::FeatureConfig *configs)
{
  String parsedFeatNames[registeredCount];
  uint8_t subStringCount = splitString(featureNames, nameSeparator, parsedFeatNames, registeredCount);
  FeatureConfig cfg = noConfig;
  uint8_t counter = 0;
  for (uint8_t i = 0; i < subStringCount; i++)
  {
    cfg = getConfigFromName(parsedFeatNames[i]);
    if (cfg.name != "") // is not noConfig?
    {
      configs[counter] = cfg;
      counter++;
    }
  }
  return counter;
}

/**
 * Register given feature as receiver of its dependency features
 * @param feature           the feature to register as receiver
 */
bool IUFeatureConfigurator::registerFeatureInFeatureDependencies(IUFeature *feature)
{
  bool success = true;
  Serial.println("OK 10");
  Serial.println(feature->getName());
  Serial.println("OK 10 10");
  FeatureConfig config = getConfigFromName(feature->getName());
  if (config.name == "")
  {
    return false;
  }
  FeatureSubConfig fss = feature->isFromSecondaryConfig() ? config.alt : config.def;
  FeatureConfig depConf[registeredCount];
  Serial.println("OK 11");
  Serial.println(fss.featureDep.names);
  uint8_t depCount = getConfigFromName(fss.featureDep.names, depConf);
  if (depCount != fss.featureDep.dependencyCount)
  {
    if (debugMode) { debugPrint("Invalid configuration: dependency count does not match dependency names"); }
    return false;
  }
  int depSendingOptions[registeredCount];
  uint8_t depSendOptCount = splitStringToInt(fss.featureDep.sendingOptions, nameSeparator, depSendingOptions, registeredCount);
  if (depSendOptCount != fss.featureDep.dependencyCount)
  {
    if (debugMode) { debugPrint("Invalid configuration: dependency count does not match dependency options"); }
    return false;
  }
  uint8_t reservedSourceForSensor = fss.sensorDep.dependencyCount;
  for (uint8_t i = 0; i < fss.featureDep.dependencyCount; i++)
  {
    Serial.println("OK 12");
    Serial.println(depConf[i].name);
    IUFeature *producerFeature = getFeatureByName(depConf[i].name);
    success = producerFeature->addReceiver(depSendingOptions[i], reservedSourceForSensor + i, feature);
    if (!success)
    {
      if (debugMode) { debugPrint("Failed to add feature as dependent of another feature"); }
      return false;
    }
  }
  return success;
}

/**
 * Register given feature as receiver of given component
 * @param feature           the feature to register as receiver
 * @param sensor            array of pointer to the sensors
 * @param sensoCount        the number of sensors
 */
bool IUFeatureConfigurator::registerFeatureInSensor(IUFeature *feature, IUABCSensor **sensors, uint8_t sensorCount)
{
  FeatureConfig config = getConfigFromName(feature->getName());
  if (config.name == "")
  {
    if (debugMode) { debugPrint("Configuration not found"); }
    return false;
  }
  FeatureSubConfig fss = feature->isFromSecondaryConfig() ? config.alt : config.def;
  String foundSensorTypes[registeredCount];
  uint8_t foundSensorCount = splitString(fss.sensorDep.names, nameSeparator, foundSensorTypes, registeredCount);
  if (foundSensorCount != fss.sensorDep.dependencyCount)
  {
    if (debugMode) { debugPrint("Invalid configuration: dependency count does not match dependency names"); }
    return false;
  }
  int depSendingOptions[registeredCount];
  uint8_t depSendOptCount = splitStringToInt(fss.sensorDep.sendingOptions, nameSeparator, depSendingOptions, registeredCount);
  if (depSendOptCount != fss.sensorDep.dependencyCount)
  {
    if (debugMode) { debugPrint("Invalid configuration: dependency count does not match dependency options"); }
    return false;
  }
  bool success = true;
  for (uint8_t i = 0; i < fss.sensorDep.dependencyCount; i++)
  {
    for (uint8_t j = 0; j < sensorCount; j++)
    {
      uint8_t count = sensors[j]->getSensorTypeCount();
      for (uint8_t k = 0; k < count; k++)
      {
        if (foundSensorTypes[i][0] == sensors[j]->getSensorType(k))
        {
          success &= sensors[j]->addReceiver(depSendingOptions[i], i, feature);
        }
      }
    }
  }
  if (!success && debugMode) { debugPrint("Failed to register feature as receiver of given sensors"); }
  return success;
}

/**
 * Register given feature as receiver of given component
 * @param feature           the feature to register as receiver
 * @param sensor            array of pointer to the sensors
 * @param sensoCount        the number of sensors
 */
void IUFeatureConfigurator::registerAllFeaturesInSensor(IUABCSensor **sensors, uint8_t sensorCount)
{
  for (uint8_t i = 0; i < m_secondaryFeatureCount; i++)
  {
    registerFeatureInSensor(m_secondaryFeatures[i], sensors, sensorCount);
  }
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    registerFeatureInSensor(m_features[i], sensors, sensorCount);
  }
}

/**
 * Create a feature following a IUFeatureConfigurator::FeatureConfig while handling dependencies (create them if needed)
 *
 * Function adds all the created feature to this instance, and call registerFeatureInFeatureDependencies
 * to handle feature inter-dependencies.
 * If alternative feature creation is possible, AND THAT ALL alternative feature dependencies
 * are ALREADY created, the feature will be created with alternative config.
 * @param config      the config to create the feature
 * @param id          the id that will be given to the new feature - default to 0
 * @param secondary   false (default) if the feature to create is primary (user requested)
 *                    true if the feature to create is secondary (needed for dependency)
 */
int IUFeatureConfigurator::createWithDependencies(IUFeatureConfigurator::FeatureConfig config, uint8_t id, bool secondary)
{
  bool success = true;
  // Check alternative config dependencies: If there is an alternative and that dependencies already exists, we will use them!
  FeatureConfig altDep[registeredCount];
  int altDepCount = getConfigFromName(config.alt.featureDep.names, altDep);
  bool alternative = config.alt.className && (altDepCount > 0); // True if there are possible alternative dependencies
  for (uint8_t i = 0; i < altDepCount; i++)                     // Check that all alternative dependencies feature have been created
  {
    alternative &= (getFeatureByName(altDep[i].name) != NULL);  // Stay true only if all alternative dep exists
  }
  Serial.println("OK 5");
  if (!alternative)
  {
    Serial.println("OK 6");
    // If not alternative, we will use the default config: we then have to create the dependency features
    FeatureConfig defDep[registeredCount];
    int defDepCount = getConfigFromName(config.def.featureDep.names, defDep);
    for (uint8_t i = 0; i < defDepCount; i++)
    {
      if (debugMode) { debugPrint(config.name + " requires dependency: " + defDep[i].name); }
      // Check if the dependency exists and, if not, create it
      if(getFeatureByName(defDep[i].name) == NULL)
      {
        success = createWithDependencies(defDep[i], 0, true);
        if (!success)
        {
          if (debugMode) { debugPrint("Failed to create " + config.name + " dependencies"); }
          return false;
        }
        else if (debugMode)
        {
          debugPrint("Created " + defDep[i].name + " feature as dependency.");
        }
      }
      else if (debugMode)
      {
        debugPrint(defDep[i].name + " feature already exists, it will be used as dependency.");
      }
    }
  }
  Serial.println("OK 7");
  IUFeature *feature = createFeature(config, id, alternative);
  success = addFeature(feature, secondary);
  Serial.println("OK 8");
  if (!success)
  {
    if (feature) { delete feature; }
    if (debugMode) { debugPrint(config.name + " incorrectly created or added to the configurator (the feature list may be full)"); }
    return false;
  }
  Serial.println("OK 9");
  // Register the feature as receiver of its feature dependencies and return "success" boolean
  return registerFeatureInFeatureDependencies(feature);
}

/**
 * Parse given config string and create all features and dependencies as specified
 * @param configBuffer  the config to implement as String of feature names, separated
 *                      by NameSeparator (see static members names and NameSeparator)
 */
bool IUFeatureConfigurator::requireConfiguration(String configBufffer)
{
  resetFeatures();
  bool success = false;
  FeatureConfig requiredConfigs[maxFeatureCount];
  int requiredConfigCount = getConfigFromName(configBufffer, requiredConfigs);
  for (uint8_t i = 0; i < requiredConfigCount; i++)
  {
    Serial.println("OK 1");
    success = createWithDependencies(requiredConfigs[i], i, false);
    Serial.println("OK 1000");
    if (!success) { return false; }
    if (debugMode) { debugPrint(requiredConfigs[i].name + " successfully created"); }
  }
  resetFeaturesCounters();
  // Check that the number of primary features is the same as required
  return requiredConfigCount == m_featureCount;
}

/**
 * Do a standard setup from pre-programmed configuration in FeatureConfigurator
 */
bool IUFeatureConfigurator::doStandardSetup()
{
  if (!requireConfiguration(standardConfig))
  {
    return false;
  }
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    m_features[i]->setThresholds(standardThresholds[i][0], standardThresholds[i][1], standardThresholds[i][2]);
    m_features[i]->setFeatureCheck(standardFeatureCheck[i]);
  }
  return true;
}

/**
 * Do the setup for presses from pre-programmed configuration in FeatureConfigurator
 */
bool IUFeatureConfigurator::doPressSetup()
{
  if (!requireConfiguration(pressConfig))
  {
    return false;
  }
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    m_features[i]->setThresholds(pressThresholds[i][0], pressThresholds[i][1], pressThresholds[i][2]);
    m_features[i]->setFeatureCheck(pressFeatureCheck[i]);
  }
  return true;
}

/**
 * Create a feature from given config and return a pointer to it
 * @param config        the config to create the feature
 * @param id            the id that will be given to the new feature (optional, default to 0)
 * @param alternative   if false (default), use default subconfig; if true, use alternative subconfig
 */
IUFeature* IUFeatureConfigurator::createFeature(IUFeatureConfigurator::FeatureConfig featConfig, uint8_t id, bool alternative)
{
  IUFeature *feature = NULL;
  String className;
  if(alternative)
  {
    className = featConfig.alt.className;
  }
  else
  {
    className = featConfig.def.className;
  }
  if(className == "IUSingleAxisEnergyFeature128")
  {
    feature = new IUSingleAxisEnergyFeature128(id, featConfig.name, featConfig.fullName, alternative);
  }
  else if (className == "IUTriAxisEnergyFeature128")
  {
    feature = new IUTriAxisEnergyFeature128(id, featConfig.name, featConfig.fullName, alternative);
  }
  else if (className == "IUSingleAxisEnergyFeature512")
  {
    feature = new IUSingleAxisEnergyFeature512(id, featConfig.name, featConfig.fullName, alternative);
  }
  else if (className == "IUTriAxisEnergyFeature512")
  {
    feature = new IUTriAxisEnergyFeature512(id, featConfig.name, featConfig.fullName, alternative);
  }
  else if (className == "IUVelocityFeature512")
  {
    feature = new IUVelocityFeature512(id, featConfig.name, featConfig.fullName, alternative);
  }
  else if (className == "IUDefaultFloatFeature")
  {
    feature = new IUDefaultFloatFeature(id, featConfig.name, featConfig.fullName, alternative);
  }
  else if (className == "IUAudioDBFeature2048")
  {
    feature = new IUAudioDBFeature2048(id, featConfig.name, featConfig.fullName, alternative);
  }
  else if (className == "IUAudioDBFeature4096")
  {
    feature = new IUAudioDBFeature4096(id, featConfig.name, featConfig.fullName, alternative);
  }
  else if (className == "IUTriSourceSummingFeature")
  {
    feature = new IUTriSourceSummingFeature(id, featConfig.name, featConfig.fullName, alternative);
  }
  else if (debugMode)
  {
    debugPrint("Feature class name is not listed in createFeature function");
   }
  if (feature)
  {
    feature->prepareSource();
    feature->setDefaultComputeFunction();
    if (!feature->activate())
    {
      if (debugMode) { debugPrint("Feature couldn't be activated: check the source and default function"); }
      delete feature; feature = NULL;
    }
  }
  return feature;
}

/**
 * Create a feature from given registered config index and return a pointer to it
 * @param configIdx     the index of the registered feature config
 * @param id            the feature id (optional, default to 0)
 * @param alternative   if false (default), use default config; if true, use alternative config
 */
IUFeature* IUFeatureConfigurator::createFeature(uint8_t configIdx, uint8_t id, bool alternative)
{
  return createFeature(registeredConfigs[configIdx], id, alternative);
}

/**
 * Create a feature from given name and return a pointer to it
 * @param name          the feature name
 * @param id            the feature id (optional, default to 0)
 * @param alternative   if false (default), use default config; if true, use alternative config
 */
IUFeature* IUFeatureConfigurator::createFeature(String name, uint8_t id, bool alternative)
{
  return createFeature(getConfigFromName(name), id, alternative);
}

/**
 * Get the feature from "main" features, or secondary features if not found in main
 */
IUFeature* IUFeatureConfigurator::getFeatureByName(String name)
{
  // Check in "main" features
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    if (m_features[i]->getName() == name)
    {
      return m_features[i];
    }
  }
  // Check in secondary features
  for (uint8_t i = 0; i < m_secondaryFeatureCount; i++)
  {
    if (m_secondaryFeatures[i]->getName() == name)
    {
      return m_secondaryFeatures[i];
    }
  }
  return NULL;
}

/**
 * Add a main or secondary feature to the configurator
 * @param feature     the feature to add.
 * @param secondary   if false, add feature to the main feature list.
 *                    if true, add feature to the secondary feature list.
 */
bool IUFeatureConfigurator::addFeature(IUFeature *feature, bool secondary)
{
  if (!feature) { return false; } // NULL pointer will not be added
  if (secondary && m_secondaryFeatureCount < maxSecondaryFeatureCount)
  {
    m_secondaryFeatures[m_secondaryFeatureCount] = feature;
    m_secondaryFeatureCount++;
    return true;
  }
  else if (m_featureCount < maxFeatureCount)
  {
    m_features[m_featureCount] = feature;
    m_featureCount++;
    return true;
  }
  return false;
}

/**
 * Compute each feature and send the result to their respective receivers
 * NB: If the feature is not calculated (eg: because not ready), nothing is sent
 * to the receivers.
 */
void IUFeatureConfigurator::computeAndSendToReceivers()
{
  bool computed = false;
  //Secondary Features first
  for (uint8_t i = 0; i < m_secondaryFeatureCount; i++)
  {
    computed = m_secondaryFeatures[i]->compute();
    if (computed)
    {
      m_secondaryFeatures[i]->sendToReceivers();
    }
  }
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    computed = m_features[i]->compute();
    if (computed)
    {
      m_features[i]->sendToReceivers();
    }
  }
}

/**
 * Reset feature source counters to start collection from scratch
 * Do this when starting the board (after prepareSource),
 * or when switching operation modes.
 */
void IUFeatureConfigurator::resetFeaturesCounters()
{
  //Secondary Features first
  for (uint8_t i = 0; i < m_secondaryFeatureCount; i++)
  {
    m_secondaryFeatures[i]->resetCounters();
  }
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    m_features[i]->resetCounters();
  }
}

/**
 * Returns the highest operationState from primary features
 */
operationState IUFeatureConfigurator::getOperationStateFromFeatures()
{
  operationState opState = operationState::idle;
  operationState featState;
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    featState = m_features[i]->getOperationState();
    if ((uint8_t) opState < (uint8_t) featState)
    {
      opState = featState;
    }
  }
  return featState;
}

/**
 * Stream feature latest values through given HardwareSerial
 * @param the port to use to stream feature data
 * @return the number of features that were actually streamed (those that were ready)
 */
uint8_t IUFeatureConfigurator::streamFeatures(HardwareSerial *port)
{
  uint8_t counter = 0;
  bool streamed = false;
  for (int i = 0; i < m_featureCount; i++)
  {
    streamed = m_features[i]->stream(port);
    if (streamed)
    {
      counter++;
      port->print(",");
    }
  }
  return counter;
}

/* ====================== Diagnostic Functions, only active when debugMode = true ====================== */

/**
 * Shows the name of features and their receiver configs
 */ 
void IUFeatureConfigurator::exposeFeaturesAndReceivers()
{
  if (!debugMode)
  {
    return; // Inactive if not in debugMode
  }
  if (m_secondaryFeatureCount == 0)
  {
    debugPrint("No secondary feature");
  }
  else
  {
    for (int i = 0; i < m_secondaryFeatureCount; i++)
    {
      debugPrint(m_secondaryFeatures[i]->getName() + ": ");
      m_secondaryFeatures[i]->exposeReceivers();
      debugPrint("\n");
    }
  }
  if (m_featureCount == 0)
  {
    debugPrint("No primary feature");
  }
  else
  {
    for (int i = 0; i < m_featureCount; i++)
    {
      debugPrint(m_features[i]->getName() + ": ");
      m_features[i]->exposeReceivers();
      debugPrint("\n");
    }
  }
}

/**
 * Shows the name of features and their receiver configs
 */ 
void IUFeatureConfigurator::exposeFeatureStates()
{
  if (!debugMode)
  {
    return; // Inactive if not in debugMode
  }
  if (m_secondaryFeatureCount > 0)
  {
    debugPrint("Secondary feature state:");
  }
  for (int i = 0; i < m_secondaryFeatureCount; i++)
  {
    debugPrint(m_secondaryFeatures[i]->getName() + " info: ");
    m_secondaryFeatures[i]->exposeSourceConfig();
    m_secondaryFeatures[i]->exposeCounterState();
  }
  if (m_featureCount > 0)
  {
    debugPrint("Primary feature state:");
  }
  for (int i = 0; i < m_featureCount; i++)
  {
    debugPrint(m_secondaryFeatures[i]->getName() + " info: ");
    m_features[i]->exposeSourceConfig();
    m_features[i]->exposeCounterState();
  }
  debugPrint("\n");
}

