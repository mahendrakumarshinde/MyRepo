#include "IUFeatureConfigurator.h"


/* ========================= Feature Configuration ================================ */
/*
Short name coding
char 1: feature type
- A for acceleration
- B for secondary acceleration, that means acceleration energy computed by summing other energy
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
  "",
  {0, "", ""},
  {0, "", ""}
};

IUFeatureConfigurator::FeatureConfig IUFeatureConfigurator::registeredConfigs[IUFeatureConfigurator::registeredCount] =
  {
    {"AX1", // "accelEnergy_X_128"
      {3, "A-A-A", "0-4-3"},
      {0, "", ""}},
      
    {"AY1", // "accelEnergy_Y_128",
      {3, "A-A-A", "1-4-3"},
      {0, "", ""}},
      
    {"AZ1", // "accelEnergy_Z_128",
      {3, "A-A-A", "2-4-3"},
      {0, "", ""}},
      
    {"CX1", // "IUAccelPreComputationFeature128" Precomputation of Accel signal energy and FFT
      {3, "A-A-A", "0-4-3"},
      {0, "", ""}},
      
    {"CY1", // "IUAccelPreComputationFeature128" Precomputation of Accel signal energy and FFT
      {3, "A-A-A", "1-4-3"},
      {0, "", ""}},
      
    {"CZ1", // "IUAccelPreComputationFeature128" Precomputation of Accel signal energy and FFT
      {3, "A-A-A", "2-4-3"},
      {0, "", ""}},
      
    {"A31", // "accelEnergy_3_128",
      {5, "A-A-A-A-A", "0-1-2-4-3"},
      {0, "", ""}},
      
    {"B31", // "accelEnergy_3_128" by summing 3 single axis accel energy,
      {0, "", ""},
      {3, "AX1-AY1-AZ1", "0-0-0"}},
      
    {"D31", // "accelEnergy_3_128" by summing 3 single axis accel energy,
      {0, "", ""},
      {3, "CX1-CY1-CZ1", "0-0-0"}},
      
    {"AX3", // "accelEnergy_X_512",
      {3, "A-A-A", "0-4-3"},
      {0, "", ""}},
      
    {"AY3", // "accelEnergy_Y_512",
      {3, "A-A-A", "1-4-3"},
      {0, "", ""}},
      
    {"AZ3", // "accelEnergy_Z_512",
      {3, "A-A-A", "2-4-3"},
      {0, "", ""}},
      
    {"CX3", // "IUAccelPreComputationFeature512" Precomputation of Accel signal energy and FFT
      {3, "A-A-A", "0-4-3"},
      {0, "", ""}},
      
    {"CY3", // "IUAccelPreComputationFeature512" Precomputation of Accel signal energy and FFT
      {3, "A-A-A", "1-4-3"},
      {0, "", ""}},
      
    {"CZ3", // "IUAccelPreComputationFeature512" Precomputation of Accel signal energy and FFT
      {3, "A-A-A", "2-4-3"},
      {0, "", ""}},
      
    {"A33", // "accelEnergy_3_512",
      {5, "A-A-A-A-A", "0-1-2-4-3"},
      {0, "", ""}},
      
    {"B33", // "accelEnergy_3_512" by summing 3 single axis accel energy,
      {0, "", ""},
      {3, "AX3-AY3-AZ3", "0-0-0"}},
      
    {"D33", // "accelEnergy_3_512" by summing 3 single axis accel energy,
      {0, "", ""},
      {3, "CX3-CY3-CZ3", "0-0-0"}},
      
    {"VX3", // "velocity_X_512",
      {0, "", ""},
      {5, "CX3-CX3-CX3-CX3-CX3", "99-2-3-4-6"}},   // 99 means VX3 will receive the destinationArray from CX3
      
    {"VY3", // "velocity_Y_512",
      {0, "", ""},
      {5, "CY3-CY3-CY3-CY3-CY3", "99-2-3-4-6"}},
      
    {"VZ3", // "velocity_Z_512",
      {0, "", ""},
      {5, "CZ3-CZ3-CZ3-CZ3-CZ3", "99-2-3-4-6"}},
    
    {"FX3", // Freq X 512
      {0, "", ""},
      {1, "CX3", "5"}},
      
    {"FY3", // Freq Y 512
      {0, "", ""},
      {1, "CY3", "5"}},
      
    {"FZ3", // Freq Z 512
      {0, "", ""},
      {1, "CZ3", "5"}},
    
    {"RX3", // RMS X 512
      {0, "", ""},
      {1, "CX3", "4"}},
      
    {"RY3", // RMS Y 512
      {0, "", ""},
      {1, "CY3", "4"}},
      
    {"RZ3", // RMS Z 512
      {0, "", ""},
      {1, "CZ3", "4"}},
      
    {"T10", // "temperature_1_1",
      {1, "T", "0"},
      {0, "", ""}},
      
    {"S15", // "audioDB_1_2048",
      {1, "S", "0"},
      {0, "", ""}},
      
    {"S16", // "audioDB_1_4096",
      {1, "S", "0"},
      {0, "", ""}},
  };
  
String IUFeatureConfigurator::calibrationConfig = "CX3-CY3-CZ3-D33-VX3-VY3-VZ3-T10-S16-FX3-FY3-FZ3-RX3-RY3-RZ3";
uint8_t IUFeatureConfigurator::calibrationFeatureIds[15] = {7, 8, 9, 1, 2, 3, 4, 5, 6, 10, 11, 12, 13, 14, 15};
bool IUFeatureConfigurator::calibrationFeatureStream[15] = {false, false, false, false,
                                                           true, true, true,  // VX3, VY3, VZ3
                                                           true, false,
                                                           true, true, true,
                                                           true, true, true};

String IUFeatureConfigurator::standardConfig = "CX3-CY3-CZ3-D33-VX3-VY3-VZ3-T10-S16-FX3-FY3-FZ3-RX3-RY3-RZ3"; //
float IUFeatureConfigurator::standardThresholds[15][3] = {{17, 40, 70},
                                                         {17, 40, 70},
                                                         {17, 40, 70},
                                                         {17, 40, 70},
                                                         {0.05, 1.2, 1.8},
                                                         {0.05, 1.2, 1.8},
                                                         {0.05, 1.2, 1.8},
                                                         {200, 205, 210},
                                                         {500, 1000, 1500},
                                                         {0, 0, 0},
                                                         {0, 0, 0},
                                                         {0, 0, 0},
                                                         {0, 0, 0},
                                                         {0, 0, 0},
                                                         {0, 0, 0}};
uint8_t IUFeatureConfigurator::standardFeatureIds[15] = {7, 8, 9, 1, 2, 3, 4, 5, 6, 10, 11, 12, 13, 14, 15};
bool IUFeatureConfigurator::standardFeatureStream[15] = {false, false, false, true, true, true, true, true, true, false, false, false, false, false, false};
bool IUFeatureConfigurator::standardFeatureCheck[15] = {false, false, false, true, false, false, false, false, false, false, false, false, false, false, false};

String IUFeatureConfigurator::pressConfig = "A33-A31-T10-S16";
float IUFeatureConfigurator::pressThresholds[4][3] = {{30, 600, 1200},
                                                      {30, 600, 1200},
                                                      {200, 205, 210},
                                                      {500, 1000, 1500}};
uint8_t IUFeatureConfigurator::pressFeatureIds[4] = {1, 2, 3, 4};
bool IUFeatureConfigurator::pressFeatureStream[4] = {true, false, false, false};
bool IUFeatureConfigurator::pressFeatureCheck[4] = {true, false, false, false};

/* ========================= Method definitions  ================================ */

IUFeatureConfigurator::IUFeatureConfigurator() :
  m_featureCount(0)
{
  for (int i = 0; i < maxFeatureCount; i++)
  {
    m_features[i] = NULL;
  }
}

IUFeatureConfigurator::~IUFeatureConfigurator()
{
  resetFeatures();
}

/**
 * Delete all features
 */
void IUFeatureConfigurator::resetFeatures()
{
  for (int i = 0; i < maxFeatureCount; i++)
  {
    //Serial.print(i); Serial.print(": "); 
    if(m_features[i])
    {
      //Serial.print("if ");
      //Serial.println(m_features[i]->getName());
      delete m_features[i];
      //Serial.print("deleted");
      m_features[i] = NULL;
      //Serial.println("now null");
    }
  }
  m_featureCount = 0;
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
    if (setupDebugMode) { debugPrint("Tried to look for an empty config name, returned NoConfig"); }
    return noConfig;
  }
  for (uint8_t i = 0; i < registeredCount; i++)
  {
    if (registeredConfigs[i].name[0] == featureName[0] &&
        registeredConfigs[i].name[1] == featureName[1] &&
        registeredConfigs[i].name[2] == featureName[2])
    {
      return registeredConfigs[i];
    }
  }
  if (setupDebugMode) { debugPrint(featureName + " is not a registered config name"); }
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
  FeatureConfig fconfig = getConfigFromName(feature->getName());
  if (fconfig.name == "")
  {
    return false;
  }
  FeatureConfig depConf[registeredCount];
  uint8_t depCount = getConfigFromName(fconfig.featureDep.names, depConf);
  if (depCount != fconfig.featureDep.dependencyCount)
  {
    if (setupDebugMode) { debugPrint("Invalid configuration: dependency count does not match dependency names"); }
    return false;
  }
  int depSendingOptions[registeredCount];
  uint8_t depSendOptCount = splitStringToInt(fconfig.featureDep.sendingOptions, nameSeparator, depSendingOptions, registeredCount);
  if (depSendOptCount != fconfig.featureDep.dependencyCount)
  {
    if (setupDebugMode) { debugPrint("Invalid configuration: dependency count does not match dependency options"); }
    return false;
  }
  uint8_t reservedSourceForSensor = fconfig.sensorDep.dependencyCount;
  for (uint8_t i = 0; i < fconfig.featureDep.dependencyCount; i++)
  {
    IUFeature *producerFeature = getFeatureByName(depConf[i].name);
    if (producerFeature == NULL)
    {
      if (setupDebugMode)
      {
        debugPrint(F("Missing feature dependency: "), false);
        debugPrint(depConf[i].name);
      }
      return false;
    }
    if (!producerFeature->getProducer()->addReceiver(depSendingOptions[i], reservedSourceForSensor + i, feature))
    {
      if (setupDebugMode) { debugPrint(F("Failed to add feature as dependent of another feature")); }
      return false;
    }
  }
  return true;
}

/**
 * Register given feature as receiver of given component
 * @param feature           the feature to register as receiver
 * @param sensor            pointer to a sensors
 */
bool IUFeatureConfigurator::registerFeatureInSensors(IUFeature *feature, IUABCSensor **sensors, uint8_t sensorCount)
{
  FeatureConfig fconfig = getConfigFromName(feature->getName());
  if (fconfig.name == "")
  {
    if (setupDebugMode) { debugPrint(F("Configuration not found")); }
    return false;
  }
  String foundSensorTypes[registeredCount];
  uint8_t foundSensorCount = splitString(fconfig.sensorDep.names, nameSeparator, foundSensorTypes, registeredCount);
  if (foundSensorCount != fconfig.sensorDep.dependencyCount)
  {
    if (setupDebugMode) { debugPrint(F("Invalid configuration: dependency count does not match dependency names")); }
    return false;
  }
  int depSendingOptions[registeredCount];
  uint8_t depSendOptCount = splitStringToInt(fconfig.sensorDep.sendingOptions, nameSeparator, depSendingOptions, registeredCount);
  if (depSendOptCount != fconfig.sensorDep.dependencyCount)
  {
    if (setupDebugMode) { debugPrint(F("Invalid configuration: dependency count does not match dependency options")); }
    return false;
  }
  bool success = true;
  for (uint8_t i = 0; i < fconfig.sensorDep.dependencyCount; i++)
  {
    for (uint8_t j = 0; j < sensorCount; j++)
    {
      if (foundSensorTypes[i][0] == (char) sensors[j]->getSensorType())
      {
        success &= sensors[j]->addScalarReceiver(depSendingOptions[i], i, feature);
        feature->getProducer()->setSamplingRate(sensors[j]->getSamplingRate());
      }
    }
  }
  if (!success && setupDebugMode) { debugPrint(F("Failed to register feature as receiver of given sensors")); }
  return success;
}

/**
 * Register given feature as receiver of given component
 * @param feature           the feature to register as receiver
 * @param sensor            pointer to a sensor
 */
bool IUFeatureConfigurator::registerAllFeaturesInSensors(IUABCSensor **sensors, uint8_t sensorCount)
{
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    if (!registerFeatureInSensors(m_features[i], sensors, sensorCount))
    {
      return false;
    }
  }
  return true;
}

/**
 * Create a feature following a IUFeatureConfigurator::FeatureConfig while handling dependencies (create them if needed)
 *
 * Function adds all the created feature to this instance, and call registerFeatureInFeatureDependencies
 * to handle feature inter-dependencies.
 * @param config      the config to create the feature
 * @param id          the id that will be given to the new feature - default to 0
 */
int IUFeatureConfigurator::createWithDependencies(IUFeatureConfigurator::FeatureConfig config, uint8_t id)
{
  if (setupDebugMode)
  {
    debugPrint("Starting creation of ", false);
    memoryLog(config.name);
  }
  bool success = true;
  /*
  // Get dependency list
  FeatureConfig defDep[registeredCount];
  int defDepCount = getConfigFromName(config.featureDep.names, defDep);
  for (uint8_t i = 0; i < defDepCount; i++)
  {
    if (setupDebugMode) { debugPrint(config.name + " requires dependency: " + defDep[i].name); }
    // Check if the dependency exists and, if not, create it
    if(getFeatureByName(defDep[i].name) == NULL)
    {
      success = createWithDependencies(defDep[i], 0);
      if (!success)
      {
        if (setupDebugMode) { debugPrint("Failed to create " + config.name + " dependencies"); }
        return false;
      }
      else if (setupDebugMode)
      {
        debugPrint("Created " + defDep[i].name + " feature as dependency.");
      }
    }
    else if (setupDebugMode)
    {
      debugPrint(defDep[i].name + " feature already exists, it will be used as dependency.");
    }
  }
  */
  IUFeature *feature = createFeature(config, id);
  success = addFeature(feature);
  if (!success)
  {
    if (feature) { delete feature; }
    if (setupDebugMode)
    {
      debugPrint(config.name, false);
      debugPrint(" incorrectly created");
    }
    return false;
  }
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
    success = createWithDependencies(requiredConfigs[i], i);
    if (!success) { return false; }
    if (setupDebugMode)
    {
      debugPrint(F("Feature "), false);
      debugPrint(requiredConfigs[i].name, false);
      memoryLog(" created");
    }
  }
  resetFeaturesCounters();
  // Check that the number of features is the same as required
  return requiredConfigCount == m_featureCount;
}

/**
 * Do a calibration setup from pre-programmed configuration in FeatureConfigurator
 */
bool IUFeatureConfigurator::doCalibrationSetup()
{
  if (!requireConfiguration(calibrationConfig))
  {
    return false;
  }
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    m_features[i]->setId(calibrationFeatureIds[i]);
    m_features[i]->setStreaming(calibrationFeatureStream[i]);
  }
  return true;
}

void IUFeatureConfigurator::setCalibrationStreaming()
{
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    m_features[i]->setStreaming(calibrationFeatureStream[i]);
  }
}

void IUFeatureConfigurator::setStandardStreaming()
{
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    m_features[i]->setStreaming(standardFeatureStream[i]);
  }
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
    m_features[i]->setId(standardFeatureIds[i]);
    m_features[i]->setThresholds(standardThresholds[i][0], standardThresholds[i][1], standardThresholds[i][2]);
    m_features[i]->setFeatureCheck(standardFeatureCheck[i]);
    m_features[i]->setStreaming(standardFeatureStream[i]);
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
    m_features[i]->setId(pressFeatureIds[i]);
    m_features[i]->setThresholds(pressThresholds[i][0], pressThresholds[i][1], pressThresholds[i][2]);
    m_features[i]->setFeatureCheck(pressFeatureCheck[i]);
    m_features[i]->setStreaming(pressFeatureStream[i]);
  }
  return true;
}



/**
 * Create a feature from given config and return a pointer to it
 * @param config        the config to create the feature
 * @param id            the id that will be given to the new feature (optional, default to 0)
 */
IUFeature* IUFeatureConfigurator::createFeature(IUFeatureConfigurator::FeatureConfig featConfig, uint8_t id)
{
  IUFeature *feature = NULL;
  switch ((char) featConfig.name[0])
  {
    case 'A':
      if (featConfig.name[1] == '3')
      {
        if (featConfig.name[2] == '1') { feature = new IUTriAxisEnergyFeature128(id, featConfig.name); }
        else if (featConfig.name[2] == '3') { feature = new IUTriAxisEnergyFeature512(id, featConfig.name); }
      }
      else
      {
        if (featConfig.name[2] == '1') { feature = new IUSingleAxisEnergyFeature128(id, featConfig.name); }
        else if (featConfig.name[2] == '3') { feature = new IUSingleAxisEnergyFeature512(id, featConfig.name); }
      }
      break;

    case 'B':
    case 'D':
      feature = new IUTriSourceSummingFeature(id, featConfig.name);
      break;
      
    case 'C':
      if (featConfig.name[2] == '1') { feature = new IUAccelPreComputationFeature128(id, featConfig.name); }
      else if (featConfig.name[2] == '3') { feature = new IUAccelPreComputationFeature512(id, featConfig.name); }
      break;

    case 'F':
    case 'R':
    case 'T':
      feature = new IUDefaultFloatFeature(id, featConfig.name);
      break;

    case 'S':
      if (featConfig.name[2] == '5') { feature = new IUAudioDBFeature2048(id, featConfig.name); }
      else if (featConfig.name[2] == '6') { feature = new IUAudioDBFeature4096(id, featConfig.name); }
      break;

    case 'V':
      if (featConfig.name[2] == '3') { feature = new IUVelocityFeature512(id, featConfig.name); }
      break;

    default:
      if (setupDebugMode)
      {
        debugPrint("Feature name ", false);
        debugPrint(featConfig.name, false);
        debugPrint(" is not listed in createFeature function");
      }
      break;
  }
  if (feature == NULL)
  {
    if (setupDebugMode) { debugPrint(F("Feature memory allocation failed.")); }
    return NULL;
  }
  feature->prepareSource();
  feature->prepareProducer();
  if (!feature->activate())
  {
    if (setupDebugMode) { debugPrint(F("Feature couldn't be activated: check the source and producer")); }
    delete feature; feature = NULL;
  }
  return feature;
}

/**
 * Get the feature with given id (lookup on instantiated features)
 */
IUFeature* IUFeatureConfigurator::getFeatureById(uint8_t id)
{
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    if (m_features[i]->getId() == id)
    {
      return m_features[i];
    }
  }
  return NULL;
}


/**
 * Get the feature with given name (lookup on instantiated features)
 */
IUFeature* IUFeatureConfigurator::getFeatureByName(char *name)
{
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    String fName = m_features[i]->getName();
    if (name[0] == fName[0] &&
        name[1] == fName[1] &&
        name[2] == fName[2])
    {
      return m_features[i];
    }
  }
  return NULL;
}

/**
 * Add a feature to the configurator
 * @param feature     the feature to add.
 */
bool IUFeatureConfigurator::addFeature(IUFeature *feature)
{
  if (feature != NULL && m_featureCount < maxFeatureCount)
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
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    computed = m_features[i]->compute();
    if (computed)
    {
      m_features[i]->getProducer()->sendToReceivers();
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
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    m_features[i]->resetCounters();
  }
}

/**
 * Returns the highest operationState from features
 */
operationState::option IUFeatureConfigurator::getOperationStateFromFeatures()
{
  operationState::option opState = operationState::IDLE;
  operationState::option featState;
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    featState = m_features[i]->updateState();
    if ((uint8_t) opState < (uint8_t) featState)
    {
      opState = featState;
    }
  }
  return opState;
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
    if (m_features[i]->isStreamed())
    {
      port->print(",");
      m_features[i]->stream(port);
      counter++;
    }
  }
  return counter;
}

/**
 * Reset the pointers of all the sensor receivers to NULL
 */
void IUFeatureConfigurator::resetAllReceivers()
{
  for (uint8_t i = 0; i < m_featureCount; i++)
  {
    m_features[i]->resetReceivers();
  }
}

/* ====================== Diagnostic Functions, only active when setupDebugMode = true ====================== */

/**
 * Shows the name of features and their receiver configs
 */
void IUFeatureConfigurator::exposeFeaturesAndReceivers()
{
  #ifdef DEBUGMODE
  if (m_featureCount == 0)
  {
    debugPrint("No feature");
  }
  else
  {
    debugPrint(m_featureCount, false);
    debugPrint(" features:");
    for (int i = 0; i < m_featureCount; i++)
    {
      debugPrint(m_features[i]->getName() + ": ");
      m_features[i]->getProducer()->exposeReceivers();
      debugPrint(' ');
    }
  }
  #endif
}

/**
 * Shows the name of features and their receiver configs
 */
void IUFeatureConfigurator::exposeFeatureStates()
{
  #ifdef DEBUGMODE
  if (m_featureCount > 0)
  {
    debugPrint("Feature config and state:");
  }
  for (int i = 0; i < m_featureCount; i++)
  {
    debugPrint(' ');
    debugPrint(m_features[i]->getName() + " info: ");
    m_features[i]->exposeSourceConfig();
    m_features[i]->exposeCounterState();
  }
  debugPrint(' ');
  #endif
}

