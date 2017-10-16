/*
  Infinite Uptime BLE Module Firmware

  Update 2017-10-10
*/

#include "Conductor.h"
#include "Configurator.h"

/* =============================================================================
    Unit test and quality assessment test
============================================================================= */

/* Comment / Uncomment the "define" lines to toggle / untoggle unit or quality
test mode */

//#define UNITTEST
#ifdef UNITTEST
  #include "UnitTest/Test_IUUtilities.h"
#endif

//#define QUALITYTEST
#ifdef QUALITYTEST
  #include "QualityTest/QA_IUBMX055.h"
#endif


/* =============================================================================
    Identification Constants
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
    #ifdef UNITTEST
        Serial.begin(115200);
        delay(4000);
    #else
        if (setupDebugMode)
        {
          Serial.begin(115200);
          delay(2000);
          memoryLog("Start");
        }
        conductor.setup(callback);
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

