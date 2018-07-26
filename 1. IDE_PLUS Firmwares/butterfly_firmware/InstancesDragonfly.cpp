#include "InstancesDragonfly.h"

#ifdef DRAGONFLY_V03

/* =============================================================================
    Interfaces
============================================================================= */

GPIORGBLed rgbLed(25, 26, 38);

#ifdef USE_LED_STRIP
    APA102RGBLedStrip rgbLedStrip(16);
    LedManager ledManager(&rgbLed, &rgbLedStrip);
#else
    LedManager ledManager(&rgbLed);
#endif

char iuUSBBuffer[20] = "";
IUUSB iuUSB(&Serial, iuUSBBuffer, 20, IUSerial::CUSTOM_PROTOCOL, 115200,
            '\n', 1000);

char iuBluetoothBuffer[500] = "";
IUBMD350 iuBluetooth(&Serial3, iuBluetoothBuffer, 500,
                     IUSerial::LEGACY_PROTOCOL, 57600, ';', 2000, 39, 40);

char iuWiFiBuffer[500] = "";
#ifdef USE_EXTERNAL_WIFI
    IUSerial iuWiFi(&Serial1, iuWiFiBuffer, 500, IUSerial::MS_PROTOCOL, 115200,
                    ';', 250);
#else
    IUESP8285 iuWiFi(&Serial1, iuWiFiBuffer, 500, IUSerial::MS_PROTOCOL,
                     115200, ';', 250);
#endif


/* =============================================================================
    Flash storage
============================================================================= */

IUFSFlash iuFlash = IUFSFlash();


/* =============================================================================
    Features
============================================================================= */

/***** Operation State Monitoring feature *****/

q15_t opStateFeatureValues[2] = {0, 0};
FeatureTemplate<q15_t> opStateFeature("OPS", 2, 1, opStateFeatureValues);


/***** Battery load *****/

float batteryLoadValues[2];
FeatureTemplate<float> batteryLoad("BAT", 2, 1, batteryLoadValues);


/***** Accelerometer Features *****/

// Sensor data
__attribute__((section(".noinit2"))) q15_t accelerationXValues[1024];
__attribute__((section(".noinit2"))) q15_t accelerationYValues[1024];
__attribute__((section(".noinit2"))) q15_t accelerationZValues[1024];
FeatureTemplate<q15_t> accelerationX("A0X", 8, 128, accelerationXValues);
FeatureTemplate<q15_t> accelerationY("A0Y", 8, 128, accelerationYValues);
FeatureTemplate<q15_t> accelerationZ("A0Z", 8, 128, accelerationZValues);


// 128 sample long accel features
__attribute__((section(".noinit2"))) float accelRMS128XValues[8];
__attribute__((section(".noinit2"))) float accelRMS128YValues[8];
__attribute__((section(".noinit2"))) float accelRMS128ZValues[8];
__attribute__((section(".noinit2"))) float accelRMS128TotalValues[8];
FeatureTemplate<float> accelRMS128X("A7X", 2, 4, accelRMS128XValues);
FeatureTemplate<float> accelRMS128Y("A7Y", 2, 4, accelRMS128YValues);
FeatureTemplate<float> accelRMS128Z("A7Z", 2, 4, accelRMS128ZValues);
FeatureTemplate<float> accelRMS128Total("A73", 2, 4, accelRMS128TotalValues);


// 512 sample long accel features
__attribute__((section(".noinit2"))) float accelRMS512XValues[2];
__attribute__((section(".noinit2"))) float accelRMS512YValues[2];
__attribute__((section(".noinit2"))) float accelRMS512ZValues[2];
__attribute__((section(".noinit2"))) float accelRMS512TotalValues[2];
FeatureTemplate<float> accelRMS512X("A9X", 2, 1, accelRMS512XValues);
FeatureTemplate<float> accelRMS512Y("A9Y", 2, 1, accelRMS512YValues);
FeatureTemplate<float> accelRMS512Z("A9Z", 2, 1, accelRMS512ZValues);
FeatureTemplate<float> accelRMS512Total("A93", 2, 1, accelRMS512TotalValues);


// FFT feature from 512 sample long accel data
__attribute__((section(".noinit2"))) q15_t accelReducedFFTXValues[300];
__attribute__((section(".noinit2"))) q15_t accelReducedFFTYValues[300];
__attribute__((section(".noinit2"))) q15_t accelReducedFFTZValues[300];
FeatureTemplate<q15_t> accelReducedFFTX("FAX", 2, 150, accelReducedFFTXValues,
                                        Feature::FIXED, true);
FeatureTemplate<q15_t> accelReducedFFTY("FAY", 2, 150, accelReducedFFTYValues,
                                        Feature::FIXED, true);
FeatureTemplate<q15_t> accelReducedFFTZ("FAZ", 2, 150, accelReducedFFTZValues,
                                        Feature::FIXED, true);


// Acceleration main Frequency features from 512 sample long accel data
__attribute__((section(".noinit2"))) float accelMainFreqXValues[2];
__attribute__((section(".noinit2"))) float accelMainFreqYValues[2];
__attribute__((section(".noinit2"))) float accelMainFreqZValues[2];
FeatureTemplate<float> accelMainFreqX("FRX", 2, 1, accelMainFreqXValues);
FeatureTemplate<float> accelMainFreqY("FRY", 2, 1, accelMainFreqYValues);
FeatureTemplate<float> accelMainFreqZ("FRZ", 2, 1, accelMainFreqZValues);

// Velocity features from 512 sample long accel data
__attribute__((section(".noinit2"))) float velRMS512XValues[2];
__attribute__((section(".noinit2"))) float velRMS512YValues[2];
__attribute__((section(".noinit2"))) float velRMS512ZValues[2];
FeatureTemplate<float> velRMS512X("VAX", 2, 1, velRMS512XValues);
FeatureTemplate<float> velRMS512Y("VAY", 2, 1, velRMS512YValues);
FeatureTemplate<float> velRMS512Z("VAZ", 2, 1, velRMS512ZValues);

// Displacements features from 512 sample long accel data
__attribute__((section(".noinit2"))) float dispRMS512XValues[2];
__attribute__((section(".noinit2"))) float dispRMS512YValues[2];
__attribute__((section(".noinit2"))) float dispRMS512ZValues[2];
FeatureTemplate<float> dispRMS512X("DAX", 2, 1, dispRMS512XValues);
FeatureTemplate<float> dispRMS512Y("DAX", 2, 1, dispRMS512YValues);
FeatureTemplate<float> dispRMS512Z("DAZ", 2, 1, dispRMS512ZValues);


/***** Gyroscope Features *****/

// Sensor data
__attribute__((section(".noinit2"))) q15_t tiltXValues[2];
__attribute__((section(".noinit2"))) q15_t tiltYValues[2];
__attribute__((section(".noinit2"))) q15_t tiltZValues[2];
FeatureTemplate<q15_t> tiltX("T0X", 2, 1, tiltXValues);
FeatureTemplate<q15_t> tiltY("T0Y", 2, 1, tiltYValues);
FeatureTemplate<q15_t> tiltZ("T0Z", 2, 1, tiltZValues);


/***** Magnetometer Features *****/

// Sensor data
__attribute__((section(".noinit2"))) q15_t magneticXValues[2];
__attribute__((section(".noinit2"))) q15_t magneticYValues[2];
__attribute__((section(".noinit2"))) q15_t magneticZValues[2];
FeatureTemplate<q15_t> magneticX("M0X", 2, 1, magneticXValues);
FeatureTemplate<q15_t> magneticY("M0Y", 2, 1, magneticYValues);
FeatureTemplate<q15_t> magneticZ("M0Z", 2, 1, magneticZValues);


/***** Barometer Features *****/

// Sensor data
__attribute__((section(".noinit2"))) float temperatureAValues[2];
FeatureTemplate<float> temperatureA("TMA", 2, 1, temperatureAValues);

__attribute__((section(".noinit2"))) float temperatureBValues[2];
FeatureTemplate<float> temperatureB("TMB", 2, 1, temperatureBValues);

// Temperaute measured on the LSM6DSM
__attribute__((section(".noinit2"))) float temperatureValues[2];
FeatureTemplate<float> temperature("TMP", 2, 1, temperatureValues);

/***** Audio Features *****/

// Sensor data
q15_t audioValues[8192];
FeatureTemplate<q15_t> audio("SND", 2, 2048, audioValues);

// 2048 sample long features
__attribute__((section(".noinit2"))) float audioDB2048Values[4];
FeatureTemplate<float> audioDB2048("S11", 4, 1, audioDB2048Values);

// 4096 sample long features
__attribute__((section(".noinit2"))) float audioDB4096Values[2];
FeatureTemplate<float> audioDB4096("S12", 2, 1, audioDB4096Values);


/***** GNSS Feature *****/


/***** RTD Temperature features *****/

#ifdef RTD_DAUGHTER_BOARD // Optionnal hardware
__attribute__((section(".noinit2"))) float rtdTempValues[8];
FeatureTemplate<float> rtdTemp("RTD", 2, 4, rtdTempValues);
#endif // RTD_DAUGHTER_BOARD


/* =============================================================================
    Sensors
============================================================================= */

IUBattery iuBattery("BAT", &batteryLoad);

IUMAX31865 iuRTDSensorA(&SPI1, 42, SPISettings(500000, MSBFIRST, SPI_MODE1),
                        "THA", &temperatureA);
IUMAX31865 iuRTDSensorB(&SPI1, 43, SPISettings(500000, MSBFIRST, SPI_MODE1),
                        "THB", &temperatureB);

IULSM6DSM iuAccelerometer(&iuI2C, "ACC", &accelerationX, &accelerationY,
                          &accelerationZ, &tiltX, &tiltY, &tiltZ,
                          &temperature);

#ifdef WITH_CAM_M8Q
    IUCAMM8Q iuGNSS(&Serial2, "GPS", -1);
#elif defined(WITH_MAX_M8Q)
    IUCAMM8Q iuGNSS(&Serial2, "GPS", 10);
#endif

IUICS43432 iuI2S(&I2S, "MIC", &audio);



/* =============================================================================
    Feature Computers
============================================================================= */

/***** Operation State Computer *****/

FeatureStateComputer opStateComputer(1, &opStateFeature);


// Shared computation space
q15_t allocatedFFTSpace[1024];


// Note that computer_id 0 is reserved to designate an absence of computer.

/***** Accelerometer Features *****/

// 128 sample long accel computers
SignalRMSComputer accel128ComputerX(10,&accelRMS128X, false, true, true,
                                    ACCEL_RMS_SCALING[0]);
SignalRMSComputer accel128ComputerY(11, &accelRMS128Y, false, true, true,
                                    ACCEL_RMS_SCALING[1]);
SignalRMSComputer accel128ComputerZ(12, &accelRMS128Z, false, true, true,
                                    ACCEL_RMS_SCALING[2]);
MultiSourceSumComputer accelRMS128TotalComputer(13, &accelRMS128Total,
                                                false, false);


// 512 sample long accel computers
SectionSumComputer accel512ComputerX(20, 1, &accelRMS512X, NULL, NULL,
                                     true, false);
SectionSumComputer accel512ComputerY(21, 1, &accelRMS512Y, NULL, NULL,
                                     true, false);
SectionSumComputer accel512ComputerZ(22, 1, &accelRMS512Z, NULL, NULL,
                                     true, false);
SectionSumComputer accel512TotalComputer(23, 1, &accelRMS512Total, NULL, NULL,
                                         true, false);


// Computers for FFT feature from 512 sample long accel data
Q15FFTComputer accelFFTComputerX(30,
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
Q15FFTComputer accelFFTComputerY(31,
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
Q15FFTComputer accelFFTComputerZ(32,
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

AudioDBComputer audioDB2048Computer(40, &audioDB2048, AUDIO_DB_SCALING);
AudioDBComputer audioDB4096Computer(41, &audioDB4096, AUDIO_DB_SCALING);


/***** Set up sources *****/

/**
 * Add sources to computer instances (must be called during main setup)
 */
void setUpComputerSources()
{
    // Operation State feature and computer always active
    opStateFeature.setAlwaysRequired(true);
    opStateComputer.activate();
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
    Feature Groups
============================================================================= */

/***** Sending Queue (for WiFi data publication) *****/
CharBufferSendingQueue sendingQueue = CharBufferSendingQueue();

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
// Bearing monitoring
FeatureGroup bearingZGroup("BEAR_Z", 512);
// Motor monitoring with focus on acceleration for each axis
FeatureGroup motorAccelGroup("MOTACC", 512);
// Forging Monitoring with focus on displacements on each axis
FeatureGroup ForgingDispGroup("PRSDIS", 512);


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

    /** Bearing monitoring **/
    bearingZGroup.addFeature(&accelRMS512Total);
    bearingZGroup.addFeature(&velRMS512X);
    bearingZGroup.addFeature(&velRMS512Y);
    bearingZGroup.addFeature(&velRMS512Z);
    bearingZGroup.addFeature(&dispRMS512Z);
    bearingZGroup.addFeature(&audioDB4096);

    /** Motor monitoring with focus on acceleration for each axis **/
    motorAccelGroup.addFeature(&accelRMS512Total);
    motorAccelGroup.addFeature(&accelRMS512X);
    motorAccelGroup.addFeature(&accelRMS512Y);
    motorAccelGroup.addFeature(&accelRMS512Z);
    motorAccelGroup.addFeature(&temperature);
    motorAccelGroup.addFeature(&audioDB4096);

    /** Forging Monitoring with focus on displacements on each axis **/
    ForgingDispGroup.addFeature(&accelRMS512Total);
    ForgingDispGroup.addFeature(&dispRMS512X);
    ForgingDispGroup.addFeature(&dispRMS512Y);
    ForgingDispGroup.addFeature(&dispRMS512Z);
    ForgingDispGroup.addFeature(&temperature);
    ForgingDispGroup.addFeature(&audioDB4096);
}

#endif // DRAGONFLY
