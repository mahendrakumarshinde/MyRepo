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
        Serial.begin(115200);
        delay(2000);
        memoryLog("TESTING");
        Serial.println(' ');
        iuI2C.setupHardware();
    #else
        iuUSB.setupHardware();  // Start with USB for Serial communication
        if (debugMode)
        {
          memoryLog("Start");
        }
        // Interfaces
        if (debugMode)
        {
            debugPrint(F("\nInitializing interfaces..."));
        }
        iuI2C.setupHardware();
        if (setupDebugMode)
        {
            iuI2C.scanDevices();
            debugPrint("");
        }
        iuBluetooth.setForceMessageSize(19);
        iuBluetooth.setupHardware();
        iuWiFi.setupHardware();
        iuSPIFlash.setupHardware();
        if(debugMode)
        {
            memoryLog(F("=> Successfully initialized interfaces"));
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
            memoryLog(F("=> Succesfully configured default features"));
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
        if (debugMode)
        {
          memoryLog(F("=> Successfully initialized sensors"));
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
    #endif
}

/**
 *
 * The regular calls to iuRGBLed.autoTurnOff() turns the LEDs off to save
 * power, once they've been lit long enough based on their internal timer.
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
                if (setupDebugMode) { memoryLog(F("Loop")); }
                debugPrint(' ');
                /*======*/
            }
            uint32_t now = millis();
            if(lastDone == 0 || lastDone + interval < now || now < lastDone)
            {
                lastDone = now;
                /* === Place your code to excute at fixed interval here ===*/
                debugPrint(now, false);
                debugPrint(": ", false);
                memoryLog("Loop");
                /*======*/
            }
        }
        // Power saving
        conductor.manageSleepCycles();
        iuRGBLed.autoTurnOff();
        // Configuration
        conductor.readFromSerial(&iuUSB);
        iuRGBLed.autoTurnOff();
        conductor.readFromSerial(&iuBluetooth);
        iuRGBLed.autoTurnOff();
        conductor.readFromSerial(&iuWiFi);
        iuRGBLed.autoTurnOff();
        // Acquire data from sensors
        conductor.acquireData(false);
        iuRGBLed.autoTurnOff();
        // Feature computation depending on operation mode
        conductor.computeFeatures();
        iuRGBLed.autoTurnOff();
        // Update the OperationState
        conductor.updateOperationState();
        iuRGBLed.autoTurnOff();
        // Stream features
        conductor.streamFeatures();
        iuRGBLed.autoTurnOff();
        uint32_t stopYield = millis() + 10;
        while (millis() < stopYield)
        {
            yield();
        }
    #endif
}

