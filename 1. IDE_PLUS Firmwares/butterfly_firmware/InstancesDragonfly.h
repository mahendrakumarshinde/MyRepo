#ifndef INSTANCESDRAGONFLY_H
#define INSTANCESDRAGONFLY_H

#include "BoardDefinition.h"

#ifdef DRAGONFLY_V03

/***** Interfaces *****/
#include "RGBLed.h"
#include "IUUSB.h"
#include "IUBMD350.h"
#ifdef USE_EXTERNAL_WIFI
#else
    #include "IUESP8285.h"
#endif

/***** Flash storage *****/
#include "IUFlash.h"

//#include<FS>h>

/****** Diagnostic Engine ****/
#include "DiagnosticFingerPrint.h"
#include "IUEthernet.h"

/***** Features *****/
#include "FeatureClass.h"
#include "FeatureComputer.h"
#include "FeatureGroup.h"
#include "CharBufferSendingQueue.h"

/***** Sensors *****/
#include "IUBattery.h"
#include "IUCAMM8Q.h"
#include "IUICS43432.h"
#include "IULSM6DSM.h"
#include "IUMAX31865.h"
#include "IUTMP116.h"
#include "IUKX222.h"
#include "IUkx224reg.h"
#include "IUOTA.h"
#include "Modbus_slave.h"

/***** Managers and helpers *****/
#include "LedManager.h"

#ifdef COMPONENTTEST
    // Interfaces
    #include "ComponentTest/CMP_IUBMD350.h"
    #include "ComponentTest/CMP_IUESP8285.h"
    // Sensors
    #include "ComponentTest/CMP_IUBattery.h"
    #include "ComponentTest/CMP_IUCAMM8Q.h"
    #include "ComponentTest/CMP_IUICS43432.h"
    #include "ComponentTest/CMP_IULSM6DSM.h"
    #include "ComponentTest/CMP_IUMAX31865.h"
#endif


/* =============================================================================
    Interfaces
============================================================================= */

extern GPIORGBLed rgbLed;

#ifdef USE_LED_STRIP
    extern APA102RGBLedStrip rgbLedStrip;
#endif

extern LedManager ledManager;

extern char iuUSBBuffer[4096];    // increase buffer size to 4K
extern IUUSB iuUSB;

extern char iuBluetoothBuffer[500]; //1024
extern IUBMD350 iuBluetooth;

extern char iuWiFiBuffer[2048];   //500

#ifdef USE_EXTERNAL_WIFI
    extern IUSerial iuWiFi;
#else
    extern IUESP8285 iuWiFi;
#endif

extern char iuEthernetBuffer[2048];

extern Usr2Eth iuEthernet;

extern IUmodbus iuModbusSlave;
/* =============================================================================
    OTA
============================================================================= */
extern IUOTA iuOta;
/* =============================================================================
    Flash storage
============================================================================= */

extern IUFSFlash iuFlash;

//extern DOSFS iuDOSFS;
/* ==============================================================================
 *  Diagnostic Engine 
 *==============================================================================*/

extern DiagnosticEngine iuDiagnosticEngine;      //object


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
extern __attribute__((section(".noinit2"))) q15_t accelerationXValues[8192/2];      // 1024 
extern __attribute__((section(".noinit2"))) q15_t accelerationYValues[8192/2];
extern __attribute__((section(".noinit2"))) q15_t accelerationZValues[8192/2];
extern FeatureTemplate<q15_t> accelerationX;
extern FeatureTemplate<q15_t> accelerationY;
extern FeatureTemplate<q15_t> accelerationZ;


// 128 sample long accel features
extern __attribute__((section(".noinit2"))) float accelRMS128XValues[8];  //8
extern __attribute__((section(".noinit2"))) float accelRMS128YValues[8];
extern __attribute__((section(".noinit2"))) float accelRMS128ZValues[8];
extern __attribute__((section(".noinit2"))) float accelRMS128TotalValues[8];
extern FeatureTemplate<float> accelRMS128X;
extern FeatureTemplate<float> accelRMS128Y;
extern FeatureTemplate<float> accelRMS128Z;
extern FeatureTemplate<float> accelRMS128Total;

// 512 sample long accel features
extern __attribute__((section(".noinit2"))) float accelRMS512XValues[2];    //2
extern __attribute__((section(".noinit2"))) float accelRMS512YValues[2];
extern __attribute__((section(".noinit2"))) float accelRMS512ZValues[2];
extern __attribute__((section(".noinit2"))) float accelRMS512TotalValues[2];
extern FeatureTemplate<float> accelRMS512X;
extern FeatureTemplate<float> accelRMS512Y;
extern FeatureTemplate<float> accelRMS512Z;
extern FeatureTemplate<float> accelRMS512Total;

// FFT feature from 512 sample long accel data
extern __attribute__((section(".noinit2"))) q15_t accelReducedFFTXValues[300];  //300
extern __attribute__((section(".noinit2"))) q15_t accelReducedFFTYValues[300];
extern __attribute__((section(".noinit2"))) q15_t accelReducedFFTZValues[300];
extern FeatureTemplate<q15_t> accelReducedFFTX;
extern FeatureTemplate<q15_t> accelReducedFFTY;
extern FeatureTemplate<q15_t> accelReducedFFTZ;

// Acceleration main Frequency features from 512 sample long accel data
extern __attribute__((section(".noinit2"))) float accelMainFreqXValues[2];  //2
extern __attribute__((section(".noinit2"))) float accelMainFreqYValues[2];
extern __attribute__((section(".noinit2"))) float accelMainFreqZValues[2];
extern FeatureTemplate<float> accelMainFreqX;
extern FeatureTemplate<float> accelMainFreqY;
extern FeatureTemplate<float> accelMainFreqZ;

// Velocity features from 512 sample long accel data
extern __attribute__((section(".noinit2"))) float velRMS512XValues[2]; //2
extern __attribute__((section(".noinit2"))) float velRMS512YValues[2];
extern __attribute__((section(".noinit2"))) float velRMS512ZValues[2];
extern FeatureTemplate<float> velRMS512X;
extern FeatureTemplate<float> velRMS512Y;
extern FeatureTemplate<float> velRMS512Z;

// Displacements features from 512 sample long accel data
extern __attribute__((section(".noinit2"))) float dispRMS512XValues[2]; //2
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


/***** Barometer Features *****/

// Sensor data
extern __attribute__((section(".noinit2"))) float temperatureAValues[2];
extern FeatureTemplate<float> temperatureA;
extern __attribute__((section(".noinit2"))) float temperatureBValues[2];
extern FeatureTemplate<float> temperatureB;
// Temperaute measured on the TMP116
extern __attribute__((section(".noinit2"))) float allTemperatureValues[2];
extern FeatureTemplate<float> allTemperatures;
extern __attribute__((section(".noinit2"))) float temperatureValues[2];
extern FeatureTemplate<float> temperature;


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

extern IUMAX31865 iuRTDSensorA;
extern IUMAX31865 iuRTDSensorB;

void LSM6DSMAccelReadCallback(uint8_t wireStatus);
extern IULSM6DSM iuAccelerometer;

void KX222AccelReadCallback();
extern IUKX222 iuAccelerometerKX222;

void TMP116TempReadCallback(uint8_t wireStatus);
extern IUTMP116 iuTemp;

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
extern q15_t allocatedFFTSpace[8192]; // 1024


/***** Accelerometer Calibration parameters *****/

extern float ACCEL_RMS_SCALING[3];
extern float VELOCITY_RMS_SCALING[3];
extern float DISPLACEMENT_RMS_SCALING[3];

/***** Accelerometer Features *****/

// 128 sample long accel computers
extern SignalRMSComputer<q15_t> accel128ComputerX;
extern SignalRMSComputer<q15_t> accel128ComputerY;
extern SignalRMSComputer<q15_t> accel128ComputerZ;
extern MultiSourceSumComputer accelRMS128TotalComputer;

// 512 sample long accel computers
extern SectionSumComputer accel512ComputerX;
extern SectionSumComputer accel512ComputerY;
extern SectionSumComputer accel512ComputerZ;
extern SectionSumComputer accel512TotalComputer;


// computers for FFT feature from 512 sample long accel data
extern FFTComputer<q15_t> accelFFTComputerX;
extern FFTComputer<q15_t> accelFFTComputerY;
extern FFTComputer<q15_t> accelFFTComputerZ;


// 512 sample long temperature computer
extern AverageComputer temperatureAverager;


/***** Accelerometer Calibration parameters *****/

extern float AUDIO_DB_SCALING;
extern float AUDIO_DB_OFFSET;


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
extern FeatureGroup bearingZGroup;
extern FeatureGroup motorAccelGroup;
extern FeatureGroup ForgingDispGroup;

/***** Populate groups *****/
extern void populateFeatureGroups();

#endif // DRAGONFLY

#endif // INSTANCESDRAGONFLY_H
