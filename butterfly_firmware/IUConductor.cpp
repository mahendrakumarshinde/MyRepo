#include "IUConductor.h"

IUConductor::IUConductor()
{
  iuI2C = NULL;
  iuBluetooth = NULL;
  iuWifi = NULL;
}

IUConductor::IUConductor(String macAddress) :
  m_macAddress(macAddress),
  m_bluesleeplimit(60000),
  m_autoSleepEnabled(false),
  m_usageMode(usageMode::operation),
  m_opMode(operationMode::sleep),
  m_opState(operationState::idle),
  m_inDataAcquistion(false),
  m_lastSentTime(0),
  m_lastSynchroTime(0)
{
  iuI2C = NULL;
  iuBluetooth = NULL;
  iuWifi = NULL;
  m_dataSendPeriod = defaultDataSendPeriod;
  m_refDatetime = defaultTimestamp;
  setClockRate(defaultClockRate);
}

IUConductor::~IUConductor()
{
  if(iuI2C)
  {
    delete iuI2C; iuI2C = NULL;
  }
  if(iuBluetooth)
  {
    delete iuBluetooth; iuBluetooth = NULL;
  }
  if(iuWifi)
  {
    delete iuWifi; iuWifi = NULL;
  }
}

/**
 * Set the master clock rate (I2S clock is used as master clock by the conductor)
 * @return true if the clock rate was correctly set, else false
 */
bool IUConductor::setClockRate(uint16_t clockRate)
{
  m_clockRate = clockRate;
  if (sensorConfigurator.iuI2S)
  {
    // if sensors have already been initialized, set the clock rate of I2S,
    // otherwise I2S clock rate will be set when it is initialized (see initSensors)
    return sensorConfigurator.iuI2S->setClockRate(clockRate);
  }
  return false;
}

void IUConductor::setDataSendPeriod(uint16_t dataSendPeriod)
{
  m_dataSendPeriod = dataSendPeriod;
  if (!iuI2C->isSilent())
  {
    iuI2C->port->print("Data send period set to : ");
    iuI2C->port->println(m_dataSendPeriod);
  }
}

/**
 * Check if it is time to send data based on m_dataSendPeriod
 * NB: Data streaming is always disabled when in "sleep" operation mode.
 */
bool IUConductor::isDataSendTime()
{
  //Timer to send data regularly//
  uint32_t current = millis();
  if (current - m_lastSentTime >= m_dataSendPeriod)
  {
    m_lastSentTime = current;
    return true;
  }
  return false;
}

/**
 * 
 */
double IUConductor::getDatetime()
{
  uint32_t now = millis();
  return m_refDatetime + (double) (now - m_lastSynchroTime) / 1000.;
}

/**
 * Create and activate interfaces
 * NB: Initialization order must be: Interfaces, then Configurators, then Sensors
 * @return true if the interfaces were correctly initialized, else false
 */
bool IUConductor::initInterfaces()
{
  iuI2C = new IUI2C();
  if (!iuI2C)
  {
    iuI2C = NULL;
    return false;
  }
  iuI2C->activate();
  iuI2C->scanDevices(); // Find components
  iuI2C->resetErrorMessage();
  
  iuBluetooth = new IUBMD350(iuI2C);
  if (!iuBluetooth)
  {
    iuBluetooth = NULL;
    return false;
  }
  iuBluetooth->activate();
  
  iuWifi = new IUESP8285(iuI2C);
  if (!iuWifi)
  {
    iuWifi = NULL;
    return false;
  }
  iuWifi->activate();
  
  return true;
}

/**
 * Create and activate sensors
 * NB: Initialization order must be: Interfaces, then Configurators, then Sensors
 * @return true if the Configurators were correctly initialized
 */
bool IUConductor::initConfigurators()
{
  sensorConfigurator = IUSensorConfigurator(iuI2C);
  featureConfigurator = IUFeatureConfigurator();
  return true;
}

/**
 * Create and activate sensors
 * NB: Initialization order must be: Interfaces, then Configurators, then Sensors
 * @return true if the sensors were correctly initialized, else false
 */
bool IUConductor::initSensors()
{
  bool success = sensorConfigurator.createAllSensorsWithDefaultConfig();
  if (!success)
  {
    return false;
  }
  sensorConfigurator.iuI2S->setClockRate(m_clockRate);
  sensorConfigurator.wakeUpSensors();
  return true;
}

/**
 * Link all features to all sensors
 */
bool IUConductor::linkFeaturesToSensors()
{
  return featureConfigurator.registerAllFeaturesInSensors(sensorConfigurator.getSensors(),
                                                          sensorConfigurator.sensorCount);
}

/**
 * Switch to a new Usage Mode
 * usage mode typically describe how the 
 */
void IUConductor::switchToUsage(usageMode mode)
{
  if(m_usageMode == mode)
  {
    return; // Nothing to do
  }
  m_usageMode = mode;
  String msg;
  if (m_usageMode == usageMode::configuration)
  {
    switchToMode(operationMode::sleep);
    msg = "configuration";
  }
  else if (m_usageMode == usageMode::calibration)
  {
    switchToMode(operationMode::sleep);
    /*
    Serial.println("reset sensor receivers");
    Serial.flush();
    sensorConfigurator.resetAllReceivers();
    Serial.println("reset feature receivers");
    Serial.flush();
    featureConfigurator.resetAllReceivers();
    Serial.println("done");
    Serial.flush();
    featureConfigurator.doCalibrationSetup();
    */
    setDataSendPeriod(calibrationDataSendPeriod);
    featureConfigurator.setCalibrationStreaming();
    switchToMode(operationMode::run);
    sensorConfigurator.iuRGBLed->changeColor(IURGBLed::CYAN_DATA); // override run mode color with calibration color
    iuI2C->silence();
    msg = "calibration";
  }
  else if (m_usageMode == usageMode::operation)
  {
    switchToMode(operationMode::sleep);
    /*
    Serial.println("reset sensor receivers");
    Serial.flush();
    sensorConfigurator.resetAllReceivers();
    Serial.println("reset feature receivers");
    Serial.flush();
    featureConfigurator.resetAllReceivers();
    Serial.println("done");
    Serial.flush();
    featureConfigurator.doStandardSetup();
    */
    setDataSendPeriod(defaultDataSendPeriod);
    featureConfigurator.setStandardStreaming();
    switchToMode(operationMode::run);
    iuI2C->unsilence();
    msg = "operation";
  }
  if (loopDebugMode) { debugPrint("\nEntering " + msg + " usage\n"); }
}

/**
 * Switch to a new operation mode
 * 1. When switching to "run", "record" or "data collection" mode, start data
 *    acquisition with beginDataAcquisition().
 * 2. When switching to any other modes, end data acquisition
 *    with endDataAcquisition().
 */
void IUConductor::switchToMode(operationMode mode)
{
  if (m_opMode == mode)
  {
    return; // do nothing if already in desired mode
  }
  m_opMode = mode;
  String msg;
  if (m_opMode == operationMode::charging)
  {
    endDataAcquisition();
    msg = "charging";
  }
  else if (m_opMode == operationMode::run)
  {
    // TODO Rebuild
    /*
    sensorConfigurator.iuRGBLed->updateFromI2C();            // Check for LED color update
    sensorConfigurator.iuBMX055->updateAccelRangeFromI2C();  // check for accel range update
    bool accUpdate = iuBMX055->updateSamplingRateFromI2C();  // check for accel sampling rate update
    bool audioUpdate = iuI2S->updateSamplingRateFromI2C();   // check for accel sampling rate update
    if (accUpdate || audioUpdate) //Accelerometer or Audio sampling rate changed
    {
      ACCEL_COUNTER_TARGET = iuI2S.getTargetSample() / iuBMX055.getTargetSample();
      accel_counter = 0;
      subsample_counter = 0;
    }
    */
    sensorConfigurator.iuRGBLed->changeColor(IURGBLed::BLUE_NOOP);
    resetDataAcquisition();
    msg = "run";
  }
  else if (m_opMode == operationMode::dataCollection)
  {
    sensorConfigurator.iuRGBLed->changeColor(IURGBLed::CYAN_DATA);
    resetDataAcquisition();
    msg = "data collection";
  }
  else if (m_opMode == operationMode::record)
  {
    resetDataAcquisition();
    msg = "record";
  }
  else if (m_opMode == operationMode::sleep)
  {
    m_sleepStartTime = millis();
    endDataAcquisition();
    sensorConfigurator.iuRGBLed->changeColor(IURGBLed::SLEEP_MODE);
    msg = "sleep";
  }
  if (loopDebugMode) { debugPrint("\nEntering " + msg + " mode\n"); }
}

/**
 * Switch to a new state and update the Led color accordingly
 * NB:Do not update state if:
 * - in sleep mode
 * - already in desired state
 * Also updates the idleStartTime timer
 */
void IUConductor::switchToState(operationState state)
{
  if (m_opState == state && m_opMode == operationMode::sleep)
  {
    return;
  }
  m_opState = state;
  String msg = "";
  if (m_opState == operationState::idle)
  {
    m_idleStartTime = millis(); // used for auto-sleep functionnality
    sensorConfigurator.iuRGBLed->changeColor(IURGBLed::BLUE_NOOP);
    msg = "idle";
  }
  else if (m_opState == operationState::normalCutting)
  {
    sensorConfigurator.iuRGBLed->changeColor(IURGBLed::GREEN_OK);
    msg = "normalCutting";
  }
  else if (m_opState == operationState::warningCutting)
  {
    sensorConfigurator.iuRGBLed->changeColor(IURGBLed::ORANGE_WARNING);
    msg = "warningCutting";
  }
  else if (m_opState == operationState::badCutting)
  {
    sensorConfigurator.iuRGBLed->changeColor(IURGBLed::RED_BAD);
    msg = "badCutting";
  }
  if (loopDebugMode) { debugPrint("\nEntering " + msg + " state\n"); }
}

/**
 * Check if the operation mode needs to be updated: if yes, update it
 * NB: To update the mode, this function calls method switchToMode.
 * @return true if the mode was updated, else false.
 */
bool IUConductor::checkAndUpdateMode()
{
  if (m_autoSleepEnabled)
  {
    if (m_opMode != operationMode::sleep && m_startSleepTimer < millis() - m_idleStartTime)
    {
      switchToMode(operationMode::sleep); // go to sleep
      return true;
    }
    else if (m_opMode == operationMode::sleep && m_opState == operationState::idle && m_endSleepTimer < millis() - m_sleepStartTime)
    {
      switchToMode(operationMode::run); // exit sleep mode into run mode
      return true;
    }
  }
  return false;
}

/**
 * Check if the operation state needs to be updated: if yes, update it
 * NB1: The conductor operation state is based on the highest state from features.
 * NB2: To update the state, this function calls method switchToState.
 * NB3: operation state update is disabled when not in "run" operation mode.
 * @return true if the state was updated, else false.
 */
bool IUConductor::checkAndUpdateState()
{
  if (m_opMode != operationMode::run)
  {
    return false;
  }
  operationState newState = featureConfigurator.getOperationStateFromFeatures();
  if (m_opState != newState)
  {
    switchToState(newState);
    return true;
  }
  return false;
}

/**
 * Data acquisition function to be used as callback
 * We use the I2S callback to do both audio and acceleration data collection
 * @return true if new data acquisition were made, else false
 * Method benchmarked for all sensors and 6 features at 10microseconds.
 */
bool IUConductor::acquireAndSendData()
{
  if (!m_inDataAcquistion)
  {
    return false;
  }
  bool newData = false;
  if (iuI2C->getErrorMessage().equals("ALL_OK")) // Acquire data only if all ok
  {
    if (m_opMode == operationMode::run)
    {
      sensorConfigurator.acquireDataAndSendToReceivers();
      newData = true;
    }
    if (m_opMode == operationMode::dataCollection)
    {
      sensorConfigurator.acquireDataAndDumpThroughI2C();
      processInstructionsFromI2C(); // In data collection mode, need to process I2C data
      newData = true;
    }
    if (m_opMode == operationMode::record)
    {
      sensorConfigurator.acquireAndStoreData();
      newData = true;
    }
  }
  else
  {
    if (debugMode)
    {
      debugPrint("I2C error: ", false);
      debugPrint(iuI2C->getErrorMessage());
    }
    iuI2C->resetErrorMessage();
    sensorConfigurator.iuI2S->readData(false);; // Empty I2S buffer to continue
  }
  return newData;
}

/**
 * Compute each feature that is ready to be computed
 * NB: Does nothing when not in "run" operation mode
 */
void IUConductor::computeFeatures()
{
  if (m_opMode == operationMode::run)
  {
    featureConfigurator.computeAndSendToReceivers();
  }
}

/**
 * Send feature data through an interface (serial, bluetooth, wifi)
 * @return true if data was sent, else false
 */
bool IUConductor::streamData(HardwareSerial *port, bool newLine)
{
  if (m_opMode != operationMode::run)
  {
    return false;
  }
  if (isDataSendTime())
  {
    port->print(m_macAddress);                                                 // MAC Address
    port->print(",0"); port->print((uint8_t) m_opState); port->print(",");     // Operation State
    port->print(sensorConfigurator.iuBattery->getBatteryStatus());             // % Battery charge
    featureConfigurator.streamFeatures(port);                                  // Each feature value
    port->print(","); port->print(getDatetime()); port->print(";");            // Datetime
    if (newLine)
    {
      port->println("");
    }
    port->flush();                                                             // End
    return true;
  }
  return false;
}

/**
 * Start data acquisition by beginning I2S data acquisition
 * NB: Other sensor data acquisition depends on I2S drumbeat
 */
bool IUConductor::beginDataAcquisition()
{
  if (m_inDataAcquistion)
  {
    return true; // Already in data acquisition
  }
  // m_inDataAcquistion should be set to true BEFORE triggering the data acquisition,
  // otherwise I2S buffer won't be emptied in time
  m_inDataAcquistion = true;
  if (!sensorConfigurator.iuI2S->triggerDataAcquisition(m_callback))
  {
    m_inDataAcquistion = false;
  }
  return m_inDataAcquistion;
}

/**
 * End data acquisition by disabling I2S data acquisition
 * NB: Other sensor data acquisition depends on I2S drumbeat
 */
void IUConductor::endDataAcquisition()
{
  if (!m_inDataAcquistion)
  {
    return; // nothing to do
  }
  sensorConfigurator.iuI2S->endDataAcquisition();
  m_inDataAcquistion = false;
  // Delay to make sure that the I2S callback function is called for the last time
  delay(50);
}

/**
 * End and restart the data acquisition
 * @return the output of beginDataAcquisition (if it was successful or not)
 */
bool IUConductor::resetDataAcquisition()
{
  endDataAcquisition();
  featureConfigurator.resetFeaturesCounters();
  delay(500);
  return beginDataAcquisition();
}

/**
 * Process the instructions sent over I2C to adjust the configuration
 */
void IUConductor::processInstructionsFromI2C()
{
  iuI2C->updateBuffer();
  if (iuI2C->getBuffer().length() > 0)
  {
    if(setupDebugMode) { debugPrint("I2C input is:"); }
    if(setupDebugMode) { debugPrint(iuI2C->getBuffer()); }
    if(m_usageMode == usageMode::operation && iuI2C->checkIfStartCalibration())
    {
      switchToUsage(usageMode::calibration);
    }
    else if (m_usageMode == usageMode::calibration && iuI2C->checkIfEndCalibration())
    {
      switchToUsage(usageMode::operation);
    }
    if (m_opMode == operationMode::run && iuI2C->checkIfStartCollection())
    {
      switchToMode(operationMode::dataCollection);
    }
    else if (m_opMode == operationMode::dataCollection && iuI2C->checkIfEndCollection())
    {
      switchToMode(operationMode::run);
      sensorConfigurator.iuBMX055->resetAccelScale();
    }
    iuI2C->resetBuffer();    // Clear wire buffer
  }
}

/**
 * Process the instructions sent over Bluetooth to adjust the configuration
 * Concretely:
 * 1. read and fill the bluetooth receiving buffer (with readToBuffer). This
 *    include a check on data reception timeout.
 * 2. scan the content of the buffer and apply changes where needed
 */
void IUConductor::processInstructionsFromBluetooth()
{
  if (m_opMode != operationMode::run)
  {
    return;
  }
  char *bleBuffer = NULL;
  while (iuBluetooth->readToBuffer())
  {
    bleBuffer = iuBluetooth->getBuffer();
    if (!bleBuffer)
    {
      break;
    }
    if(loopDebugMode)
    {
      debugPrint(F("BLE input received:"), false);
      debugPrint(bleBuffer);
    }
    switch(bleBuffer[0])
    {
      case '0': // Set Thresholds
        if (bleBuffer[4] == '-' && bleBuffer[9] == '-' && bleBuffer[14] == '-')
        {
          int featureIdx = 0, newThres = 0, newThres2 = 0, newThres3 = 0;
          sscanf(bleBuffer, "%d-%d-%d-%d", &featureIdx, &newThres, &newThres2, &newThres3);
          IUFeature *feat = featureConfigurator.getFeatureById(featureIdx + 1);
          if (featureIdx == 1 || featureIdx == 2 || featureIdx == 3)
          {
            feat->setThresholds((float)newThres / 100., (float)newThres2 / 100., (float)newThres3 / 100.);
          }
          else
          {
            feat->setThresholds((float)newThres, (float)newThres2, (float)newThres3);
          }
          if (loopDebugMode)
          {
            debugPrint(feat->getName(), false); debugPrint(':', false);
            debugPrint(feat->getThreshold(0), false); debugPrint(' - ', false);
            debugPrint(feat->getThreshold(1), false); debugPrint(' - ', false);
            debugPrint(feat->getThreshold(2));
          }
        }
        break;
      case '1': // Receive the timestamp data from the bluetooth hub
        if (bleBuffer[1] == ':' && bleBuffer[12] == '.')
        {
          int flag(0), timestamp(0), microsec(0);
          sscanf(bleBuffer, "%d:%d.%d", &flag, &timestamp, &microsec);
          m_refDatetime = double(timestamp) + double(microsec) / double(1000000);
          m_lastSynchroTime = millis();
          if (loopDebugMode)
          {
            debugPrint("Time sync: ", false);
            debugPrint(getDatetime());
          }
        }
        break;

      case '2': // Bluetooth parameter setting
        if (bleBuffer[1] == ':' && bleBuffer[7] == '-' && bleBuffer[13] == '-')
        {
          int dataRecTimeout(0), paramtag(0), dataSendPeriod(0);
          sscanf(bleBuffer, "%d:%d-%d-%d", &paramtag, &dataSendPeriod, m_startSleepTimer, &dataRecTimeout);
          m_endSleepTimer = m_startSleepTimer;
          setDataSendPeriod((uint16_t) dataSendPeriod);
          iuBluetooth->setDataReceptionTimeout((uint16_t) dataRecTimeout);
        }
        break;

      case '3': // Record button pressed - go into record mode to record FFTs
        if (bleBuffer[7] == '0' && bleBuffer[9] == '0' && bleBuffer[11] == '0' && bleBuffer[13] == '0' && bleBuffer[15] == '0' && bleBuffer[17] == '0')
        {
          if (loopDebugMode) { debugPrint("Record mode"); }
          iuBluetooth->port->print("REC,");
          iuBluetooth->port->print(m_macAddress);
          IUFeature *feat = NULL;
          feat = featureConfigurator.getFeatureByName("CX3");
          if (feat)
          {
            iuBluetooth->port->print(",X");
            feat->streamSourceData(iuBluetooth->port, 0, q4_11ToFloat);
          }
          feat = featureConfigurator.getFeatureByName("CY3");
          if (feat)
          {
            iuBluetooth->port->print(",Y");
            feat->streamSourceData(iuBluetooth->port, 0, q4_11ToFloat);
          }
          feat = featureConfigurator.getFeatureByName("CZ3");
          if (feat)
          {
            iuBluetooth->port->print(",Z");
            feat->streamSourceData(iuBluetooth->port, 0, q4_11ToFloat);
          }
          iuBluetooth->port->print(";");
          iuBluetooth->port->flush();
        }
        break;
      case '4': // Stop button pressed - go out of record mode back into RUN mode
        if (bleBuffer[7] == '0' && bleBuffer[9] == '0' && bleBuffer[11] == '0' && bleBuffer[13] == '0' && bleBuffer[15] == '0' && bleBuffer[17] == '0')
        {
          iuI2C->port->print("Stop recording and sending FFTs");
        }
        break;

      case '5':
        if (bleBuffer[7] == '0' && bleBuffer[9] == '0' && bleBuffer[11] == '0' && bleBuffer[13] == '0' && bleBuffer[15] == '0' && bleBuffer[17] == '0')
        {
          iuBluetooth->port->print("HB,");
          iuBluetooth->port->print(m_macAddress);
          iuBluetooth->port->print(",");
          iuBluetooth->port->print(iuI2C->getErrorMessage());
          iuBluetooth->port->print(";");
          iuBluetooth->port->flush();
        }
        break;

      case '6':
        //TODO: Reimplement
        if (bleBuffer[7] == ':' && bleBuffer[9] == '.' && bleBuffer[11] == '.' && bleBuffer[13] == '.' && bleBuffer[15] == '.' && bleBuffer[17] == '.')
        {
          int parametertag(0);
          int fcheck[6] = {0, 0, 0, 0, 0, 0};
          sscanf(bleBuffer, "%d:%d.%d.%d.%d.%d.%d", &parametertag, &fcheck[0], &fcheck[1], &fcheck[2], &fcheck[3], &fcheck[4], &fcheck[5]);
          for (uint8_t i = 0; i < 6; i++)
          {
            IUFeature *feat = featureConfigurator.getFeatureById(i + 1);
            feat->setFeatureCheck((bool) fcheck[i]);
          }
        }
        break;
    }
  }
}

void IUConductor::processInstructionsFromWifi()
{

}

void IUConductor::setup(void (*callback)())
{
  if (!initInterfaces())
  {
    if (setupDebugMode) { debugPrint(F("Failed to initialize interfaces\n")); }
    while(1);                                                // hang
  }
  if (setupDebugMode)
  {
    memoryLog(F("Interfaces created"));
    debugPrint(' ');
    iuBluetooth->exposeInfo();
    debugPrint(' ');
  }
  
  iuI2C->port->println("Successfully initialized interfaces\n");
  iuI2C->port->println("Initializing components and setting up default configurations...");
  
  if (!initConfigurators())
  {
    iuI2C->port->println("Failed to initialize configurators\n");
    while(1);                                                // hang
  }
  if (setupDebugMode)
  {
    memoryLog(F("Configurators created"));
    debugPrint(' ');
  }
  
  if (!initSensors())
  {
    iuI2C->port->println("Failed to initialize sensors\n");
    while(1);                                                // hang
  }
  if (setupDebugMode)
  {
      memoryLog(F("Sensors created"));
      debugPrint(' ');
  }
  
  if (!featureConfigurator.doStandardSetup())
  {
    iuI2C->port->println("Failed to configure features\n");
    while(1);                                                // hang
  }
  if (setupDebugMode)
  {
    memoryLog(F("Features created"));
    debugPrint(' ');
  }
  
  if (!linkFeaturesToSensors())
  {
    iuI2C->port->println("Failed to link feature sources to sensors\n");
    while(1);                                                // hang
  }
  if (setupDebugMode)
  {
    memoryLog(F("Feature sources successfully linked to sensors"));
    debugPrint(' ');
  }
  
  iuI2C->port->println("Done setting up components and configurations\n");
  
  if (setupDebugMode)
  {
    sensorConfigurator.exposeSensorsAndReceivers();
    featureConfigurator.exposeFeaturesAndReceivers();
    featureConfigurator.exposeFeatureStates();
    
    debugPrint(F("I2C status: "), false);
    debugPrint(iuI2C->getErrorMessage());
    debugPrint(F("\nFinished setup at (ms): "), false);
    debugPrint(millis());
    debugPrint(' ');
  }
  
  m_callback = callback;
  switchToMode(operationMode::run);
  switchToState(operationState::idle);
}

void IUConductor::loop()
{
  if (m_usageMode == usageMode::operation)
  {
    processInstructionsFromBluetooth();  // Receive instructions via BLE
    processInstructionsFromI2C();        // Receive instructions to enter / exit modes, plus options during data collection
    computeFeatures();                   // Feature computation depending on operation mode
    streamData(iuBluetooth->port);       // Stream data over BLE
    checkAndUpdateState();
    checkAndUpdateMode();
  }
  else if (m_usageMode == usageMode::calibration)
  {
    processInstructionsFromI2C();        // Receive instructions to enter / exit modes
    computeFeatures();                   // Feature computation depending on operation mode
    streamData(iuI2C->port);             // Stream data over I2C during calibration
  }
  else if (m_usageMode == usageMode::configuration)
  {
    processInstructionsFromBluetooth();  // Receive instructions via BLE
    processInstructionsFromI2C();        // Receive instructions to enter / exit modes
  }
}


/* ====================== Diagnostic Functions, only active when setupDebugMode = true ====================== */

