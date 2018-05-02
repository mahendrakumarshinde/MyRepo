/*
  Infinite Uptime BLE Module Firmware Updated 24-12-2016 v33

  TIME DOMAIN FEATURES
  NOTE: Intervals are checked based on AUDIO, not ACCEL. Read vibration energy
  code to understand the sampling mechanism.

  FREQUENCY DOMAIN FEATURES
  NOTE: Most of these variables are for accessing calculation results.
  Read any of frequency domain feature calculation flow to understand.

  TIMESTAMP HANDLING
  Timestamp are handled by the BLE componant. Current time can be sent from Bluetooth
  Hub to the board (accessible via IUBMD350.getHubDatetime()). Current datetime (accessible 
  via IUBMD350.getDatetime)is then approximated usi ng hub datetime and millis function.

*/

#include <string.h>
#include "IUConductor.h"

/*=========================================================================================== 
 * Unit test and quality assessment test 
 * Comment / Uncomment the "defines" to toggle / untoggle unit / quality test mode
 * ==========================================================================================*/

//#define UNITTEST
#ifdef UNITTEST
  #include "UnitTest/Test_IUUtilities.h"
#endif

//#define QUALITYTEST
#ifdef QUALITYTEST
  #include "QualityTest/QA_IUBMX055.h"
#endif


//========================= Module Configuration Variables ===========================
String MAC_ADDRESS = "94:54:93:0F:67:01";

// Reduce RUN frequency if needed.
const uint32_t RESTING_INTERVAL = 0;  // Inter-batch gap


//====================== Instanciate Conductor from IU library ========================

bool doOnce = true;
uint32_t interval = 500; //ms
uint32_t lastDisplay = 0;
IUConductor conductor;

//==============================================================================
//================================= Main Code ==================================
//==============================================================================

void callback()
{
  uint32_t startT = 0;
  if (callbackDebugMode)
  {
    startT = micros();
  }
  conductor.acquireAndSendData(true);
  if (callbackDebugMode)
  {
    debugPrint(micros() - startT);
  }
}

/* ------------------------------------ begin ---------------------------------------- */

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
      debugPrint(F("Start\n"));
    }
    
    if (setupDebugMode)
    {
      memoryLog("Start");
    }
    
    conductor = IUConductor(MAC_ADDRESS);
    if (setupDebugMode)
    {
      memoryLog(F("Conductor initialized"));
      debugPrint(' ');
    }
    
    conductor.setup(callback);
   
   #endif
}

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
      if(millis() - lastDisplay > interval || millis() < lastDisplay) // For millis overflowing
      {
        lastDisplay = millis();
        /* === Place your code to excute at fixed interval here ===*/
        
        /*======*/
      }
    }

    conductor.loop();
    
  #endif
}

