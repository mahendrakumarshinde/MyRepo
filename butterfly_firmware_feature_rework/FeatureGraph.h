#ifndef FEATUREGRAPH_H
#define FEATUREGRAPH_H

#include <ArduinoJson.h>

#include "FeatureClass.h"
#include "FeatureComputer.h"
#include "FeatureStreamingGroup.h"
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
extern __attribute__((section(".noinit2"))) float accelEnergy128XValues[8];
extern __attribute__((section(".noinit2"))) float accelEnergy128YValues[8];
extern __attribute__((section(".noinit2"))) float accelEnergy128ZValues[8];
extern FloatFeature accelEnergy128X;
extern FloatFeature accelEnergy128Y;
extern FloatFeature accelEnergy128Z;

extern __attribute__((section(".noinit2"))) float accelPower128XValues[8];
extern __attribute__((section(".noinit2"))) float accelPower128YValues[8];
extern __attribute__((section(".noinit2"))) float accelPower128ZValues[8];
extern FloatFeature accelPower128X;
extern FloatFeature accelPower128Y;
extern FloatFeature accelPower128Z;

extern __attribute__((section(".noinit2"))) float accelRMS128XValues[8];
extern __attribute__((section(".noinit2"))) float accelRMS128YValues[8];
extern __attribute__((section(".noinit2"))) float accelRMS128ZValues[8];
extern FloatFeature accelRMS128X;
extern FloatFeature accelRMS128Y;
extern FloatFeature accelRMS128Z;

extern __attribute__((section(".noinit2"))) float accelEnergy128TotalValues[8];
extern __attribute__((section(".noinit2"))) float accelPower128TotalValues[8];
extern __attribute__((section(".noinit2"))) float accelRMS128TotalValues[8];
extern FloatFeature accelEnergy128Total;
extern FloatFeature accelPower128Total;
extern FloatFeature accelRMS128Total;


// 512 sample long accel features
extern __attribute__((section(".noinit2"))) float accelEnergy512XValues[2];
extern __attribute__((section(".noinit2"))) float accelEnergy512YValues[2];
extern __attribute__((section(".noinit2"))) float accelEnergy512ZValues[2];
extern FloatFeature accelEnergy512X;
extern FloatFeature accelEnergy512Y;
extern FloatFeature accelEnergy512Z;

extern __attribute__((section(".noinit2"))) float accelPower512XValues[2];
extern __attribute__((section(".noinit2"))) float accelPower512YValues[2];
extern __attribute__((section(".noinit2"))) float accelPower512ZValues[2];
extern FloatFeature accelPower512X;
extern FloatFeature accelPower512Y;
extern FloatFeature accelPower512Z;

extern __attribute__((section(".noinit2"))) float accelRMS512XValues[2];
extern __attribute__((section(".noinit2"))) float accelRMS512YValues[2];
extern __attribute__((section(".noinit2"))) float accelRMS512ZValues[2];
extern FloatFeature accelRMS512X;
extern FloatFeature accelRMS512Y;
extern FloatFeature accelRMS512Z;

extern __attribute__((section(".noinit2"))) float accelEnergy512TotalValues[2];
extern __attribute__((section(".noinit2"))) float accelPower512TotalValues[2];
extern __attribute__((section(".noinit2"))) float accelRMS512TotalValues[2];
extern FloatFeature accelEnergy512Total;
extern FloatFeature accelPower512Total;
extern FloatFeature accelRMS512Total;


// FFT feature from 512 sample long accel data
extern __attribute__((section(".noinit2"))) q15_t accelReducedFFTXValues[100];
extern __attribute__((section(".noinit2"))) q15_t accelReducedFFTYValues[100];
extern __attribute__((section(".noinit2"))) q15_t accelReducedFFTZValues[100];
extern Q15Feature accelReducedFFTX;
extern Q15Feature accelReducedFFTY;
extern Q15Feature accelReducedFFTZ;

extern __attribute__((section(".noinit2"))) float velocityAmplitude512XValues[2];
extern __attribute__((section(".noinit2"))) float velocityAmplitude512YValues[2];
extern __attribute__((section(".noinit2"))) float velocityAmplitude512ZValues[2];
extern FloatFeature velocityAmplitude512X;
extern FloatFeature velocityAmplitude512Y;
extern FloatFeature velocityAmplitude512Z;

extern __attribute__((section(".noinit2"))) float velocityRMS512XValues[2];
extern __attribute__((section(".noinit2"))) float velocityRMS512YValues[2];
extern __attribute__((section(".noinit2"))) float velocityRMS512ZValues[2];
extern FloatFeature velocityRMS512X;
extern FloatFeature velocityRMS512Y;
extern FloatFeature velocityRMS512Z;

extern __attribute__((section(".noinit2"))) float displacementAmplitude512XValues[2];
extern __attribute__((section(".noinit2"))) float displacementAmplitude512YValues[2];
extern __attribute__((section(".noinit2"))) float displacementAmplitude512ZValues[2];
extern FloatFeature displacementAmplitude512X;
extern FloatFeature displacementAmplitude512Y;
extern FloatFeature displacementAmplitude512Z;

extern __attribute__((section(".noinit2"))) float displacementRMS512XValues[2];
extern __attribute__((section(".noinit2"))) float displacementRMS512YValues[2];
extern __attribute__((section(".noinit2"))) float displacementRMS512ZValues[2];
extern FloatFeature displacementRMS512X;
extern FloatFeature displacementRMS512Y;
extern FloatFeature displacementRMS512Z;


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

const uint8_t FEATURE_COUNT = 47;
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
extern SignalEnergyComputer accel128XComputer;
extern SignalEnergyComputer accel128YComputer;
extern SignalEnergyComputer accel128ZComputer;

extern MultiSourceSumComputer accelEnergy128TotalComputer;
extern MultiSourceSumComputer accelPower128TotalComputer;
extern MultiSourceSumComputer accelRMS128TotalComputer;

// 512 sample long accel computers
extern SectionSumComputer accel512XComputer;
extern SectionSumComputer accel512YComputer;
extern SectionSumComputer accel512ZComputer;

extern SectionSumComputer accel512TotalComputer;


// computers for FFT feature from 512 sample long accel data
extern Q15FFTComputer accelReducedFFTXComputer;
extern Q15FFTComputer accelReducedFFTYComputer;
extern Q15FFTComputer accelReducedFFTZComputer;


/***** Audio Features *****/

extern AudioDBComputer audioDB2048Computer;
extern AudioDBComputer audioDB4096Computer;


/***** Pointers *****/

const uint8_t FEATURE_COMPUTER_COUNT = 15;
extern FeatureComputer *FEATURE_COMPUTERS[FEATURE_COMPUTER_COUNT];


/***** Selector *****/

FeatureComputer* getFeatureComputerById(uint8_t id);


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
    Feature streaming group declarations
============================================================================= */

extern FeatureStreamingGroup featureGroup1;
extern FeatureStreamingGroup featureGroup2;
extern FeatureStreamingGroup featureGroup3;


/* =============================================================================
    Utilities
============================================================================= */

/***** Computers setup *****/
void setUpComputerSource();


/***** Activate / deactivate features *****/
void activateFeature(Feature* feature);
bool isFeatureDeactivatable(Feature* feature);
void deactivateFeature(Feature* feature);
void deactivateEverything();


/***** Configuration *****/
bool configureFeature(Feature *feature, JsonVariant &config);
bool atLeastOneStreamingFeature();


/***** Default feature sets *****/
void enableCalibrationFeatures();
void enableMotorFeatures();
void enablePressFeatures();
void exposeAllConfigurations();

#endif // FEATUREGRAPH_H
