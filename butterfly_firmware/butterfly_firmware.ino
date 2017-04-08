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


//========================= Module Configuration Variables ===========================
String MAC_ADDRESS = "88:4A:EA:69:DF:8C";

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
  if (setupDebugMode) { memoryLog(F("Conductor initialized")); }
  
  if (!conductor.initInterfaces())
  {
    if (setupDebugMode) { debugPrint(F("Failed to initialize interfaces\n")); }
    while(1);                                                // hang
  }
  if (setupDebugMode) { memoryLog(F("Interfaces created")); }
  conductor.printMsg("Successfully initialized interfaces\n");
  conductor.printMsg("Initializing components and setting up default configurations\n");
  
  if (!conductor.initConfigurators())
  {
    conductor.printMsg("Failed to initialize configurators\n");
    while(1);                                                // hang
  }
  if (setupDebugMode) { memoryLog(F("Configurators created")); }
  
  if (!conductor.initSensors())
  {
    conductor.printMsg("Failed to initialize sensors\n");
    while(1);                                                // hang
  }
  if (setupDebugMode) { memoryLog(F("Sensors created")); }
  
  if (!conductor.featureConfigurator.doStandardSetup())
  {
    conductor.printMsg("Failed to configure features\n");
    while(1);                                                // hang
  }
  if (setupDebugMode) { memoryLog(F("Features created")); }
  
  if (!conductor.linkFeaturesToSensors())
  {
    conductor.printMsg("Failed to link feature sources to sensors\n");
    while(1);                                                // hang
  }
  if (setupDebugMode)
  {
    debugPrint(F("Feature sources successfully linked to sensors\n"));
    conductor.sensorConfigurator.exposeSensorsAndReceivers();
    conductor.featureConfigurator.exposeFeaturesAndReceivers();
    conductor.featureConfigurator.exposeFeatureStates();
    
    debugPrint(F("I2C status: "), false);
    debugPrint(conductor.iuI2C->getErrorMessage());
    debugPrint(F("\nBegin run at (ms): "), false);
    debugPrint(millis());
    debugPrint(' ');
  }
  //Serial.flush();
  conductor.setCallback(callback);
  conductor.switchToMode(operationMode::run);
  conductor.switchToState(operationState::idle);
}

void loop()
{
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
  conductor.processInstructionsFromBluetooth();   // Receive instructions via BLE during run mode
  conductor.processInstructionsFromI2C();         // Receive instructions to enter / exit data collection mode, plus options during data collection
  conductor.computeFeatures();                    // Conductor handles feature computation depending on operation mode
  conductor.streamData();                         // Stream data over BLE during run mode
  conductor.checkAndUpdateState();
  conductor.checkAndUpdateMode();    
}

