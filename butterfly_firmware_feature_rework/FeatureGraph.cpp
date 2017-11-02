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
__attribute__((section(".noinit2"))) q15_t accelReducedFFTXValues[300];
__attribute__((section(".noinit2"))) q15_t accelReducedFFTYValues[300];
__attribute__((section(".noinit2"))) q15_t accelReducedFFTZValues[300];
Q15Feature accelReducedFFTX("FAX", 2, 150, accelReducedFFTXValues);
Q15Feature accelReducedFFTY("FAY", 2, 150, accelReducedFFTYValues);
Q15Feature accelReducedFFTZ("FAZ", 2, 150, accelReducedFFTZValues);

// Velocity features from 512 sample long accel data
__attribute__((section(".noinit2"))) float velRMS512XValues[2];
__attribute__((section(".noinit2"))) float velRMS512YValues[2];
__attribute__((section(".noinit2"))) float velRMS512ZValues[2];
FloatFeature velRMS512X("VAX", 2, 1, velRMS512XValues);
FloatFeature velRMS512Y("VAY", 2, 1, velRMS512YValues);
FloatFeature velRMS512Z("VAZ", 2, 1, velRMS512ZValues);

// Displacements features from 512 sample long accel data
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
    &velRMS512X,
    &velRMS512Y,
    &velRMS512Z,
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
SignalRMSComputer accel128ComputerX(1,&accelRMS128X);
SignalRMSComputer accel128ComputerY(2, &accelRMS128Y);
SignalRMSComputer accel128ComputerZ(3, &accelRMS128Z);
MultiSourceSumComputer accelRMS128TotalComputer(4, &accelRMS128Total);


// 512 sample long accel computers
SectionSumComputer accel512ComputerX(5, 1, &accelRMS512X);
SectionSumComputer accel512ComputerY(6, 1, &accelRMS512Y);
SectionSumComputer accel512ComputerZ(7, 1, &accelRMS512Z);
SectionSumComputer accel512TotalComputer(8, 1, &accelRMS512Total);


// Computers for FFT feature from 512 sample long accel data
Q15FFTComputer accelFFTComputerX(9,
                                 &accelReducedFFTX,
                                 &velRMS512X,
                                 &dispRMS512X,
                                 allocatedFFTSpace);
Q15FFTComputer accelFFTComputerY(10,
                                 &accelReducedFFTY,
                                 &velRMS512Y,
                                 &dispRMS512Y,
                                 allocatedFFTSpace);
Q15FFTComputer accelFFTComputerZ(11,
                                 &accelReducedFFTZ,
                                 &velRMS512Z,
                                 &dispRMS512Z,
                                 allocatedFFTSpace);


/***** Audio Features *****/

AudioDBComputer audioDB2048Computer(12, &audioDB2048);
AudioDBComputer audioDB4096Computer(13, &audioDB4096);


/***** Pointers *****/

// Some featureComputers alter their data source, so it is important that those
// be listed last of all the computers that use the same data source.

FeatureComputer *FEATURE_COMPUTERS[FEATURE_COMPUTER_COUNT] = {
    &accel128ComputerX,
    &accel128ComputerY,
    &accel128ComputerZ,
    &accelRMS128TotalComputer,
    &accel512ComputerX,
    &accel512ComputerY,
    &accel512ComputerZ,
    &accel512TotalComputer,
    &accelFFTComputerX,
    &accelFFTComputerY,
    &accelFFTComputerZ,
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


/***** Set up source *****/

/**
 * Associate the FeatureComputer instances to their sources.
 */
void setUpComputerSource()
{
    // From acceleration sensor data
    accel128ComputerX.addSource(&accelerationX, 1);
    accel128ComputerY.addSource(&accelerationY, 1);
    accel128ComputerZ.addSource(&accelerationZ, 1);
    accelRMS128TotalComputer.addSource(&accelRMS128X, 1);
    accelRMS128TotalComputer.addSource(&accelRMS128Y, 1);
    accelRMS128TotalComputer.addSource(&accelRMS128Z, 1);

    // Aggregate acceleration RMS
    accel512ComputerX.addSource(&accelRMS128X, 1);
    accel512ComputerY.addSource(&accelRMS128Y, 1);
    accel512ComputerZ.addSource(&accelRMS128Z, 1);
    accel512TotalComputer.addSource(&accelRMS128Total, 1);

    // Acceleration FFTs
    accelFFTComputerX.addSource(&accelerationX, 4);
    accelFFTComputerY.addSource(&accelerationY, 4);
    accelFFTComputerZ.addSource(&accelerationZ, 4);

    // Audio DB
    audioDB2048Computer.addSource(&audio, 1);
    audioDB4096Computer.addSource(&audio, 2);
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
    Feature Profiles declarations
============================================================================= */

// Health Check
FeatureProfile healthCheckProfile("HEALTH", 500);
// Calibration
FeatureProfile calibrationProfile("CAL001", 512);
// Standard Press Monitoring
FeatureProfile pressStandardProfile("PRSSTD", 512);
// Standard Motor Monitoring
FeatureProfile motorStandardProfile("MOTSTD", 512);


/***** Pointers *****/

FeatureProfile *FEATURE_PROFILES[FEATURE_PROFILE_COUNT] = {
    &healthCheckProfile,
    &calibrationProfile,
    &pressStandardProfile,
    &motorStandardProfile
};


/***** Selector *****/

/**
 * Return a pointer to the 1st existing featureProfile that has given name.
 *
 * NB: Will return a NULL pointer if no such featureProfile is found
 */
FeatureProfile* getProfileByName(const char* name)
{
    for (uint8_t i = 0; i < FEATURE_PROFILE_COUNT; ++i)
    {
        if (FEATURE_PROFILES[i]->isNamed(name))
        {
            return FEATURE_PROFILES[i];
        }
    }
    return NULL;
}


/***** Profile Configuration *****/

void deactivateAllProfiles()
{
    for (uint8_t i = 0; i < FEATURE_PROFILE_COUNT; ++i)
    {
        FEATURE_PROFILES[i]->deactivate();
    }
    deactivateAllFeatures();
}

void setUpProfiles()
{
    // Health Check
    healthCheckProfile.addFeature(&batteryLoad);
    // TODO => configure full HealthCheckProfile
    // Calibration (previously VX3, VY3, VZ3, T10, FX3, FY3, FZ3, RX3, RY3, RZ3
    // TODO Fix CalibrationProfile features
    calibrationProfile.addFeature(&velRMS512X);
    calibrationProfile.addFeature(&velRMS512Y);
    calibrationProfile.addFeature(&velRMS512Z);
    pressStandardProfile.addFeature(&temperature);
    calibrationProfile.addFeature(&dispRMS512X);
    calibrationProfile.addFeature(&dispRMS512Y);
    calibrationProfile.addFeature(&dispRMS512Z);
    calibrationProfile.addFeature(&accelRMS512X);
    calibrationProfile.addFeature(&accelRMS512Y);
    calibrationProfile.addFeature(&accelRMS512Z);
    //calibrationProfile
    // Standard Press Monitoring
    pressStandardProfile.addFeature(&accelRMS512Total);
    pressStandardProfile.addFeature(&accelRMS512X);
    pressStandardProfile.addFeature(&accelRMS512Y);
    pressStandardProfile.addFeature(&accelRMS512Z);
    pressStandardProfile.addFeature(&temperature);
    pressStandardProfile.addFeature(&audioDB4096);
    // Standard Motor Monitoring
    motorStandardProfile.addFeature(&accelRMS512Total);
    motorStandardProfile.addFeature(&velRMS512X);
    motorStandardProfile.addFeature(&velRMS512Y);
    motorStandardProfile.addFeature(&velRMS512Z);
    motorStandardProfile.addFeature(&temperature);
    motorStandardProfile.addFeature(&audioDB4096);
}


/* =============================================================================
    Utilities
============================================================================= */

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
void deactivateAllFeatures()
{
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

