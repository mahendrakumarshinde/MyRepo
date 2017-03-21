#ifndef IUFEATURECONFIGURATOR_H
#define IUFEATURECONFIGURATOR_H

#include <Arduino.h>
#include "IUUtilities.h"
#include "IUFeature.h"
#include "IUABCSensor.h"
#include <string.h>

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
    bool registerFeatureInFeatureDependencies(IUFeature *feature);
    bool registerFeatureInSensor(IUFeature *feature, IUABCSensor **sensors, uint8_t sensorCount);
    void registerAllFeaturesInSensor(IUABCSensor **sensors, uint8_t sensorCount);
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
    // Diagnostic Functions
    void exposeFeaturesAndReceivers();
    void exposeFeatureStates();


  protected:
    uint8_t m_featureCount; // Dynamic
    uint8_t m_secondaryFeatureCount; // Dynamic
    IUFeature *m_features[maxFeatureCount];
    IUFeature *m_secondaryFeatures[maxFeatureCount];
};

#endif // IUFEATURECONFIGURATOR_H
