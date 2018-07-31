#ifndef INSTANCESBUTTERFLY_H
#define INSTANCESBUTTERFLY_H

#include "BoardDefinition.h"

#if defined(BUTTERFLY_V03) || defined(BUTTERFLY_V04)

/***** Interfaces *****/
#include "RGBLed.h"
#include "IUUSB.h"
#include "IUBMD350.h"
#ifdef USE_EXTERNAL_WIFI
#else
    #include "IUESP8285.h"
#endif

/***** Features *****/
#include "FeatureClass.h"
#include "FeatureComputer.h"
#include "FeatureGroup.h"
#include "CharBufferSendingQueue.h"

/***** Sensors *****/
#include "IUBattery.h"
#include "IUBMP280.h"
#include "IUBMX055Acc.h"
#include "IUBMX055Gyro.h"
#include "IUBMX055Mag.h"
#include "IUCAMM8Q.h"
#include "IUICS43432.h"

/***** Managers and helpers *****/
#include "LedManager.h"

#ifdef COMPONENTTEST
    // Interfaces
    #include "ComponentTest/CMP_IUBMD350.h"
    #include "ComponentTest/CMP_IUESP8285.h"
    // Sensors
    #include "ComponentTest/CMP_IUBattery.h"
    #include "ComponentTest/CMP_IUBMP280.h"
    #include "ComponentTest/CMP_IUBMX055.h"
    #include "ComponentTest/CMP_IUCAMM8Q.h"
    #include "ComponentTest/CMP_IUICS43432.h"
#endif


/* =============================================================================
    Interfaces
============================================================================= */

extern GPIORGBLed rgbLed;

extern LedManager ledManager;

extern char iuUSBBuffer[20];
extern IUUSB iuUSB;

extern char iuBluetoothBuffer[500];
extern IUBMD350 iuBluetooth;

extern char iuWiFiBuffer[500];
#ifdef USE_EXTERNAL_WIFI
    extern IUSerial iuWiFi;
#else
    extern IUESP8285 iuWiFi;
#endif


/* =============================================================================
    Features
============================================================================= */

/***** Operation State Monitoring feature *****/

extern q15_t opStateFeatureValues[2];
extern FeatureTemplate<q15_t> opStateFeature;

/***** Battery load *****/

extern float batteryLoadValues[2];
extern FeatureTemplate<float> batteryLoad;


/***** Accelerometer Features *****/

// Sensor data
extern __attribute__((section(".noinit2"))) q15_t accelerationXValues[1024];
extern __attribute__((section(".noinit2"))) q15_t accelerationYValues[1024];
extern __attribute__((section(".noinit2"))) q15_t accelerationZValues[1024];
extern FeatureTemplate<q15_t> accelerationX;
extern FeatureTemplate<q15_t> accelerationY;
extern FeatureTemplate<q15_t> accelerationZ;


// 128 sample long accel features
extern __attribute__((section(".noinit2"))) float accelRMS128XValues[8];
extern __attribute__((section(".noinit2"))) float accelRMS128YValues[8];
extern __attribute__((section(".noinit2"))) float accelRMS128ZValues[8];
extern __attribute__((section(".noinit2"))) float accelRMS128TotalValues[8];
extern FeatureTemplate<float> accelRMS128X;
extern FeatureTemplate<float> accelRMS128Y;
extern FeatureTemplate<float> accelRMS128Z;
extern FeatureTemplate<float> accelRMS128Total;

// 512 sample long accel features
extern __attribute__((section(".noinit2"))) float accelRMS512XValues[2];
extern __attribute__((section(".noinit2"))) float accelRMS512YValues[2];
extern __attribute__((section(".noinit2"))) float accelRMS512ZValues[2];
extern __attribute__((section(".noinit2"))) float accelRMS512TotalValues[2];
extern FeatureTemplate<float> accelRMS512X;
extern FeatureTemplate<float> accelRMS512Y;
extern FeatureTemplate<float> accelRMS512Z;
extern FeatureTemplate<float> accelRMS512Total;

// FFT feature from 512 sample long accel data
extern __attribute__((section(".noinit2"))) q15_t accelReducedFFTXValues[300];
extern __attribute__((section(".noinit2"))) q15_t accelReducedFFTYValues[300];
extern __attribute__((section(".noinit2"))) q15_t accelReducedFFTZValues[300];
extern FeatureTemplate<q15_t> accelReducedFFTX;
extern FeatureTemplate<q15_t> accelReducedFFTY;
extern FeatureTemplate<q15_t> accelReducedFFTZ;

// Acceleration main Frequency features from 512 sample long accel data
extern __attribute__((section(".noinit2"))) float accelMainFreqXValues[2];
extern __attribute__((section(".noinit2"))) float accelMainFreqYValues[2];
extern __attribute__((section(".noinit2"))) float accelMainFreqZValues[2];
extern FeatureTemplate<float> accelMainFreqX;
extern FeatureTemplate<float> accelMainFreqY;
extern FeatureTemplate<float> accelMainFreqZ;

// Velocity features from 512 sample long accel data
extern __attribute__((section(".noinit2"))) float velRMS512XValues[2];
extern __attribute__((section(".noinit2"))) float velRMS512YValues[2];
extern __attribute__((section(".noinit2"))) float velRMS512ZValues[2];
extern FeatureTemplate<float> velRMS512X;
extern FeatureTemplate<float> velRMS512Y;
extern FeatureTemplate<float> velRMS512Z;

// Displacements features from 512 sample long accel data
extern __attribute__((section(".noinit2"))) float dispRMS512XValues[2];
extern __attribute__((section(".noinit2"))) float dispRMS512YValues[2];
extern __attribute__((section(".noinit2"))) float dispRMS512ZValues[2];
extern FeatureTemplate<float> dispRMS512X;
extern FeatureTemplate<float> dispRMS512Y;
extern FeatureTemplate<float> dispRMS512Z;


/***** Gyroscope Features *****/

// Sensor data
extern __attribute__((section(".noinit2"))) q15_t tiltXValues[2];
extern __attribute__((section(".noinit2"))) q15_t tiltYValues[2];
extern __attribute__((section(".noinit2"))) q15_t tiltZValues[2];
extern FeatureTemplate<q15_t> tiltX;
extern FeatureTemplate<q15_t> tiltY;
extern FeatureTemplate<q15_t> tiltZ;


/***** Magnetometer Features *****/

// Sensor data
extern __attribute__((section(".noinit2"))) q15_t magneticXValues[2];
extern __attribute__((section(".noinit2"))) q15_t magneticYValues[2];
extern __attribute__((section(".noinit2"))) q15_t magneticZValues[2];
extern FeatureTemplate<q15_t> magneticX;
extern FeatureTemplate<q15_t> magneticY;
extern FeatureTemplate<q15_t> magneticZ;


/***** Barometer Features *****/

// Sensor data
extern __attribute__((section(".noinit2"))) float temperatureValues[2];
extern FeatureTemplate<float> temperature;

extern __attribute__((section(".noinit2"))) float pressureValues[2];
extern FeatureTemplate<float> pressure;


/***** Audio Features *****/

// Sensor data
extern q15_t audioValues[8192];
extern FeatureTemplate<q15_t> audio;

// 2048 sample long features
extern __attribute__((section(".noinit2"))) float audioDB2048Values[4];
extern FeatureTemplate<float> audioDB2048;

// 4096 sample long features
extern __attribute__((section(".noinit2"))) float audioDB4096Values[2];
extern FeatureTemplate<float> audioDB4096;


/***** GNSS Feature *****/


/***** RTD Temperature features *****/

#ifdef RTD_DAUGHTER_BOARD // Optionnal hardware
extern __attribute__((section(".noinit2"))) float rtdTempValues[8];
extern FeatureTemplate<float> rtdTemp;
#endif // RTD_DAUGHTER_BOARD


/* =============================================================================
    Sensors
============================================================================= */

extern IUBattery iuBattery;


void temperatureReadCallback(uint8_t wireStatus);
void pressureReadCallback(uint8_t wireStatus);
extern IUBMP280 iuAltimeter;

void BMX055AccelReadCallback(uint8_t wireStatus);
extern IUBMX055Acc iuAccelerometer;
extern IUBMX055Gyro iuGyroscope;
extern IUBMX055Mag iuMagnetometer;

#if defined(WITH_CAM_M8Q) || defined(WITH_MAX_M8Q)
    extern IUCAMM8Q iuGNSS;
#endif

extern IUICS43432 iuI2S;


/* =============================================================================
    Feature Computers
============================================================================= */

/***** Operation State Computer *****/

extern FeatureStateComputer opStateComputer;


// Shared computation space
extern q15_t allocatedFFTSpace[1024];


/***** Accelerometer Calibration parameters *****/

extern float ACCEL_RMS_SCALING[3];
extern float VELOCITY_RMS_SCALING[3];
extern float DISPLACEMENT_RMS_SCALING[3];


/***** Accelerometer Feature computation parameters *****/

extern uint16_t DEFAULT_LOW_CUT_FREQUENCY;
extern uint16_t DEFAULT_HIGH_CUT_FREQUENCY;
extern float DEFAULT_MIN_AGITATION;


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


/***** Accelerometer Calibration parameters *****/

extern float AUDIO_DB_SCALING;


/***** Audio Features *****/

extern AudioDBComputer audioDB2048Computer;
extern AudioDBComputer audioDB4096Computer;


/***** Set up sources *****/

extern void setUpComputerSources();


/* =============================================================================
    Feature Groups
============================================================================= */

/***** Sending Queue (for WiFi data publication) *****/
extern CharBufferSendingQueue sendingQueue;

/***** Default feature group *****/
extern FeatureGroup *DEFAULT_FEATURE_GROUP;

/***** Instantiation *****/
extern FeatureGroup healthCheckGroup;
extern FeatureGroup calibrationGroup;
extern FeatureGroup rawAccelGroup;
extern FeatureGroup pressStandardGroup;
extern FeatureGroup motorStandardGroup;

/***** Populate groups *****/
extern void populateFeatureGroups();

#endif // BUTTERFLY

#endif // INSTANCESBUTTERFLY_H
