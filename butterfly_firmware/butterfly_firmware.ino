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


//====================== Module Configuration Variables ========================
String MAC_ADDRESS = "88:4A:EA:69:DF:8C";

// Reduce RUN frequency if needed.
const uint32_t RESTING_INTERVAL = 0;  // Inter-batch gap


//====================== Instanciate Conductor from IU library ========================

IUConductor conductor;
bool firstLoop = true;


//==============================================================================
//================================= Main Code ==================================
//==============================================================================

void callback()
{
  conductor.acquireAndSendData();
}

/* ------------------------------------ begin ---------------------------------------- */

void setup()
{
  if (debugMode)
  {
    Serial.begin(115200);
    debugPrint(F("Start\n"));
  }

  delay(4000);
  if (debugMode)
  {
    debugPrint(F("Start - Available Memory: "), false);
    debugPrint(freeMemory());
  }
  
  conductor = IUConductor(MAC_ADDRESS);
  if (setupDebugMode)
  {
    debugPrint(F("Conductor - Available Memory: "), false);
    debugPrint(freeMemory());
  }
  
  if (!conductor.initInterfaces())
  {
    if (setupDebugMode) { debugPrint(F("Failed to initialize interfaces\n")); }
    while(1);                                                // hang
  }
  if (setupDebugMode)
  {
    debugPrint(F("Interfaces - Available Memory: "), false);
    debugPrint(freeMemory());
  }
  conductor.printMsg("Successfully initialized interfaces\n");
  conductor.printMsg("Initializing components and setting up default configurations\n");
  
  if (!conductor.initConfigurators())
  {
    conductor.printMsg("Failed to initialize configurators\n");
    while(1);                                                // hang
  }
  if (setupDebugMode)
  {
    debugPrint(F("configurators - Available Memory: "), false);
    debugPrint(freeMemory());
  }
  
  if (!conductor.initSensors())
  {
    conductor.printMsg("Failed to initialize sensors\n");
    while(1);                                                // hang
  }
  if (setupDebugMode)
  {
    debugPrint(F("Sensors - Available Memory: "), false);
    debugPrint(freeMemory());
  }
  
  if (!conductor.featureConfigurator.doStandardSetup())
  {
    conductor.printMsg("Failed to configure features\n");
    while(1);                                                // hang
  }
  if (setupDebugMode)
  {
    debugPrint(F("Features - Available Memory: "), false);
    debugPrint(freeMemory());
  }
  
  conductor.linkFeaturesToSensors();

  if (setupDebugMode)
  {
    debugPrint(conductor.iuI2C->getErrorMessage());
    conductor.iuI2C->resetErrorMessage();
    debugPrint(F((char*) conductor.iuI2C->getReadError()));
    conductor.iuI2C->resetReadError();
    conductor.sensorConfigurator.exposeSensorsAndReceivers();
    conductor.featureConfigurator.exposeFeaturesAndReceivers();
    conductor.featureConfigurator.exposeFeatureStates();
  }
  
  conductor.setCallback(callback);
  conductor.switchToMode(operationMode::run);
  conductor.switchToState(operationState::idle);

  if (debugMode)
  {
    debugPrint(conductor.iuI2C->getErrorMessage());
    debugPrint(F("Begin run at (ms): "), false);
    debugPrint(millis());
  }
}

void loop()
{
  //TODO Design and develop a calibration and hardware testing framework
  if (firstLoop)
  {
    if (loopDebugMode)
    {
      debugPrint(F("Loop - Available Memory: "), false);
      debugPrint(freeMemory());
    }
    firstLoop = false;
  }
  
  /* -------------------------- USB Connection Check ----------------------- */
  /*
  if (bitRead(USB0_OTGSTAT, 5)) {
    // When disconnected; this flag will briefly come up when USB gets disconnected => Immediately put into RUN mode with idle state
    conductor.switchToMode(operationMode::run);
    conductor.switchToState(operationState::idle);
  }
  */

  conductor.processInstructionsFromBluetooth();   // Receive instructions via BLE during run mode
  conductor.processInstructionsFromI2C();         // Receive instructions to enter / exit data collection mode, plus options during data collection
  conductor.computeFeatures();                    // Conductor handles feature computation depending on operation mode
  conductor.streamData();                         // Conductor choose streaming port depending on operation mode
  conductor.checkAndUpdateState();
  conductor.checkAndUpdateMode();
  
}

