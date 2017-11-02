#ifndef FEATUREGRAPH_H
#define FEATUREGRAPH_H

#include <ArduinoJson.h>

#include "FeatureClass.h"
#include "FeatureComputer.h"
#include "FeatureProfile.h"
#include "IUI2C.h"
#include "IUBattery.h"
#include "IUBMP280.h"
#include "IUBMX055Acc.h"
#include "IUBMX055Gyro.h"
#include "IUBMX055Mag.h"
#include "IUCAMM8Q.h"
#include "IUI2S.h"


/* =============================================================================
    Feature declarations
============================================================================= */

/***** Battery load *****/

extern float batteryLoadValues[2];
extern FloatFeature batteryLoad;


/***** Accelerometer Features *****/

// Sensor data
extern __attribute__((section(".noinit2"))) q15_t accelerationXValues[1024];
extern __attribute__((section(".noinit2"))) q15_t accelerationYValues[1024];
extern __attribute__((section(".noinit2"))) q15_t accelerationZValues[1024];
extern Q15Feature accelerationX;
extern Q15Feature accelerationY;
extern Q15Feature accelerationZ;


// 128 sample long accel features
extern __attribute__((section(".noinit2"))) float accelRMS128XValues[8];
extern __attribute__((section(".noinit2"))) float accelRMS128YValues[8];
extern __attribute__((section(".noinit2"))) float accelRMS128ZValues[8];
extern __attribute__((section(".noinit2"))) float accelRMS128TotalValues[8];
extern FloatFeature accelRMS128X;
extern FloatFeature accelRMS128Y;
extern FloatFeature accelRMS128Z;
extern FloatFeature accelRMS128Total;

// 512 sample long accel features
extern __attribute__((section(".noinit2"))) float accelRMS512XValues[2];
extern __attribute__((section(".noinit2"))) float accelRMS512YValues[2];
extern __attribute__((section(".noinit2"))) float accelRMS512ZValues[2];
extern __attribute__((section(".noinit2"))) float accelRMS512TotalValues[2];
extern FloatFeature accelRMS512X;
extern FloatFeature accelRMS512Y;
extern FloatFeature accelRMS512Z;
extern FloatFeature accelRMS512Total;

// FFT feature from 512 sample long accel data
extern __attribute__((section(".noinit2"))) q15_t accelReducedFFTXValues[300];
extern __attribute__((section(".noinit2"))) q15_t accelReducedFFTYValues[300];
extern __attribute__((section(".noinit2"))) q15_t accelReducedFFTZValues[300];
extern Q15Feature accelReducedFFTX;
extern Q15Feature accelReducedFFTY;
extern Q15Feature accelReducedFFTZ;

// Velocity features from 512 sample long accel data
extern __attribute__((section(".noinit2"))) float velRMS512XValues[2];
extern __attribute__((section(".noinit2"))) float velRMS512YValues[2];
extern __attribute__((section(".noinit2"))) float velRMS512ZValues[2];
extern FloatFeature velRMS512X;
extern FloatFeature velRMS512Y;
extern FloatFeature velRMS512Z;

// Displacements features from 512 sample long accel data
extern __attribute__((section(".noinit2"))) float dispRMS512XValues[2];
extern __attribute__((section(".noinit2"))) float dispRMS512YValues[2];
extern __attribute__((section(".noinit2"))) float dispRMS512ZValues[2];
extern FloatFeature dispRMS512X;
extern FloatFeature dispRMS512Y;
extern FloatFeature dispRMS512Z;


/***** Gyroscope Features *****/

// Sensor data
extern __attribute__((section(".noinit2"))) q15_t tiltXValues[2];
extern __attribute__((section(".noinit2"))) q15_t tiltYValues[2];
extern __attribute__((section(".noinit2"))) q15_t tiltZValues[2];
extern Q15Feature tiltX;
extern Q15Feature tiltY;
extern Q15Feature tiltZ;


/***** Magnetometer Features *****/

// Sensor data
extern __attribute__((section(".noinit2"))) q15_t magneticXValues[2];
extern __attribute__((section(".noinit2"))) q15_t magneticYValues[2];
extern __attribute__((section(".noinit2"))) q15_t magneticZValues[2];
extern Q15Feature magneticX;
extern Q15Feature magneticY;
extern Q15Feature magneticZ;


/***** Barometer Features *****/

// Sensor data
extern __attribute__((section(".noinit2"))) float temperatureValues[2];
extern FloatFeature temperature;

extern __attribute__((section(".noinit2"))) float pressureValues[2];
extern FloatFeature pressure;


/***** Audio Features *****/

// Sensor data
extern q15_t audioValues[8192];
extern Q15Feature audio;

// 2048 sample long features
extern __attribute__((section(".noinit2"))) float audioDB2048Values[4];
extern FloatFeature audioDB2048;

// 4096 sample long features
extern __attribute__((section(".noinit2"))) float audioDB4096Values[2];
extern FloatFeature audioDB4096;


/***** GNSS Feature *****/


/***** Pointers *****/

const uint8_t FEATURE_COUNT = 25;
extern Feature *FEATURES[FEATURE_COUNT];


/***** Selector *****/

Feature* getFeatureByName(const char* name);


/* =============================================================================
    Computers declarations
============================================================================= */

// Shared computation space
extern q15_t allocatedFFTSpace[1024];


/***** Accelerometer Features *****/

// 128 sample long accel computers
extern SignalRMSComputer accel128ComputerX;
extern SignalRMSComputer accel128ComputerY;
extern SignalRMSComputer accel128ComputerZ;
extern MultiSourceSumComputer accelRMS128TotalComputer;

// 512 sample long accel computers
extern SectionSumComputer accel512ComputerX;
extern SectionSumComputer accel512ComputerY;
extern SectionSumComputer accel512ComputerZ;
extern SectionSumComputer accel512TotalComputer;


// computers for FFT feature from 512 sample long accel data
extern Q15FFTComputer accelFFTComputerX;
extern Q15FFTComputer accelFFTComputerY;
extern Q15FFTComputer accelFFTComputerZ;


/***** Audio Features *****/

extern AudioDBComputer audioDB2048Computer;
extern AudioDBComputer audioDB4096Computer;


/***** Pointers *****/

const uint8_t FEATURE_COMPUTER_COUNT = 13;
extern FeatureComputer *FEATURE_COMPUTERS[FEATURE_COMPUTER_COUNT];


/***** Selector *****/

FeatureComputer* getFeatureComputerById(uint8_t id);


/***** Set up sources *****/

void setUpComputerSource();


/* =============================================================================
    Sensors declarations
============================================================================= */

extern IUBattery iuBattery;
extern IUBMP280 iuAltimeter;
extern IUBMX055Acc iuAccelerometer;
extern IUBMX055Gyro iuGyroscope;
extern IUBMX055Mag iuBMX055Mag;
extern IUCAMM8Q iuGNSS;
extern IUI2S iuI2S;


/***** Pointers *****/

const uint8_t SENSOR_COUNT = 7;
extern Sensor *SENSORS[SENSOR_COUNT];


/***** Selector *****/

Sensor* getSensorByName(const char* name);


/* =============================================================================
    Feature Profiles declarations
============================================================================= */

extern FeatureProfile healthCheckProfile;
extern FeatureProfile calibrationProfile;
extern FeatureProfile pressStandardProfile;
extern FeatureProfile motorStandardProfile;


/***** Pointers *****/

const uint8_t FEATURE_PROFILE_COUNT = 4;
extern FeatureProfile *FEATURE_PROFILES[FEATURE_PROFILE_COUNT];


/***** Selector *****/

FeatureProfile* getProfileByName(const char* name);


/***** Profile Configuration *****/

void deactivateAllProfiles();
void setUpProfiles();


/* =============================================================================
    Utilities
============================================================================= */


/***** Activate / deactivate features *****/
void activateFeature(Feature* feature);
bool isFeatureDeactivatable(Feature* feature);
void deactivateFeature(Feature* feature);
void deactivateAllFeatures();

#endif // FEATUREGRAPH_H
