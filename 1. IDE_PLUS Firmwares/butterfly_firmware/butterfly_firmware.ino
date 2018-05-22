/*
Infinite Uptime BLE Module Firmware

Update 2017-10-10
*/

/* =============================================================================
    Library imports
============================================================================= */

#include "BoardDefinition.h"
#include "Conductor.h"

#include <MemoryFree.h>
#include <Timer.h>
#include <FS.h>

#ifdef DRAGONFLY_V03
#else
    // FIXME For some reason, if this is included in conductor,
    // it blocks the I2S callback
    #include "IUFlash.h"
    IUSPIFlash iuFlash(&SPI, A1, SPISettings(50000000, MSBFIRST, SPI_MODE0));
#endif


/* =============================================================================
    MAC Address
============================================================================= */

const char MAC_ADDRESS[18] = "94:54:93:10:63:DE";


/* =============================================================================
    Unit test includes
============================================================================= */

#ifdef UNITTEST
    #include <UnitTest/Test_IUSerial.h>
    #include "UnitTest/Test_Component.h"
    #include "UnitTest/Test_FeatureClass.h"
    #include "UnitTest/Test_FeatureComputer.h"
    #include "UnitTest/Test_FeatureGroup.h"
    #include "UnitTest/Test_Sensor.h"
    #include "UnitTest/Test_Utilities.h"
#endif

#ifdef INTEGRATEDTEST
    #include "IntegratedTest/IT_All_Conductor.h"
    #if defined(BUTTERFLY_V03) || defined(BUTTERFLY_V04)
        #include "IntegratedTest/IT_Butterfly.h"
    #elif defined(DRAGONFLY_V03)
        #include "IntegratedTest/IT_Dragonfly.h"
    #endif
#endif


/* =============================================================================
    Configuration Variables
============================================================================= */

/***** Accelerometer Calibration parameters *****/

float ACCEL_RMS_SCALING[3] = {
    1.00, // Axis X
    1.00, // Axis Y
    1.04, // Axis Z
};
float VELOCITY_RMS_SCALING[3] = {
    1.00, // Axis X
    1.00, // Axis Y
    1.00, // Axis Z
};
float DISPLACEMENT_RMS_SCALING[3] = {
    1.00, // Axis X
    1.00, // Axis Y
    1.00, // Axis Z
};


/***** Acceleration Energy 512 default thresholds *****/

float DEFAULT_ACCEL_ENERGY_NORMAL_TH = 110;
float DEFAULT_ACCEL_ENERGY_WARNING_TH = 130;
float DEFAULT_ACCEL_ENERGY_HIGH_TH = 150;


/***** Accelerometer Feature computation parameters *****/

uint16_t DEFAULT_LOW_CUT_FREQUENCY = 5;  // Hz
uint16_t DEFAULT_HIGH_CUT_FREQUENCY = 500;  // Hz
float DEFAULT_MIN_AGITATION = 0.1;


/***** Audio DB calibration parameters *****/

float AUDIO_DB_SCALING = 1.0;


/* =============================================================================
    Main
============================================================================= */

/***** Debbugging variables *****/

bool doOnce = true;
uint32_t interval = 30000;
uint32_t lastDone = 0;


/***** Main operator *****/

Conductor conductor(MAC_ADDRESS);


/***** Driven sensors acquisition callback *****/

/**
 * This function will be called every time the Microphone sends an interrupt.
 *
 * The interrupt is raised every time the raw sound data buffer is full. The
 * interrupt frequency then depends on the Microphone (here I2S) clock rate and
 * on the size of the buffer.
 * NB: Printing is time consuming and may cause issues in callback. Always
 * deactivate the asyncDebugMode in prod.
 */
void callback()
{
    uint32_t startT = 0;
    if (asyncDebugMode)
    {
        startT = micros();
    }
    conductor.acquireData(true);
    if (asyncDebugMode)
    {
        debugPrint(micros() - startT);
    }
}


/***** Led callbacks *****/

static armv7m_timer_t ledShowTimer;

static void ledUShowCallback(void) {
    rgbLed.updateColors();
    armv7m_timer_start(&ledShowTimer, 1);
}


static armv7m_timer_t ledTransitionTimer;

static void ledTransitionCallback(void) {
    rgbLed.manageColorTransitions();
    armv7m_timer_start(&ledTransitionTimer, 10);
}


/***** Watch Dog *****/

static armv7m_timer_t watchdogTimer;
uint32_t lastActive = 0;
uint32_t loopTimeout = 60000;  // 1min timeout

static void watchdogCallback(void) {
    if (lastActive > 0 && millis() - lastActive > loopTimeout)
    {
        STM32.reset();
    }
    armv7m_timer_start(&watchdogTimer, 1000);
}


/***** Begin *****/

void setup()
{
    iuUSB.begin();
    rgbLed.setupHardware();
    armv7m_timer_create(&ledShowTimer, (armv7m_timer_callback_t)ledUShowCallback);
    armv7m_timer_start(&ledShowTimer, 1);
    armv7m_timer_create(&ledTransitionTimer, (armv7m_timer_callback_t)ledTransitionCallback);
    armv7m_timer_start(&ledTransitionTimer, 10);
    #if defined(UNITTEST) || defined(COMPONENTTEST) || defined(INTEGRATEDTEST)
        delay(2000);
        iuI2C.begin();
    #else
        armv7m_timer_create(&watchdogTimer, (armv7m_timer_callback_t)watchdogCallback);
        armv7m_timer_start(&watchdogTimer, 1000);
        if (debugMode)
        {
            delay(5000);
            debugPrint(F("Start - Mem: "), false);
            debugPrint(String(freeMemory(), DEC));
        }
        iuI2C.begin();
        // Interfaces
        if (debugMode)
        {
            debugPrint(F("\nInitializing interfaces..."));
        }
        iuBluetooth.setupHardware();
        iuWiFi.setupHardware();
        if (setupDebugMode)
        {
            iuI2C.scanDevices();
            debugPrint("");
        }
        if (debugMode)
        {
            debugPrint(F("=> Successfully initialized interfaces - Mem: "),
                       false);
            debugPrint(String(freeMemory(), DEC));
        }
        // Default feature configuration
        if (debugMode)
        {
            debugPrint(F("\nSetting up default feature configuration..."));
        }
        conductor.setCallback(callback);
        setUpComputerSources();
        populateFeatureGroups();
        if (debugMode)
        {
            debugPrint(F("=> Succesfully configured default features - Mem: "),
                       false);
            debugPrint(String(freeMemory(), DEC));
        }
        // Sensors
        if (debugMode)
        {
            debugPrint(F("\nInitializing sensors..."));
        }
        uint16_t callbackRate = iuI2S.getCallbackRate();
        for (uint8_t i = 0; i < Sensor::instanceCount; ++i)
        {
            Sensor::instances[i]->setupHardware();
            if (Sensor::instances[i]->isHighFrequency())
            {
                Sensor::instances[i]->setCallbackRate(callbackRate);
            }
        }
        #ifdef BUTTERFLY_V04
            iuGyroscope.setPowerMode(PowerMode::SUSPEND);
        #endif
        if (debugMode)
        {
            debugPrint(F("=> Succesfully initialized sensors - Mem: "),
                       false);
            debugPrint(String(freeMemory(), DEC));
        }
        if (setupDebugMode)
        {
            conductor.exposeAllConfigurations();
            if (iuI2C.isError())
            {
                debugPrint(F("\nI2C Satus: Error"));
            }
            else
            {
                debugPrint(F("\nI2C Satus: OK"));
            }
            debugPrint(F("\n***Finished setup at (ms): "), false);
            debugPrint(millis(), false);
            debugPrint(F("***\n"));
        }
        // Start flash and load configuration files
        if (!USBDevice.configured())
        {
            iuFlash.begin();
            // WiFi configuration
            conductor.configureFromFlash(IUFlash::CFG_WIFI0);
            // Feature, FeatureGroup and sensors coonfigurations
            for (uint8_t i = 0; i < conductor.CONFIG_TYPE_COUNT; ++i)
            {
                conductor.configureFromFlash(conductor.CONFIG_TYPES[i]);
            }
            if (setupDebugMode)
            {
                conductor.overrideLedColor(RGB_PURPLE);
                delay(5000);
                conductor.resetLed();
            }
        }
        else if (setupDebugMode)
        {
            conductor.overrideLedColor(RGB_ORANGE);
            delay(5000);
            conductor.resetLed();
        }
        conductor.changeUsageMode(UsageMode::OPERATION);
    #endif
}

/**
 * 
 */
void loop()
{
    lastActive = millis();
    #if defined(UNITTEST) || defined(COMPONENTTEST) || defined(INTEGRATEDTEST)
        Test::run();
        if (Test::getCurrentFailed() > 0)
        {
            conductor.showStatusOnLed(RGB_RED);
        }
        else
        {
            conductor.showStatusOnLed(RGB_GREEN);
        }
    #else
        if (loopDebugMode)
        {
            if (doOnce)
            {
                doOnce = false;
                /* === Place your code to excute once here ===*/
                if (setupDebugMode)
                {
                    debugPrint(F("Loop - Mem: "),
                               false);
                    debugPrint(String(freeMemory(), DEC));
                }
                debugPrint(' ');
                /*======*/
            }
        }
        // Power saving
        conductor.manageSleepCycles();
        // Configuration
        conductor.readFromSerial(StreamingMode::WIRED, &iuUSB);
        conductor.readFromSerial(StreamingMode::BLE, &iuBluetooth);
        conductor.readFromSerial(StreamingMode::WIFI, &iuWiFi);
        // Acquire data from sensors
        conductor.acquireData(false);
        // Feature computation depending on operation mode
        conductor.computeFeatures();
        // Update the OperationState
        conductor.updateOperationState();
        // Stream features
        conductor.streamFeatures();
        // Send accel raw data
        conductor.periodicSendAccelRawData();
        // Send config checksum
        conductor.periodicSendConfigChecksum();
        uint32_t now = millis();
        if (lastDone == 0 || lastDone + interval < now || now < lastDone)
        {
            lastDone = now;
            /* === Place your code to excute at fixed interval here ===*/
            conductor.streamMCUUInfo(iuWiFi.port);
            /*======*/
        }
        yield();
    #endif
}

