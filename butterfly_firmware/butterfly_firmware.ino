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


//==============================================================================
//================================= Main Code ==================================
//==============================================================================

void callback()
{
  uint32_t beg = micros();
  conductor.acquireAndSendData();
  Serial.print("Elapsed: ");
  Serial.println((micros() - beg), DEC);
}

/* ------------------------------------ begin ---------------------------------------- */

void setup()
{
  Serial.begin(115200);
  Serial.println("Start\n");
  
  conductor = IUConductor(MAC_ADDRESS);
  if (!conductor.initInterfaces())
  {
    Serial.begin(115200);
    Serial.println("Failed to initialize interfaces\n");
    while(1);                                                // hang
  }
  conductor.printMsg("Successfully initialized interfaces\n");
  conductor.printMsg("Initializing components and setting up default configurations\n");
  
  if (!conductor.initConfigurators())
  {
    conductor.printMsg("Failed to initialize configurators\n");
    while(1);                                                // hang
  }
  
  if (!conductor.initSensors())
  {
    conductor.printMsg("Failed to initialize sensors\n");
    while(1);                                                // hang
  }
  if (!conductor.featureConfigurator.doStandardSetup())
  {
    conductor.printMsg("Failed to configure features\n");
    while(1);                                                // hang
  }

  if (debugMode)
  {
    // force reset error before starting, so we can see what's next
    conductor.iuI2C->resetErrorMessage();
  }
  
  conductor.linkFeaturesToSensors();

  if (debugMode)
  {
    debugPrint(conductor.iuI2C->getErrorMessage());
    conductor.iuI2C->resetErrorMessage();
    debugPrint((char*) conductor.iuI2C->getReadError());
    conductor.iuI2C->resetReadError();
  }
  
  conductor.setCallback(callback);
  conductor.switchToMode(operationMode::run);
  conductor.switchToState(operationState::idle);

  Serial.println(conductor.iuI2C->getErrorMessage());
  Serial.println("Begin looping");
  
  if (debugMode) {while(1) {} } // hang here
  
}

void loop()
{
  //TODO Design and develop a calibration and hardware testing framework
  
  /* -------------------------- USB Connection Check ----------------------- */
  /*
  if (bitRead(USB0_OTGSTAT, 5)) {
    // When disconnected; this flag will briefly come up when USB gets disconnected => Immediately put into RUN mode with idle state
    conductor.switchToMode(operationMode::run);
    conductor.switchToState(operationState::idle);
  }
  */

  Serial.println(conductor.iuI2C->getErrorMessage());
  Serial.println(conductor.iuI2C->getReadError());
  
  conductor.processInstructionsFromBluetooth();   // Receive instructions via BLE during run mode
  conductor.processInstructionsFromI2C();         // Receive instructions to enter / exit data collection mode, plus options during data collection
  conductor.computeFeatures();                    // Conductor handles feature computation depending on operation mode
  conductor.streamData();                         // Conductor choose streaming port depending on operation mode
  conductor.checkAndUpdateState();
  conductor.checkAndUpdateMode();
}

