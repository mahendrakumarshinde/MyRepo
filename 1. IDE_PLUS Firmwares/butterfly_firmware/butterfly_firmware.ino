/*
  Infinite Uptime BLE Module Firmware

  Update 2017-10-10
*/

/* =============================================================================
    Library imports
============================================================================= */

#include "Conductor.h"
#include "IUSPIFlash.h"  // FIXME For some reason, if this is included in
// conductor, it blocks the I2S callback

#include <MemoryFree.h>

/* Comment / Uncomment the "define" lines to toggle / untoggle unit or quality
test mode */

//#define UNITTEST
#ifdef UNITTEST
    #include "UnitTest/Test_Component.h"
    #include "UnitTest/Test_FeatureClass.h"
    #include "UnitTest/Test_FeatureComputer.h"
    #include "UnitTest/Test_FeatureGroup.h"
    #include "UnitTest/Test_Sensor.h"
    #include "UnitTest/Test_Utilities.h"
#endif

//#define INTEGRATEDTEST
#ifdef INTEGRATEDTEST
    #include "IntegratedTest/IT_Conductor.h"
    #include "IntegratedTest/IT_IUBMX055.h"
    #include "IntegratedTest/IT_IUSPIFlash.h"
    #include "IntegratedTest/IT_Sensors.h"
#endif


/* =============================================================================
    MAC Address
============================================================================= */

const char MAC_ADDRESS[18] = "94:54:93:0F:67:01";
    // "94:54:93:0E:81:44";
    // "94:54:93:0E:63:FC";
    // "94:54:93:0E:81:A4";
    // "94:54:93:0E:7B:2B";


/* =============================================================================
    Configuration Variables
============================================================================= */

/***** Accelerometer Calibration parameters *****/

float ACCEL_RMS_SCALING = 1.04;
float VELOCITY_RMS_SCALING = 1.;
float DISPLACEMENT_RMS_SCALING = 1.;


/***** Acceleration Energy 512 default thresholds *****/

float DEFAULT_ACCEL_ENERGY_NORMAL_TH = 2;  // 110;
float DEFAULT_ACCEL_ENERGY_WARNING_TH = 5;  // 130;
float DEFAULT_ACCEL_ENERGY_HIGH_TH = 8;  // 150;


/***** Accelerometer Feature computation parameters *****/

uint16_t DEFAULT_LOW_CUT_FREQUENCY = 5;  // Hz
uint16_t DEFAULT_HIGH_CUT_FREQUENCY = 500;  // Hz
float DEFAULT_MIN_AGITATION = 0.1;


/***** Audio DB calibration parameters *****/

float AUDIO_DB_SCALING = 1.1;


/* =============================================================================
    Main
============================================================================= */

/***** Debbugging variables *****/

bool doOnce = true;
uint32_t interval = 30000;
uint32_t lastDone = 0;


/***** Driven sensors acquisition callback *****/

/**
 * This function will be called every time the Microphone sends an interrupt.
 *
 * The interrupt is raised every time the raw sound data buffer is full. The
 * interrupt frequency then depends on the Microphone (here I2S) clock rate and
 * on the size of the buffer.
 * NB: Printing is time consuming and may cause issues in callback. Always
 * deactivate the callbackDebugMode in prod.
 */
void callback()
{
    uint32_t startT = 0;
    if (callbackDebugMode)
    {
        startT = micros();
    }
    conductor.acquireData(true);
    if (callbackDebugMode)
    {
        debugPrint(micros() - startT);
    }
}


/***** Begin *****/

Conductor conductor(MAC_ADDRESS);

void setup()
{
    #if defined(UNITTEST) || defined(INTEGRATEDTEST)
        iuUSB.begin();
        delay(2000);
        if (debugMode)
        {
            debugPrint(F("TESTING - Mem: "), false);
            debugPrint(String(freeMemory(), DEC));
            debugPrint(' ');
        }
        iuI2C.begin();
    #else
        iuUSB.begin();  // Start with USB for Serial communication
        if (debugMode)
        {
          debugPrint(F("Start - Mem: "), false);
          debugPrint(String(freeMemory(), DEC));
        }
        iuI2C.begin();
        // Interfaces
        if (debugMode)
        {
            debugPrint(F("\nInitializing interfaces..."));
        }
        if (setupDebugMode)
        {
            iuI2C.scanDevices();
            debugPrint("");
        }
        iuBluetooth.setupHardware();
        iuWiFi.setupHardware();
        iuSPIFlash.setupHardware();
        if(debugMode)
        {
            debugPrint(F("=> Successfully initialized interfaces - Mem: "),
                       false);
            debugPrint(String(freeMemory(), DEC));
        }
        if (setupDebugMode)
        {
            debugPrint(' ');
            iuBluetooth.exposeInfo();
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
            if (Sensor::instances[i]->isDriven())
            {
                Sensor::instances[i]->setCallbackRate(callbackRate);
            }
        }
        iuGyroscope.suspend();
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
        conductor.changeUsageMode(UsageMode::OPERATION);
        iuWiFi.preventFromSleeping();
    #endif
}

/**
 *
 * The regular calls to iuRGBLed.autoManage() manage any required LED blinking.
 */
void loop()
{
    #if defined(UNITTEST) || defined(INTEGRATEDTEST)
        Test::run();
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
        iuRGBLed.autoManage();
        // Configuration
        conductor.readFromSerial(StreamingMode::WIRED, &iuUSB);
        iuRGBLed.autoManage();
        conductor.readFromSerial(StreamingMode::BLE, &iuBluetooth);
        iuRGBLed.autoManage();
        conductor.readFromSerial(StreamingMode::WIFI, &iuWiFi);
        iuRGBLed.autoManage();
        // Acquire data from sensors
        conductor.acquireData(false);
        iuRGBLed.autoManage();
        // Feature computation depending on operation mode
        conductor.computeFeatures();
        iuRGBLed.autoManage();
        // Update the OperationState
        conductor.updateOperationState();
        iuRGBLed.autoManage();
        // Stream features
        conductor.streamFeatures();
        iuRGBLed.autoManage();
        uint32_t now = millis();
        if(lastDone == 0 || lastDone + interval < now || now < lastDone)
        {
            lastDone = now;
            /* === Place your code to excute at fixed interval here ===*/
            conductor.streamMCUUInfo();
            /*======*/
        }
        uint32_t stopYield = millis() + 10;
        while (millis() < stopYield)
        {
            yield();
        }
    #endif
}

