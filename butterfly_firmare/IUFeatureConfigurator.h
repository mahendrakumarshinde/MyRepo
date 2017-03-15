#ifndef IUFEATURECONFIGURATOR_H
#define IUFEATURECONFIGURATOR_H

#include <Arduino.h>
#include "IUUtilities.h"
#include "IUFeature.h"
#include "IUABCSensor.h"
#include <string.h>

/*
//TODO delete this commented section after confirming struct declaration works OK

    static String names[registeredCount] = {
      "AX1",  // 0
      "AY1",  // 1
      "AZ1",  // 2
      "A31",  // 3
      "AX3",  // 4
      "AY3",  // 5
      "AZ3",  // 6
      "A33",  // 7
      "VX3",  // 8
      "VY3",  // 9
      "VZ3",  // 10
      "T10",  // 11
      "S15"}; // 12
    static String fullNames[registeredCount] = {
      "accelEnergy_X_128", // 0
      "accelEnergy_Y_128", // 1
      "accelEnergy_Z_128", // 2
      "accelEnergy_3_128", // 3
      "accelEnergy_X_512", // 4
      "accelEnergy_Y_512", // 5
      "accelEnergy_Z_512", // 6
      "accelEnergy_3_512", // 7
      "velocity_X_512",    // 8
      "velocity_Y_512",    // 9
      "velocity_Z_512",    // 10
      "temperature_1_1",   // 11
      "audioDB_1_2048"};   // 12
    static String hardDependencies[registeredCount] = {
      "", "", "", "", "", "", "", "",
      "AX3", // 8 => Velocity X needs AccelEnergy X
      "AY3", // 9 => Velocity Y needs AccelEnergy Y
      "AZ3", // 10 => Velocity Z needs AccelEnergy Z
      "", ""};
    static String softDependencies[registeredCount] = {
      "", "", "",
      "AX1-AY1-AZ1", // 3 => Total Accel Energy can be computed from all 3 axis Accel Energy
      "", "", "",
      "AX3-AY3-AZ3", // 7 => Total Accel Energy can be computed from all 3 axis Accel Energy
      "", "", "", "", ""};
    static IUABCSensor::allSensorTypes sensorDependencies[registeredCount] = {
      IUABCSensor::accelerometer, // 0
      IUABCSensor::accelerometer, // 1
      IUABCSensor::accelerometer, // 2
      IUABCSensor::accelerometer, // 3
      IUABCSensor::accelerometer, // 4
      IUABCSensor::accelerometer, // 5
      IUABCSensor::accelerometer, // 6
      IUABCSensor::accelerometer, // 7
      IUABCSensor::accelerometer, // 8
      IUABCSensor::accelerometer, // 9
      IUABCSensor::accelerometer, // 10
      IUABCSensor::thermometer,   // 11
      IUABCSensor::microphone};   // 12

    static String hardSendingOptions[registeredCount] = {
      "", "", "", "", "", "", "", "",
      "1", // 8  => option 1 = feature operation state
      "1", // 9  => option 1 = feature operation state
      "1", // 10 => option 1 = feature operation state
      "", ""};
    static String softSendingOptions[registeredCount] = = {
      "", "", "",
      "0-0-0", // 3 => Option 0 = feature latest value
      "", "", "",
      "0-0-0", // 7 => Option 0 = feature latest value
      "", "", "", "", ""};
    static String sensorSendingOptions[registeredCount] = {
      "0",      // 0
      "1",      // 1
      "2",      // 2
      "0-1-2",  // 3
      "0",      // 4
      "1",      // 5
      "2",      // 6
      "0-1-2",  // 7
      "0",      // 8
      "1",      // 9
      "2",      // 10
      "0",      // 11
      "0"};     // 12
    static String featureClassNames[registeredCount] = {
      "IUSingleAxisEnergyFeature128", // 0
      "IUSingleAxisEnergyFeature128", // 1
      "IUSingleAxisEnergyFeature128", // 2
      "IUTriAxisEnergyFeature128", // 3
      "IUSingleAxisEnergyFeature512", // 4
      "IUSingleAxisEnergyFeature512", // 5
      "IUSingleAxisEnergyFeature512", // 6
      "IUTriAxisEnergyFeature512", // 7
      "IUVelocityFeature512",    // 8
      "IUVelocityFeature512",    // 9
      "IUVelocityFeature512",    // 10
      "IUDefaultFloatFeature",    // 11
      "IUAudioDBFeature2048"};  // 12
    }
    static String featureSoftDependenciesClassNames[registeredCount] = {
      "", "", "",
      "IUTriSourceSummingFeature", // 3
      "", "", "",
      "IUTriSourceSummingFeature", // 7
      "", "", "", "", ""};

*/

/**
 * An object dedicated to hold feature configuration and features themselves
 *
 * Static members of this class list possible feature names and their dependencies.
 * A configurator instance makes the link between user required configuration and
 * feature object instantiations, while handling dependencies.
 * User required features are considered main features (stored in m_features),
 * while dependencies features are considered secondary features (stored in
 * m_secondaryFeatures).
 *
 */
class IUFeatureConfigurator
{
  public:
    // Arrays have to be initialized with a fixed size,
    // so we chose max_size = max number of features per collections
    static const uint8_t maxFeatureCount = 10;
    static const uint8_t maxSecondaryFeatureCount = 10;
    static const uint8_t registeredCount = 14;
    static const char nameSeparator = '-';

    struct Dependencies {
      uint8_t dependencyCount;
      String names;
      String sendingOptions;
      /*
      Sending options represents what the dependency (ie producer) is expected
      to send to the feature (ie receiver).
      For sending option list, see DependecyFeatureClass.dataSendingOptions.
      */
    };

    struct FeatureSubConfig {
      String className;
      Dependencies sensorDep;  // Dependencies on sensors
      Dependencies featureDep; // Dependencies on other features
    };

    struct FeatureConfig {
      String name;
      String fullName;
      FeatureSubConfig def;    // default subconfiguration
      FeatureSubConfig alt;    // alternative subconfiguration
    };

    static FeatureConfig noConfig; // Handle no config found cases.
    static FeatureConfig registeredConfigs[registeredCount];

    static String standardConfig;
    static float standardThresholds[6][3];
    static bool standardFeatureCheck[6];
    static String pressConfig;
    static float pressThresholds[4][3];
    static bool pressFeatureCheck[4];

    // Constructor, destructor, getters and setters
    IUFeatureConfigurator();
    virtual ~IUFeatureConfigurator();
    void resetFeatures();
    FeatureConfig getConfigFromName(String featureName);
    uint8_t getConfigFromName(String featureNames, FeatureConfig *configs);
    bool registerFeatureInFeatureDependencies(IUFeature *feature, bool alternative = false);
    bool registerFeatureInSensor(IUFeature *feature, IUABCSensor *sensor, bool alternative = false);
    int createWithDependencies(FeatureConfig config, uint8_t id, bool secondary = false);
    bool requireConfiguration(String configBufffer);
    bool doStandardSetup();
    bool doPressSetup();
    IUFeature* createFeature(FeatureConfig config, uint8_t id = 0, bool alternative = false);
    IUFeature* createFeature(uint8_t configIdx, uint8_t id = 0, bool alternative = false);
    IUFeature* createFeature(String name, uint8_t id = 0, bool alternative = false);
    IUFeature* getFeature(uint8_t index) {return m_features[index]; }
    IUFeature* getSecondaryFeature(uint8_t index) {return m_secondaryFeatures[index]; }
    IUFeature* getFeatureByName(String name);
    bool addFeature(IUFeature *feature, bool secondary = false);
    void computeAndSendToReceivers();
    void resetFeaturesCounters();
    operationState getOperationStateFromFeatures();
    uint8_t streamFeatures(HardwareSerial *port);


  protected:
    uint8_t m_featureCount; // Dynamic
    uint8_t m_secondaryFeatureCount; // Dynamic
    IUFeature *m_features[maxFeatureCount];
    IUFeature *m_secondaryFeatures[maxFeatureCount];
};

#endif // IUFEATURECONFIGURATOR_H
