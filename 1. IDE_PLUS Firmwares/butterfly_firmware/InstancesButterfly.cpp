#include "InstancesButterfly.h"

#if defined(BUTTERFLY_V03) || defined(BUTTERFLY_V04)

/* =============================================================================
    Interfaces
============================================================================= */

#ifdef BUTTERFLY_V04
    RGBLed rgbLed(A5, A3, A4);
#else
    RGBLed rgbLed(A5, A3, A0);
#endif

char iuUSBBuffer[20] = "";
IUUSB iuUSB(&Serial, iuUSBBuffer, 20, IUSerial::CUSTOM_PROTOCOL, 115200, '\n',
            1000);

char iuBluetoothBuffer[500] = "";
IUBMD350 iuBluetooth(&Serial1, iuBluetoothBuffer, 500,
                     IUSerial::LEGACY_PROTOCOL, 57600, ';', 2000, 38, 26);

char iuWiFiBuffer[500] = "";
#ifdef USE_EXTERNAL_WIFI
    IUSerial iuWiFi(&Serial3, iuWiFiBuffer, 500, IUSerial::LEGACY_PROTOCOL,
                    115200, ';', 250);
#else
    IUESP8285 iuWiFi(&Serial3, iuWiFiBuffer, 500, IUSerial::LEGACY_PROTOCOL,
                     115200, ';', 250);
#endif


/* =============================================================================
    Features
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
Q15Feature accelReducedFFTX("FAX", 2, 150, accelReducedFFTXValues,
                            Feature::FIXED, true);
Q15Feature accelReducedFFTY("FAY", 2, 150, accelReducedFFTYValues,
                            Feature::FIXED, true);
Q15Feature accelReducedFFTZ("FAZ", 2, 150, accelReducedFFTZValues,
                            Feature::FIXED, true);


// Acceleration main Frequency features from 512 sample long accel data
__attribute__((section(".noinit2"))) float accelMainFreqXValues[2];
__attribute__((section(".noinit2"))) float accelMainFreqYValues[2];
__attribute__((section(".noinit2"))) float accelMainFreqZValues[2];
FloatFeature accelMainFreqX("FRX", 2, 1, accelMainFreqXValues);
FloatFeature accelMainFreqY("FRY", 2, 1, accelMainFreqYValues);
FloatFeature accelMainFreqZ("FRZ", 2, 1, accelMainFreqZValues);

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


/***** RTD Temperature features *****/

#ifdef RTD_DAUGHTER_BOARD // Optionnal hardware
__attribute__((section(".noinit2"))) float rtdTempValues[8];
FloatFeature rtdTemp("RTD", 2, 4, rtdTempValues);
#endif // RTD_DAUGHTER_BOARD


/* =============================================================================
    Sensors
============================================================================= */

IUBattery iuBattery("BAT", &batteryLoad);

IUBMP280 iuAltimeter(&iuI2C, "ALT", &temperature, &pressure);

IUBMX055Acc iuAccelerometer(&iuI2C, "ACC", &accelerationX, &accelerationY,
                            &accelerationZ);
IUBMX055Gyro iuGyroscope(&iuI2C, "GYR", &tiltX, &tiltY, &tiltZ);
IUBMX055Mag iuMagnetometer(&iuI2C, "MAG", &magneticX, &magneticY, &magneticZ);

#ifdef NO_GPS
#else
    IUCAMM8Q iuGNSS(&Serial2, "GPS");
#endif

#ifdef BUTTERFLY_V04
    IUICS43432 iuI2S(&I2S1, "MIC", &audio);
#else
    IUICS43432 iuI2S(&I2S, "MIC", &audio);
#endif


/* =============================================================================
    Feature Computers
============================================================================= */


// Shared computation space
q15_t allocatedFFTSpace[1024];


// Note that computer_id 0 is reserved to designate an absence of computer.

/***** Accelerometer Features *****/

// 128 sample long accel computers
SignalRMSComputer accel128ComputerX(1,&accelRMS128X, false, true, true,
                                    ACCEL_RMS_SCALING[0]);
SignalRMSComputer accel128ComputerY(2, &accelRMS128Y, false, true, true,
                                    ACCEL_RMS_SCALING[1]);
SignalRMSComputer accel128ComputerZ(3, &accelRMS128Z, false, true, true,
                                    ACCEL_RMS_SCALING[2]);
MultiSourceSumComputer accelRMS128TotalComputer(4, &accelRMS128Total,
                                                false, false);


// 512 sample long accel computers
SectionSumComputer accel512ComputerX(5, 1, &accelRMS512X, NULL, NULL,
                                     true, false);
SectionSumComputer accel512ComputerY(6, 1, &accelRMS512Y, NULL, NULL,
                                     true, false);
SectionSumComputer accel512ComputerZ(7, 1, &accelRMS512Z, NULL, NULL,
                                     true, false);
SectionSumComputer accel512TotalComputer(8, 1, &accelRMS512Total, NULL, NULL,
                                         true, false);


// Computers for FFT feature from 512 sample long accel data
Q15FFTComputer accelFFTComputerX(9,
                                 &accelReducedFFTX,
                                 &accelMainFreqX,
                                 &velRMS512X,
                                 &dispRMS512X,
                                 allocatedFFTSpace,
                                 DEFAULT_LOW_CUT_FREQUENCY,
                                 DEFAULT_HIGH_CUT_FREQUENCY,
                                 DEFAULT_MIN_AGITATION,
                                 VELOCITY_RMS_SCALING[0],
                                 DISPLACEMENT_RMS_SCALING[0]);
Q15FFTComputer accelFFTComputerY(10,
                                 &accelReducedFFTY,
                                 &accelMainFreqY,
                                 &velRMS512Y,
                                 &dispRMS512Y,
                                 allocatedFFTSpace,
                                 DEFAULT_LOW_CUT_FREQUENCY,
                                 DEFAULT_HIGH_CUT_FREQUENCY,
                                 DEFAULT_MIN_AGITATION,
                                 VELOCITY_RMS_SCALING[1],
                                 DISPLACEMENT_RMS_SCALING[1]);
Q15FFTComputer accelFFTComputerZ(11,
                                 &accelReducedFFTZ,
                                 &accelMainFreqZ,
                                 &velRMS512Z,
                                 &dispRMS512Z,
                                 allocatedFFTSpace,
                                 DEFAULT_LOW_CUT_FREQUENCY,
                                 DEFAULT_HIGH_CUT_FREQUENCY,
                                 DEFAULT_MIN_AGITATION,
                                 VELOCITY_RMS_SCALING[2],
                                 DISPLACEMENT_RMS_SCALING[2]);


/***** Audio Features *****/

AudioDBComputer audioDB2048Computer(12, &audioDB2048, AUDIO_DB_SCALING);
AudioDBComputer audioDB4096Computer(13, &audioDB4096, AUDIO_DB_SCALING);


/***** Set up sources *****/

/**
 * Add sources to computer instances (must be called during main setup)
 */
void setUpComputerSources()
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
    Instantiation
============================================================================= */

/***** Default feature group *****/
FeatureGroup *DEFAULT_FEATURE_GROUP = &motorStandardGroup;

/***** Instantiation *****/
// Health Check
FeatureGroup healthCheckGroup("HEALTH", 45000);
// Calibration
FeatureGroup calibrationGroup("CAL001", 512);
// Raw acceleration data
FeatureGroup rawAccelGroup("RAWACC", 512);
// Standard Press Monitoring
FeatureGroup pressStandardGroup("PRSSTD", 512);
// Standard Motor Monitoring
FeatureGroup motorStandardGroup("MOTSTD", 512);


/***** Populate groups *****/

void populateFeatureGroups()
{
    /** Health Check **/
    healthCheckGroup.addFeature(&batteryLoad);
    // TODO => configure HealthCheckGroup

    /** Calibration **/
    calibrationGroup.addFeature(&velRMS512X);
    calibrationGroup.addFeature(&velRMS512Y);
    calibrationGroup.addFeature(&velRMS512Z);
    calibrationGroup.addFeature(&temperature);
    calibrationGroup.addFeature(&accelMainFreqX);
    calibrationGroup.addFeature(&accelMainFreqY);
    calibrationGroup.addFeature(&accelMainFreqZ);
    calibrationGroup.addFeature(&accelRMS512X);
    calibrationGroup.addFeature(&accelRMS512Y);
    calibrationGroup.addFeature(&accelRMS512Z);

    /** Raw acceleration data **/
    rawAccelGroup.addFeature(&accelerationX);
    rawAccelGroup.addFeature(&accelerationY);
    rawAccelGroup.addFeature(&accelerationZ);

    /** Standard Press Monitoring **/
    pressStandardGroup.addFeature(&accelRMS512Total);
    pressStandardGroup.addFeature(&accelRMS512X);
    pressStandardGroup.addFeature(&accelRMS512Y);
    pressStandardGroup.addFeature(&accelRMS512Z);
    pressStandardGroup.addFeature(&temperature);
    pressStandardGroup.addFeature(&audioDB4096);

    /** Standard Motor Monitoring **/
    motorStandardGroup.addFeature(&accelRMS512Total);
    motorStandardGroup.addFeature(&velRMS512X);
    motorStandardGroup.addFeature(&velRMS512Y);
    motorStandardGroup.addFeature(&velRMS512Z);
    motorStandardGroup.addFeature(&temperature);
    motorStandardGroup.addFeature(&audioDB4096);
}

#endif // BUTTERFLY
