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
#include "IUButterfly.h"

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

IUConductor conductor;
bool doOnce = true;
uint32_t interval = 500; //ms
uint32_t lastDisplay = 0;


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
  conductor.acquireAndSendData();
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
  
    if (debugMode)
    {
      Serial.begin(115200);
      debugPrint(F("Start\n"));
      delay(2000);
    }
  
    delay(2000);
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
    
    if (!conductor.initInterfaces())
    {
      if (setupDebugMode) { debugPrint(F("Failed to initialize interfaces\n")); }
      while(1);                                                // hang
    }
    if (setupDebugMode)
    {
      memoryLog(F("Interfaces created"));
      debugPrint(' ');
      conductor.iuBluetooth->exposeInfo();
      debugPrint(' ');
    }
    
    conductor.printMsg("Successfully initialized interfaces\n");
    conductor.printMsg("Initializing components and setting up default configurations...");
    
    if (!conductor.initConfigurators())
    {
      conductor.printMsg("Failed to initialize configurators\n");
      while(1);                                                // hang
    }
    if (setupDebugMode)
    {
      memoryLog(F("Configurators created"));
      debugPrint(' ');
    }
    
    if (!conductor.initSensors())
    {
      conductor.printMsg("Failed to initialize sensors\n");
      while(1);                                                // hang
    }
    if (setupDebugMode)
    {
        memoryLog(F("Sensors created"));
        debugPrint(' ');
    }
    
    if (!conductor.featureConfigurator.doStandardSetup())
    {
      conductor.printMsg("Failed to configure features\n");
      while(1);                                                // hang
    }
    if (setupDebugMode)
    {
      memoryLog(F("Features created"));
      debugPrint(' ');
    }
    
    if (!conductor.linkFeaturesToSensors())
    {
      conductor.printMsg("Failed to link feature sources to sensors\n");
      while(1);                                                // hang
    }
    if (setupDebugMode)
    {
      memoryLog(F("Feature sources successfully linked to sensors"));
      debugPrint(' ');
    }
    
    conductor.printMsg("Done setting up components and configurations\n");
    
    if (setupDebugMode)
    {
      conductor.sensorConfigurator.exposeSensorsAndReceivers();
      conductor.featureConfigurator.exposeFeaturesAndReceivers();
      conductor.featureConfigurator.exposeFeatureStates();
      
      debugPrint(F("I2C status: "), false);
      debugPrint(conductor.iuI2C->getErrorMessage());
      debugPrint(F("\nFinished setup at (ms): "), false);
      debugPrint(millis());
      debugPrint(' ');
    }
    
    conductor.setCallback(callback);
    conductor.switchToMode(operationMode::run);
    conductor.switchToState(operationState::idle);
   
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
    conductor.processInstructionsFromBluetooth(MAC_ADDRESS);  // Receive instructions via BLE during run mode
    conductor.processInstructionsFromI2C();                   // Receive instructions to enter / exit data collection mode, plus options during data collection
    conductor.computeFeatures();                              // Conductor handles feature computation depending on operation mode
    conductor.streamData();                                   // Stream data over BLE during run mode
    conductor.checkAndUpdateState();
    conductor.checkAndUpdateMode();
    
  #endif
}

