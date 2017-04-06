#ifndef IUFEATURECONFIGURATOR_H
#define IUFEATURECONFIGURATOR_H

#include <Arduino.h>
#include "IUFeature.h"
#include "IUABCSensor.h"

/**
 * An object dedicated to hold feature configuration and features themselves
 *
 * Static members of this class list possible feature names and their dependencies.
 * A configurator instance makes the link between user required configuration and
 * feature object instantiations, while handling dependencies.
 *
 */
class IUFeatureConfigurator
{
  public:
    // Arrays have to be initialized with a fixed size,
    // so we chose max_size = max number of features per collections
    static const uint8_t maxFeatureCount = 10;
    static const uint8_t registeredCount = 24;
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

    struct FeatureConfig {
      char name[4];
      Dependencies sensorDep;  // Dependencies on sensors
      Dependencies featureDep; // Dependencies on other features
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
    uint8_t getFeatureCount() { return m_featureCount; }
    void resetFeatures();
    FeatureConfig getConfigFromName(String featureName);
    uint8_t getConfigFromName(String featureNames, FeatureConfig *configs);
    bool registerFeatureInFeatureDependencies(IUFeature *feature);
    bool registerFeatureInSensors(IUFeature *feature, IUABCSensor **sensors, uint8_t sensorCount);
    bool registerAllFeaturesInSensors(IUABCSensor **sensors, uint8_t sensorCount);
    int createWithDependencies(FeatureConfig config, uint8_t id);
    bool requireConfiguration(String configBufffer);
    bool doStandardSetup();
    bool doPressSetup();
    IUFeature* createFeature(FeatureConfig config, uint8_t id = 0);
    IUFeature* getFeature(uint8_t index) {return m_features[index]; }
    IUFeature* getFeatureByName(char *name);
    bool addFeature(IUFeature *feature);
    void computeAndSendToReceivers();
    void resetFeaturesCounters();
    operationState getOperationStateFromFeatures();
    uint8_t streamFeatures(HardwareSerial *port);
    // Diagnostic Functions
    void exposeFeaturesAndReceivers();
    void exposeFeatureStates();


  protected:
    uint8_t m_featureCount; // Dynamic
    IUFeature *m_features[maxFeatureCount];
};

#endif // IUFEATURECONFIGURATOR_H
