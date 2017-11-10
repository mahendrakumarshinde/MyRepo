/*
  Infinite Uptime BLE Module Firmware

  Update 2017-10-10
*/

/* =============================================================================
    Library imports
============================================================================= */


#include "Conductor.h"
#include "Configurator.h"

/* Comment / Uncomment the "define" lines to toggle / untoggle unit or quality
test mode */

//#define UNITTEST
#ifdef UNITTEST
  #include "UnitTest/Test_Component.h"
  #include "UnitTest/Test_Configurator.h"
  #include "UnitTest/Test_FeatureClass.h"
  #include "UnitTest/Test_FeatureComputer.h"
  #include "UnitTest/Test_FeatureGraph.h"
  #include "UnitTest/Test_FeatureProfile.h"
  #include "UnitTest/Test_Sensor.h"
  #include "UnitTest/Test_Utilities.h"
#endif

//#define QUALITYTEST
#ifdef QUALITYTEST
  #include "QualityTest/QA_IUBMX055.h"
#endif


/* =============================================================================
    MAC Address and Firmware version
============================================================================= */

const char FIRMWARE_VERSION[6] = "1.0.0";
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

Conductor conductor(FIRMWARE_VERSION, MAC_ADDRESS);

void setup()
{
    // Setup USB first for Serial communication
    #ifdef UNITTEST
        Serial.begin(115200);
        delay(2000);
        memoryLog("UNIT TEST");
        Serial.println(' ');
    #else
        iuUSB.setupHardware();
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
        iuBluetooth.setupHardware();
        iuWiFi.setupHardware();
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
        setUpComputerSource();
        setUpProfiles();
        // Activate a profile by default
        activateProfile(&motorStandardProfile);
        accelRMS512Total.enableOperationState();
        accelRMS512Total.setThresholds(110, 130, 150);
//        accelRMS512X.enableOperationState();
//        accelRMS512Total.setThresholds(0.5, 1.2, 1.8);
//        accelRMS512Y.enableOperationState();
//        accelRMS512Total.setThresholds(0.5, 1.2, 1.8);
//        accelRMS512Z.enableOperationState();
//        accelRMS512Total.setThresholds(0.5, 1.2, 1.8);
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
        for (uint8_t i = 0; i < SENSOR_COUNT; ++i)
        {
            SENSORS[i]->setupHardware();
            if (SENSORS[i]->isAsynchronous())
            {
                SENSORS[i]->setCallbackRate(callbackRate);
            }
        }
        if (debugMode)
        {
          memoryLog(F("=> Successfully initialized sensors"));
        }
        if (setupDebugMode)
        {
            configurator.exposeAllConfigurations();
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
        conductor.setCallback(callback);
        conductor.changeAcquisitionMode(AcquisitionMode::FEATURE);
    #endif
}

/**
 *
 * The regular calls to iuRGBLed.autoTurnOff() turns the LEDs off to save
 * power, once they've been lit long enough based on their internal timer.
 */
void loop()
{
    #ifdef UNITTEST
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
        configurator.readFromSerial(&iuUSB);
        iuRGBLed.autoTurnOff();
        configurator.readFromSerial(&iuBluetooth);
        iuRGBLed.autoTurnOff();
        configurator.readFromSerial(&iuWiFi);
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

