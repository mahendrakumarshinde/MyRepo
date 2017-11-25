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


/* =============================================================================
    Main
============================================================================= */

/***** Configuration variables *****/

bool doOnce = true;
uint32_t interval = 500;
uint32_t lastDone = 0;


/***** Asynchronous acquisition callback *****/

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
        iuSerial3.setupHardware();
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
            if (Sensor::instances[i]->isAsynchronous())
            {
                Sensor::instances[i]->setCallbackRate(callbackRate);
            }
        }
        if (debugMode)
        {
          memoryLog(F("=> Successfully initialized sensors"));
        }
//        conductor.activateGroup(&motorStandardGroup);
//        accelRMS512Total.enableOperationState();
//        accelRMS512Total.setThresholds(110, 130, 150);
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
//        conductor.changeAcquisitionMode(AcquisitionMode::FEATURE);
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
            if(now - lastDone > interval || now < lastDone)
            {
                lastDone = now;
                /* === Place your code to excute at fixed interval here ===*/

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
        // Acquire data from synchronous sensor
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
    #endif
}

