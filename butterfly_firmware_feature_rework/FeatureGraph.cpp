#include "FeatureGraph.h"


/* =============================================================================
    Feature definitions
============================================================================= */

/***** Battery load *****/

float batteryLoadValues[2];
FloatFeature batteryLoad("BAT", 2, 1, batteryLoadValues);


/***** Accelerometer Features *****/

// Sensor data
__attribute__((section(".noinit2"))) q15_t accelerationXValues[1024];
__attribute__((section(".noinit2"))) q15_t accelerationYValues[1024];
__attribute__((section(".noinit2"))) q15_t accelerationZValues[1024];
Q15Feature accelerationX("A0X", 8, 128, accelerationXValues);
Q15Feature accelerationY("A0Y", 8, 128, accelerationYValues);
Q15Feature accelerationZ("A0Z", 8, 128, accelerationZValues);


// 128 sample long accel features
__attribute__((section(".noinit2"))) float accelRMS128XValues[8];
__attribute__((section(".noinit2"))) float accelRMS128YValues[8];
__attribute__((section(".noinit2"))) float accelRMS128ZValues[8];
__attribute__((section(".noinit2"))) float accelRMS128TotalValues[8];
FloatFeature accelRMS128X("A7X", 2, 4, accelRMS128XValues);
FloatFeature accelRMS128Y("A7Y", 2, 4, accelRMS128YValues);
FloatFeature accelRMS128Z("A7Z", 2, 4, accelRMS128ZValues);
FloatFeature accelRMS128Total("A73", 2, 4, accelRMS128TotalValues);


// 512 sample long accel features
__attribute__((section(".noinit2"))) float accelRMS512XValues[2];
__attribute__((section(".noinit2"))) float accelRMS512YValues[2];
__attribute__((section(".noinit2"))) float accelRMS512ZValues[2];
__attribute__((section(".noinit2"))) float accelRMS512TotalValues[2];
FloatFeature accelRMS512X("A9X", 2, 1, accelRMS512XValues);
FloatFeature accelRMS512Y("A9Y", 2, 1, accelRMS512YValues);
FloatFeature accelRMS512Z("A9Z", 2, 1, accelRMS512ZValues);
FloatFeature accelRMS512Total("A93", 2, 1, accelRMS512TotalValues);


// FFT feature from 512 sample long accel data
__attribute__((section(".noinit2"))) q15_t accelReducedFFTXValues[100];
__attribute__((section(".noinit2"))) q15_t accelReducedFFTYValues[100];
__attribute__((section(".noinit2"))) q15_t accelReducedFFTZValues[100];
Q15Feature accelReducedFFTX("FAX", 2, 50, accelReducedFFTXValues);
Q15Feature accelReducedFFTY("FAY", 2, 50, accelReducedFFTYValues);
Q15Feature accelReducedFFTZ("FAZ", 2, 50, accelReducedFFTZValues);

// Velocity features from 512 sample long accel data
__attribute__((section(".noinit2"))) float velAmplitude512XValues[2];
__attribute__((section(".noinit2"))) float velAmplitude512YValues[2];
__attribute__((section(".noinit2"))) float velAmplitude512ZValues[2];
FloatFeature velAmplitude512X("VVX", 2, 1, velAmplitude512XValues);
FloatFeature velAmplitude512Y("VVY", 2, 1, velAmplitude512YValues);
FloatFeature velAmplitude512Z("VVZ", 2, 1, velAmplitude512ZValues);

__attribute__((section(".noinit2"))) float velRMS512XValues[2];
__attribute__((section(".noinit2"))) float velRMS512YValues[2];
__attribute__((section(".noinit2"))) float velRMS512ZValues[2];
FloatFeature velRMS512X("VAX", 2, 1, velRMS512XValues);
FloatFeature velRMS512Y("VAY", 2, 1, velRMS512YValues);
FloatFeature velRMS512Z("VAZ", 2, 1, velRMS512ZValues);

// Displacements features from 512 sample long accel data
__attribute__((section(".noinit2"))) float dispAmplitude512XValues[2];
__attribute__((section(".noinit2"))) float dispAmplitude512YValues[2];
__attribute__((section(".noinit2"))) float dispAmplitude512ZValues[2];
FloatFeature dispAmplitude512X("DVX", 2, 1, dispAmplitude512XValues);
FloatFeature dispAmplitude512Y("DVY", 2, 1, dispAmplitude512YValues);
FloatFeature dispAmplitude512Z("DVZ", 2, 1, dispAmplitude512ZValues);

__attribute__((section(".noinit2"))) float dispRMS512XValues[2];
__attribute__((section(".noinit2"))) float dispRMS512YValues[2];
__attribute__((section(".noinit2"))) float dispRMS512ZValues[2];
FloatFeature dispRMS512X("DAX", 2, 1, dispRMS512XValues);
FloatFeature dispRMS512Y("DAX", 2, 1, dispRMS512YValues);
FloatFeature dispRMS512Z("DAZ", 2, 1, dispRMS512ZValues);


/***** Gyroscope Features *****/

// Sensor data
__attribute__((section(".noinit2"))) q15_t tiltXValues[2];
__attribute__((section(".noinit2"))) q15_t tiltYValues[2];
__attribute__((section(".noinit2"))) q15_t tiltZValues[2];
Q15Feature tiltX("T0X", 2, 1, tiltXValues);
Q15Feature tiltY("T0Y", 2, 1, tiltYValues);
Q15Feature tiltZ("T0Z", 2, 1, tiltZValues);


/***** Magnetometer Features *****/

// Sensor data
__attribute__((section(".noinit2"))) q15_t magneticXValues[2];
__attribute__((section(".noinit2"))) q15_t magneticYValues[2];
__attribute__((section(".noinit2"))) q15_t magneticZValues[2];
Q15Feature magneticX("M0X", 2, 1, magneticXValues);
Q15Feature magneticY("M0Y", 2, 1, magneticYValues);
Q15Feature magneticZ("M0Z", 2, 1, magneticZValues);


/***** Barometer Features *****/

// Sensor data
__attribute__((section(".noinit2"))) float temperatureValues[2];
__attribute__((section(".noinit2"))) float pressureValues[2];
FloatFeature temperature("TMP", 2, 1, temperatureValues);
FloatFeature pressure("PRS", 2, 1, pressureValues);


/***** Audio Features *****/

// Sensor data
q15_t audioValues[8192];
Q15Feature audio("SND", 2, 2048, audioValues);

// 2048 sample long features
__attribute__((section(".noinit2"))) float audioDB2048Values[4];
FloatFeature audioDB2048("S11", 4, 1, audioDB2048Values);

// 4096 sample long features
__attribute__((section(".noinit2"))) float audioDB4096Values[2];
FloatFeature audioDB4096("S12", 2, 1, audioDB4096Values);


/***** GNSS Feature *****/



/***** Pointers *****/

Feature *FEATURES[FEATURE_COUNT] = {
    &accelerationX,
    &accelerationY,
    &accelerationZ,
    &accelRMS128X,
    &accelRMS128Y,
    &accelRMS128Z,
    &accelRMS128Total,
    &accelRMS512X,
    &accelRMS512Y,
    &accelRMS512Z,
    &accelRMS512Total,
    &accelReducedFFTX,
    &accelReducedFFTY,
    &accelReducedFFTZ,
    &velAmplitude512X,
    &velAmplitude512Y,
    &velAmplitude512Z,
    &velRMS512X,
    &velRMS512Y,
    &velRMS512Z,
    &dispAmplitude512X,
    &dispAmplitude512Y,
    &dispAmplitude512Z,
    &dispRMS512X,
    &dispRMS512Y,
    &dispRMS512Z,
    &temperature,
    &pressure,
    &audio,
    &audioDB2048,
    &audioDB4096,
};


/***** Selectors*****/

/**
 * Return a pointer to the 1st existing Feature that has given name.
 *
 * NB: Will return a NULL pointer if no such Feature is found
 */
Feature* getFeatureByName(const char* name)
{
    for (uint8_t i = 0; i < FEATURE_COUNT; ++i)
    {
        if (FEATURES[i]->isNamed(name))
        {
            return FEATURES[i];
        }
    }
    return NULL;
}


/* =============================================================================
    Computers declarations
============================================================================= */

// Shared computation space
q15_t allocatedFFTSpace[1024];

// Note that computer_id 0 is reserved to designate an absence of computer.

/***** Accelerometer Features *****/

// 128 sample long accel computers
SignalRMSComputer accel128XComputer(1,&accelRMS128X);
SignalRMSComputer accel128YComputer(2, &accelRMS128Y);
SignalRMSComputer accel128ZComputer(3, &accelRMS128Z);
MultiSourceSumComputer accelRMS128TotalComputer(4, &accelRMS128Total);


// 512 sample long accel computers
SectionSumComputer accel512XComputer(5, 1, &accelRMS512X);
SectionSumComputer accel512YComputer(6, 1, &accelRMS512Y);
SectionSumComputer accel512ZComputer(7, 1, &accelRMS512Z);
SectionSumComputer accel512TotalComputer(8, 1, &accelRMS512Total);


// Computers for FFT feature from 512 sample long accel data
Q15FFTComputer accelReducedFFTXComputer(9,
                                        &accelReducedFFTX,
                                        &velAmplitude512X,
                                        &velRMS512X,
                                        &dispAmplitude512X,
                                        &dispRMS512X,
                                        allocatedFFTSpace);
Q15FFTComputer accelReducedFFTYComputer(10,
                                        &accelReducedFFTY,
                                        &velAmplitude512Y,
                                        &velRMS512Y,
                                        &dispAmplitude512Y,
                                        &dispRMS512Y,
                                        allocatedFFTSpace);
Q15FFTComputer accelReducedFFTZComputer(11,
                                        &accelReducedFFTZ,
                                        &velAmplitude512Z,
                                        &velRMS512Z,
                                        &dispAmplitude512Z,
                                        &dispRMS512Z,
                                        allocatedFFTSpace);


/***** Audio Features *****/

AudioDBComputer audioDB2048Computer(12, &audioDB2048);
AudioDBComputer audioDB4096Computer(13, &audioDB4096);


/***** Pointers *****/

// Some featureComputers alter there data source, so it is important that those
// be listed last of all the computers that use the same data source.

FeatureComputer *FEATURE_COMPUTERS[FEATURE_COMPUTER_COUNT] = {
    &accel128XComputer,
    &accel128YComputer,
    &accel128ZComputer,
    &accelRMS128TotalComputer,
    &accel512XComputer,
    &accel512YComputer,
    &accel512ZComputer,
    &accel512TotalComputer,
    &accelReducedFFTXComputer,
    &accelReducedFFTYComputer,
    &accelReducedFFTZComputer,
    &audioDB2048Computer,
    &audioDB4096Computer,
};


/***** Selectors*****/

/**
 * Return a pointer to the 1st existing FeatureComputer that has given id.
 *
 * NB: Will return a NULL pointer if no such FeatureComputer is found
 */
FeatureComputer* getFeatureComputerById(uint8_t id)
{
    for (uint8_t i = 0; i < FEATURE_COMPUTER_COUNT; ++i)
    {
        if (FEATURE_COMPUTERS[i]->getId() == id)
        {
            return FEATURE_COMPUTERS[i];
        }
    }
    return NULL;
}


/* =============================================================================
    Sensors declarations
============================================================================= */

IUBattery iuBattery(&iuI2C, "BAT", &batteryLoad);
IUBMP280 iuAltimeter(&iuI2C, "ALT", &temperature, &pressure);
IUBMX055Acc iuAccelerometer(&iuI2C, "ACC", &accelerationX, &accelerationY,
                            &accelerationZ);
IUBMX055Gyro iuGyroscope(&iuI2C, "GYR", &tiltX, &tiltY, &tiltZ);
IUBMX055Mag iuBMX055Mag(&iuI2C, "MAG", &magneticX, &magneticY, &magneticZ);
IUCAMM8Q iuGNSS(&iuI2C, "GPS");
IUI2S iuI2S(&iuI2C, "MIC", &audio);


/***** Pointers *****/

Sensor *SENSORS[SENSOR_COUNT] = {
    &iuBattery,
    &iuAltimeter,
    &iuAccelerometer,
    &iuGyroscope,
    &iuBMX055Mag,
    &iuGNSS,
    &iuI2S
};


/***** Selectors*****/

/**
 * Return a pointer to the 1st existing sensor that has given name.
 *
 * NB: Will return a NULL pointer if no such sensor is found
 */
Sensor* getSensorByName(const char* name)
{
    for (uint8_t i = 0; i < SENSOR_COUNT; ++i)
    {
        if (SENSORS[i]->isNamed(name))
        {
            return SENSORS[i];
        }
    }
    return NULL;
}


/* =============================================================================
    Feature streaming group declarations
============================================================================= */

FeatureStreamingGroup featureGroup1(1);
FeatureStreamingGroup featureGroup2(2);
FeatureStreamingGroup featureGroup3(3);


/* =============================================================================
    Utilities
============================================================================= */

/***** Computers setup *****/

/**
 * Associate the FeatureComputer instances to their sources.
 */
void setUpComputerSource()
{
    // From acceleration sensor data
    accel128XComputer.addSource(&accelerationX, 1);
    accel128YComputer.addSource(&accelerationY, 1);
    accel128ZComputer.addSource(&accelerationZ, 1);
    accelRMS128TotalComputer.addSource(&accelRMS128X, 1);
    accelRMS128TotalComputer.addSource(&accelRMS128Y, 1);
    accelRMS128TotalComputer.addSource(&accelRMS128Z, 1);

    // Aggregate acceleration RMS
    accel512XComputer.addSource(&accelRMS128X, 1);
    accel512YComputer.addSource(&accelRMS128Y, 1);
    accel512ZComputer.addSource(&accelRMS128Z, 1);
    accel512TotalComputer.addSource(&accelRMS128Total, 1);

    // Acceleration FFTs
    accelReducedFFTXComputer.addSource(&accelerationX, 4);
    accelReducedFFTYComputer.addSource(&accelerationY, 4);
    accelReducedFFTZComputer.addSource(&accelerationZ, 4);

    // Audio DB
    audioDB2048Computer.addSource(&audio, 1);
    audioDB4096Computer.addSource(&audio, 2);
}


/***** Activate / deactivate features *****/

/**
 * Activate the computation of a feature
 *
 * Also recursively activate the computation of all the feature's required
 * components (ie the sources of its computer).
 */
void activateFeature(Feature* feature)
{
    feature->activate();
    uint8_t compId = feature->getComputerId();
    if (compId == 0)
    {
        return;
    }
    FeatureComputer* computer = getFeatureComputerById(compId);
    if (computer != NULL)
    {
        // Activate the feature's computer
        computer->activate();
        // Activate the computer's sources
        for (uint8_t i = 0; i < computer->getSourceCount(); ++i)
        {
            activateFeature(computer->getSource(i));
        }
    }
    // TODO Activate sensors as well?
}

/**
 * Check whether the feature computation can be deactivated.
 *
 * A feature is deactivatable if:
 *  - it is not streaming
 *  - all it's receivers (computers who use it as source) are deactivated
 */
bool isFeatureDeactivatable(Feature* feature)
{
    if (feature->isStreaming())
    {
        return false;
    }
    FeatureComputer* computer;
    for (uint8_t i = 0; i < feature->getReceiverCount(); ++i)
    {
        computer = getFeatureComputerById(feature->getReceiverId(i));
        if (computer != NULL && !computer->isActive())
        {
            return false;
        }
    }
    return true;
}

/**
 * Deactivate the computation of a feature
 *
 * Will also disable the feature streaming.
 * Will also check if the computation of the feature's required components can
 * be deactivated.
 */
void deactivateFeature(Feature* feature)
{
    feature->deactivate();
    uint8_t compId = feature->getComputerId();
    if (compId != 0)
    {
        FeatureComputer* computer = getFeatureComputerById(compId);
        if (computer != NULL)
        {
            // If none of the computer's destinations are active, the computer
            // can be deactivated too.
            bool deactivatable = true;
            for (uint8_t i = 0; i < computer->getDestinationCount(); ++i)
            {
                deactivatable &= (!computer->getDestination(i)->isActive());
            }
            if (deactivatable)
            {
                computer->deactivate();
                //
                Feature* antecedent;
                for (uint8_t i = 0; i < computer->getSourceCount(); ++i)
                {
                    antecedent = computer->getSource(i);
                    if (!isFeatureDeactivatable(antecedent))
                    {
                        deactivateFeature(antecedent);
                    }
                }
            }
        }
    }
    // TODO Deactivate sensors as well?
}

/**
 * Deactivate all features and feature computers
 */
void deactivateEverything()
{
    featureGroup1.reset();
    featureGroup2.reset();
    featureGroup3.reset();
    for (uint8_t i = 0; i < FEATURE_COUNT; ++i)
    {
        FEATURES[i]->deactivate();
    }
    for (uint8_t i = 0; i < FEATURE_COMPUTER_COUNT; ++i)
    {
        FEATURE_COMPUTERS[i]->deactivate();
    }
    // TODO Deactivate sensors as well?
}


/***** Configuration *****/

/**
 * Applied the given config to the feature.
 */
bool configureFeature(Feature *feature, JsonVariant &config)
{
    JsonVariant my_config = config[feature->getName()];
    if (!my_config)
    {
        return false;
    }
    // Activate the feature
    activateFeature(feature);
    // Configure the streaming
    JsonVariant group = my_config["GRP"];
    JsonVariant index = my_config["IDX"];
    if (group.success() && index.success())
    {
        switch(group.as<int>())
        {
            case 1:
                featureGroup1.add((uint8_t) (index.as<int>()), feature);
                break;
            case 2:
                featureGroup2.add((uint8_t) (index.as<int>()), feature);
                break;
            case 3:
                featureGroup3.add((uint8_t) (index.as<int>()), feature);
                break;
        }
    }
    JsonVariant value = my_config["OPS"];
    if (value)
    {
        if (value.as<int>())
        {
            feature->enableOperationState();
        }
        else
        {
            feature->disableOperationState();
        }
    }
    // Thresholds setting
    value = my_config["TRH"][0];
    if (value.success())
    {
        feature->setThreshold(0, value);
    }
    value = my_config["TRH"][1];
    if (value.success())
    {
        feature->setThreshold(1, value);
    }
    value = my_config["TRH"][2];
    if (value.success())
    {
        feature->setThreshold(2, value);
    }
    if (debugMode)
    {
        debugPrint(F("Configured feature "), false);
        debugPrint(feature->getName());
    }
    return true;
}

/**
 * Check if at least one feature is streaming.
 */
bool atLeastOneStreamingFeature()
{
    for (uint8_t i = 0; i < FEATURE_COUNT; ++i)
    {
        if (FEATURES[i]->isStreaming())
        {
            return true;
        }
    }
    return false;
}


/***** Default feature sets *****/

void enableCalibrationFeatures()
{
    //calibrationFeatureGroup.setDataSendPeriod(defaultDataSendPeriod);
}

void enableMotorFeatures()
{
    deactivateEverything();
    Feature *feature = getFeatureByName("A93");
    activateFeature(feature);
    featureGroup1.add(0, feature);
    feature = getFeatureByName("VAX");
    activateFeature(feature);
    featureGroup1.add(1, feature);
    feature = getFeatureByName("VAY");
    activateFeature(feature);
    featureGroup1.add(2, feature);
    feature = getFeatureByName("VAZ");
    activateFeature(feature);
    featureGroup1.add(3, feature);
    feature = getFeatureByName("TMP");
    activateFeature(feature);
    featureGroup1.add(4, feature);
    feature = getFeatureByName("S11");
    activateFeature(feature);
    featureGroup1.add(5, feature);

}

void enablePressFeatures()
{
    deactivateEverything();
    Feature *feature = getFeatureByName("A93");
    activateFeature(feature);
    featureGroup1.add(0, feature);
    feature = getFeatureByName("A9X");
    activateFeature(feature);
    featureGroup1.add(1, feature);
    feature = getFeatureByName("A9Y");
    activateFeature(feature);
    featureGroup1.add(2, feature);
    feature = getFeatureByName("A9Z");
    activateFeature(feature);
    featureGroup1.add(3, feature);
    feature = getFeatureByName("TMP");
    activateFeature(feature);
    featureGroup1.add(4, feature);
    feature = getFeatureByName("S11");
    activateFeature(feature);
    featureGroup1.add(5, feature);
}

void exposeAllConfigurations()
{
    #ifdef DEBUGMODE
    for (uint8_t i = 0; i < SENSOR_COUNT; ++i)
    {
        SENSORS[i]->expose();
    }
    debugPrint("");
    for (uint8_t i = 0; i < FEATURE_COUNT; ++i)
    {
        FEATURES[i]->exposeConfig();
        FEATURES[i]->exposeCounters();
        debugPrint("_____");
    }
    debugPrint("");
    for (uint8_t i = 0; i < FEATURE_COMPUTER_COUNT; ++i)
    {
        FEATURE_COMPUTERS[i]->exposeConfig();
    }
    #endif
}
