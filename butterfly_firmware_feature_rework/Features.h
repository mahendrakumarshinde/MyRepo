#ifndef FEATURES_H
#define FEATURES_H

#include "FeatureBuffer.h"
#include "FeatureComputer.h"


/* =============================================================================
    Feature declarations
============================================================================= */

/***** Accelerometer Features *****/

// Sensor data
extern q15_t accelerationXValues[1024];
extern q15_t accelerationYValues[1024];
extern q15_t accelerationZValues[1024];
extern Q15Feature accelerationX;
extern Q15Feature accelerationY;
extern Q15Feature accelerationZ;


// 128 sample long accel features
extern float accelEnergy128XValues[8];
extern float accelEnergy128YValues[8];
extern float accelEnergy128ZValues[8];
extern FloatFeature accelEnergy128X;
extern FloatFeature accelEnergy128Y;
extern FloatFeature accelEnergy128Z;

extern float accelPower128XValues[8];
extern float accelPower128YValues[8];
extern float accelPower128ZValues[8];
extern FloatFeature accelPower128X;
extern FloatFeature accelPower128Y;
extern FloatFeature accelPower128Z;

extern float accelRMS128XValues[8];
extern float accelRMS128YValues[8];
extern float accelRMS128ZValues[8];
extern FloatFeature accelRMS128X;
extern FloatFeature accelRMS128Y;
extern FloatFeature accelRMS128Z;

extern float accelEnergy128TotalValues[8];
extern float accelPower128TotalValues[8];
extern float accelRMS128TotalValues[8];
extern FloatFeature accelEnergy128Total;
extern FloatFeature accelPower128Total;
extern FloatFeature accelRMS128Total;


// 512 sample long accel features
extern float accelEnergy512XValues[2];
extern float accelEnergy512YValues[2];
extern float accelEnergy512ZValues[2];
extern FloatFeature accelEnergy512X;
extern FloatFeature accelEnergy512Y;
extern FloatFeature accelEnergy512Z;

extern float accelPower512XValues[2];
extern float accelPower512YValues[2];
extern float accelPower512ZValues[2];
extern FloatFeature accelPower512X;
extern FloatFeature accelPower512Y;
extern FloatFeature accelPower512Z;

extern float accelRMS512XValues[2];
extern float accelRMS512YValues[2];
extern float accelRMS512ZValues[2];
extern FloatFeature accelRMS512X;
extern FloatFeature accelRMS512Y;
extern FloatFeature accelRMS512Z;

extern float accelEnergy512TotalValues[2];
extern float accelPower512TotalValues[2];
extern float accelRMS512TotalValues[2];
extern FloatFeature accelEnergy512Total;
extern FloatFeature accelPower512Total;
extern FloatFeature accelRMS512Total;


// FFT feature from 512 sample long accel data
extern q15_t accelReducedFFTXValues[100];
extern q15_t accelReducedFFTYValues[100];
extern q15_t accelReducedFFTZValues[100];
extern Q15Feature accelReducedFFTX;
extern Q15Feature accelReducedFFTY;
extern Q15Feature accelReducedFFTZ;

extern float velocityAmplitude512XValues[2];
extern float velocityAmplitude512YValues[2];
extern float velocityAmplitude512ZValues[2];
extern FloatFeature velocityAmplitude512X;
extern FloatFeature velocityAmplitude512Y;
extern FloatFeature velocityAmplitude512Z;

extern float velocityRMS512XValues[2];
extern float velocityRMS512YValues[2];
extern float velocityRMS512ZValues[2];
extern FloatFeature velocityRMS512X;
extern FloatFeature velocityRMS512Y;
extern FloatFeature velocityRMS512Z;

extern float displacementAmplitude512XValues[2];
extern float displacementAmplitude512YValues[2];
extern float displacementAmplitude512ZValues[2];
extern FloatFeature displacementAmplitude512X;
extern FloatFeature displacementAmplitude512Y;
extern FloatFeature displacementAmplitude512Z;

extern float displacementRMS512XValues[2];
extern float displacementRMS512YValues[2];
extern float displacementRMS512ZValues[2];
extern FloatFeature displacementRMS512X;
extern FloatFeature displacementRMS512Y;
extern FloatFeature displacementRMS512Z;


/***** Gyroscope Features *****/

// Sensor data
//extern Q15Feature tiltX;
//extern Q15Feature tiltY;
//extern Q15Feature tiltZ;


/***** Magnetometer Features *****/

// Sensor data
//extern Q15Feature magneticX;
//extern Q15Feature magneticY;
//extern Q15Feature magneticZ;


/***** Barometer Features *****/

// Sensor data
extern float temperatureValues[2];
extern float pressureValues[2];

extern FloatFeature temperature;
extern FloatFeature pressure;


/***** Audio Features *****/

// Sensor data
extern q15_t audioValues[8192];
extern Q15Feature audio;

// 2048 sample long features
extern float audioDB2048Values[4];
extern FloatFeature audioDB2048;

// 4096 sample long features
extern float audioDB4096Values[2];
extern FloatFeature audioDB4096;


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


/* =============================================================================
    Utilities
============================================================================= */

const uint8_t FEATURE_COUNT = 47;
extern FeatureBuffer *FEATURES[FEATURE_COUNT];

const uint8_t FEATURE_COMPUTER_COUNT = 15;
extern FeatureComputer *FEATURE_COMPUTERS[FEATURE_COMPUTER_COUNT];

/***** Computers setup *****/
void setUpComputerSource();

/***** Feature and computer selectors*****/
FeatureBuffer* getFeatureByName(const char* name);
FeatureComputer* getFeatureComputerById(uint8_t id);

/***** Activate / deactivate features *****/
void activateFeature(FeatureBuffer* feature);
bool isFeatureDeactivatable(FeatureBuffer* feature);
void deactivateFeature(FeatureBuffer* feature);
void enableStreaming(FeatureBuffer* feature);
void disableStreaming(FeatureBuffer* feature);


/***** Default feature sets *****/
void enableMotorFeatures();
void enablePressFeatures();
void exposeAllFeatureConfigurations();


#endif // FEATURES_H
