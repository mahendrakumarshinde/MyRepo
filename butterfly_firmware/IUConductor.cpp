#include "IUConductor.h"


/* ============================ Constructors, destructor, getters and setters ============================ */

IUConductor::IUConductor()
{
  iuI2C = NULL;
  iuBluetooth = NULL;
  iuWifi = NULL;
  iuRGBLed = NULL;
}

IUConductor::IUConductor(String macAddress) :
  m_macAddress(macAddress),
  m_bluesleeplimit(60000),
  m_autoSleepEnabled(false),
  m_usageMode(usageMode::OPERATION),
  m_acquisitionMode(acquisitionMode::NONE),
  m_operationState(operationState::IDLE),
  m_inDataAcquistion(false),
  m_lastSentTime(0),
  m_lastSynchroTime(0)
{
  iuI2C = NULL;
  iuBluetooth = NULL;
  iuWifi = NULL;
  iuRGBLed = NULL;
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
  if(iuRGBLed)
  {
    delete iuRGBLed; iuRGBLed = NULL;
  }
}


/* ============================ Time management methods ============================ */


/**
 * Set the master clock rate (I2S clock is used as master clock by the conductor)
 * @return true if the clock rate was correctly set, else false
 */
bool IUConductor::setClockRate(uint16_t clockRate)
{
  m_clockRate = clockRate;
  if (!sensorConfigurator.iuI2S)
  {
    // if I2S has not been initialized initialized yet, return.
    // I2S clock rate will be set when it is initialized (see initSensors)
    return false;
  }
  if (!sensorConfigurator.iuI2S->setClockRate(clockRate))
  {
    return false;
  }
  int callbackRate = sensorConfigurator.iuI2S->getCallbackRate();
  // Let the other sensors know the new callback rate
  sensorConfigurator.iuAccelerometer->setCallbackRate(callbackRate);
  sensorConfigurator.iuBMP280->setCallbackRate(callbackRate);
  sensorConfigurator.iuBattery->setCallbackRate(callbackRate);
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


/* ============================ Usage, acquisition and power modes mgmt methods methods ============================ */

void IUConductor::changePowerPreset(powerPreset::option powerPreset)
{
  m_powerPreset = powerPreset;
  if (m_powerPreset == powerPreset::ACTIVE)
  {
    /*
      ACTIVE  = 0,
      SUSPEND = 2,
      LOW1    = 3,
      LOW2    = 4,
      COUNT   = 5,
     */
  }
}


/**
 * Switch to a new Usage Mode
 * usage mode typically describe how the
 */
void IUConductor::changeUsageMode(usageMode::option mode)
{
  m_usageMode = mode;
  String msg;
  if (m_usageMode == usageMode::CONFIGURATION)
  {
    changeAcquisitionMode(acquisitionMode::NONE);
    msg = "configuration";
  }
  else if (m_usageMode == usageMode::CALIBRATION)
  {
    iuI2C->silence();
    setDataSendPeriod(shortestDataSendPeriod);
    featureConfigurator.setCalibrationStreaming();
    iuRGBLed->changeColor(IURGBLed::CYAN);
    msg = "calibration";
  }
  else if (m_usageMode == usageMode::OPERATION)
  {
    setDataSendPeriod(defaultDataSendPeriod);
    featureConfigurator.setStandardStreaming();
    iuRGBLed->changeColor(IURGBLed::BLUE);
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
void IUConductor::changeAcquisitionMode(acquisitionMode::option mode)
{
  m_acquisitionMode = mode;
  String msg;
  if (m_acquisitionMode == acquisitionMode::FEATURE)
  {
    iuRGBLed->changeColor(IURGBLed::BLUE);
    iuI2C->unsilence();
    beginDataAcquisition();
    //resetDataAcquisition();
    msg = "run";
  }
  else if (m_acquisitionMode == acquisitionMode::WIRED)
  {
    iuI2C->silence();
    iuRGBLed->changeColor(IURGBLed::CYAN);
    //resetDataAcquisition();
    msg = "WIRED data collection";
  }
  else if (m_acquisitionMode == acquisitionMode::RECORD)
  {
    //resetDataAcquisition();
    msg = "record";
  }
  else if (m_acquisitionMode == acquisitionMode::NONE)
  {
    m_sleepStartTime = millis();
    endDataAcquisition();
    iuRGBLed->changeColor(IURGBLed::SLEEP);
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
void IUConductor::changeOperationState(operationState::option state)
{
  operationState::option previousState = m_operationState;
  m_operationState = state;
  String msg = "";
  if (m_operationState == operationState::IDLE)
  {
    if (previousState != operationState::IDLE)
    {
      m_idleStartTime = millis();
      iuRGBLed->changeColor(IURGBLed::BLUE);
    }
    msg = "Idle";
  }
  else if (m_operationState == operationState::NORMAL)
  {
    iuRGBLed->changeColor(IURGBLed::GREEN);
    msg = "Normal";
  }
  else if (m_operationState == operationState::WARNING)
  {
    iuRGBLed->changeColor(IURGBLed::ORANGE);
    msg = "Warning";
  }
  else if (m_operationState == operationState::DANGER)
  {
    iuRGBLed->changeColor(IURGBLed::RED);
    msg = "Danger";
  }
  if (loopDebugMode) { debugPrint("\nIn '" + msg + "' state\n"); }
}

/**
 * Check if the powerPreset needs to be updated: if yes, update it
 * NB: To update the mode, this function calls method changePowerPreset.
 * @return true if the powerPreset was updated, else false.
 */
bool IUConductor::checkAndUpdatePowerPreset()
{
  // TODO: Determine conditions
  return false;
}

/**
 * Check if the acquisitionMode needs to be updated: if yes, update it
 * NB: To update the mode, this function calls method changeAcquisitionMode.
 * @return true if the mode was updated, else false.
 */
bool IUConductor::checkAndUpdateAcquisitionMode()
{
  if (m_autoSleepEnabled)
  {
    if (m_acquisitionMode != acquisitionMode::NONE && m_startSleepTimer < millis() - m_idleStartTime)
    {
      changeAcquisitionMode(acquisitionMode::NONE); // go to sleep
      return true;
    }
    else if (m_acquisitionMode == acquisitionMode::NONE && m_operationState == operationState::IDLE && m_endSleepTimer < millis() - m_sleepStartTime)
    {
      changeAcquisitionMode(acquisitionMode::FEATURE); // exit sleep mode into run mode
      return true;
    }
  }
  return false;
}

/**
 * Check if the operation state needs to be updated: if yes, update it
 * NB1: The conductor operation state is based on the highest state from features.
 * NB2: To update the state, this function calls method changeOperationState.
 * NB3: operation state update is disabled when not in "run" operation mode.
 * @return true if the state was updated, else false.
 */
bool IUConductor::checkAndUpdateOperationState()
{
  if (m_acquisitionMode != acquisitionMode::FEATURE)
  {
    return false;
  }
  operationState::option newState = featureConfigurator.getOperationStateFromFeatures();
  if (m_operationState != newState)
  {
    changeOperationState(newState);
    return true;
  }
  return false;
}


/* ============================ Time management methods ============================ */

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
  iuI2C->scanDevices(); // Find components
  iuI2C->resetErrorMessage();

  iuBluetooth = new IUBMD350(iuI2C);
  if (!iuBluetooth)
  {
    iuBluetooth = NULL;
    return false;
  }
  
  iuWifi = new IUESP8285(iuI2C);
  if (!iuWifi)
  {
    iuWifi = NULL;
    return false;
  }

  // Also create and init the LED
  iuRGBLed = new IURGBLed();
  if (!iuRGBLed)
  {
    iuRGBLed = NULL;
    return false;
  }
  iuRGBLed->changeColor(IURGBLed::WHITE);

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
 * Data acquisition function
 *
 * @param  When asynchronous is true, we use the I2S callback read data. We do this
 *         for audio and acceleration data collection. When asynchronous is False,
 *         the function should be called during the main loop runtime.
 * Method benchmarked for (accel + sound + temp) and 6 features at 10microseconds.
 */
void IUConductor::acquireAndSendData(bool asynchronous)
{
  if (!m_inDataAcquistion)
  {
    return;
  }
  if (iuI2C->getErrorMessage().equals("ALL_OK")) // Acquire data only if all ok
  {
    if (m_acquisitionMode == acquisitionMode::FEATURE)
    {
      sensorConfigurator.acquireDataAndSendToReceivers(asynchronous);
    }
    // dataCollection is only processed asynchronously
    if (m_acquisitionMode == acquisitionMode::WIRED && asynchronous)
    {
      sensorConfigurator.acquireDataAndDumpThroughI2C();
      // In data collection mode, need to process I2C data
      processInstructionsFromI2C();
    }
    if (m_acquisitionMode == acquisitionMode::RECORD)
    {
      sensorConfigurator.acquireAndStoreData(asynchronous);
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
}

/**
 * Compute each feature that is ready to be computed
 * NB: Does nothing when not in "run" operation mode
 */
void IUConductor::computeFeatures()
{
  if (m_acquisitionMode == acquisitionMode::FEATURE)
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
  if (m_acquisitionMode != acquisitionMode::FEATURE)
  {
    return false;
  }
  if (isDataSendTime())
  {
    port->print(m_macAddress);                                                 // MAC Address
    port->print(",0"); port->print((uint8_t) m_operationState); port->print(",");     // Operation State
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
  String msg = iuI2C->getBuffer();
  if (msg.length() == 0)
  {
    return;
  }
  if(setupDebugMode) { debugPrint("I2C input is:"); }
  if(setupDebugMode) { debugPrint(msg); }

  // Usage mode switching
  if(m_usageMode == usageMode::OPERATION && iuI2C->checkIfStartCalibration())
  {
    changeUsageMode(usageMode::CALIBRATION);
  }
  else if (m_usageMode == usageMode::CALIBRATION && iuI2C->checkIfEndCalibration())
  {
    changeUsageMode(usageMode::OPERATION);
  }

  // Operation mode switching
  if (m_acquisitionMode == acquisitionMode::FEATURE)
  {
    if (iuI2C->checkIfStartCollection())
    {
      changeAcquisitionMode(acquisitionMode::WIRED);
    }
  }
  else if (m_acquisitionMode == acquisitionMode::WIRED)
  {
    if (iuI2C->checkIfEndCollection())
    {
      changeAcquisitionMode(acquisitionMode::FEATURE);
      sensorConfigurator.iuAccelerometer->resetScale();
      return;
    }
    if (msg.indexOf("Arange") > -1)
    {
      // Update Accel range
      int loc = msg.indexOf("Arange") + 7;
      String accelRangeStr = (String) (msg.charAt(loc));
      int accelRange = accelRangeStr.toInt();
      switch (accelRange)
      {
        case 0:
          sensorConfigurator.iuAccelerometer->setScale(sensorConfigurator.iuAccelerometer->AFS_2G);
          break;
        case 1:
          sensorConfigurator.iuAccelerometer->setScale(sensorConfigurator.iuAccelerometer->AFS_2G);
          break;
        case 2:
          sensorConfigurator.iuAccelerometer->setScale(sensorConfigurator.iuAccelerometer->AFS_2G);
          break;
        case 3:
          sensorConfigurator.iuAccelerometer->setScale(sensorConfigurator.iuAccelerometer->AFS_2G);
          break;
      }
    }
    else if (msg.indexOf("rgb") > -1)
    {
      // Change LED color
      int loc = msg.indexOf("rgb") + 7;
      int R = msg.charAt(loc) - 48;
      int G = msg.charAt(loc + 1) - 48;
      int B = msg.charAt(loc + 2) - 48;
      iuRGBLed->changeColor((bool) R, (bool) G, (bool) B);
    }
    else if (msg.indexOf("acosr") > -1)
    {
      // Change audio sampling rate
      int loc = msg.indexOf("acosr") + 6;
      int A = msg.charAt(loc) - 48;
      int B = msg.charAt(loc + 1) - 48;
      uint16_t samplingRate = (uint16_t) ((A * 10 + B) * 1000);
      sensorConfigurator.iuI2S->setSamplingRate(samplingRate);
      // Update the conductor clock rate (since it's based on I2S clock)
      setClockRate(sensorConfigurator.iuI2S->getClockRate());
    }
    else if (msg.indexOf("accsr") > -1)
    {
      int loc = msg.indexOf("accsr") + 6;
      int A = msg.charAt(loc) - 48;
      int B = msg.charAt(loc + 1) - 48;
      int C = msg.charAt(loc + 2) - 48;
      int D = msg.charAt(loc + 3) - 48;
      int samplingRate = (A * 1000 + B * 100 + C * 10 + D);
      sensorConfigurator.iuAccelerometer->setSamplingRate(samplingRate);
    }
  }
  iuI2C->resetBuffer();    // Clear wire buffer
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
  if (m_acquisitionMode != acquisitionMode::FEATURE)
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

      case '3': // Record button pressed - go into extensive mode to record FFTs
        if (bleBuffer[7] == '0' && bleBuffer[9] == '0' && bleBuffer[11] == '0' && bleBuffer[13] == '0' && bleBuffer[15] == '0' && bleBuffer[17] == '0')
        {
          if (loopDebugMode) { debugPrint("Record mode"); }
          IUFeature *feat = NULL;
          feat = featureConfigurator.getFeatureByName("CX3");
          if (feat)
          {
            feat->streamSourceData(iuBluetooth->port, m_macAddress, "X");
          }
          feat = featureConfigurator.getFeatureByName("CY3");
          if (feat)
          {
            feat->streamSourceData(iuBluetooth->port, m_macAddress, "Y");
          }
          feat = featureConfigurator.getFeatureByName("CZ3");
          if (feat)
          {
            feat->streamSourceData(iuBluetooth->port, m_macAddress, "Z");
          }
        }
        break;
      case '4': // Stop button pressed - go out of record mode back into FEATURE mode
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
  // Interfaces
  if (setupDebugMode) { debugPrint(F("\nInitializing interfaces...")); }
  if (!initInterfaces())
  {
    if (setupDebugMode){ debugPrint(F("Failed to initialize interfaces\n")); }
    while(1);
  }
  if (setupDebugMode)
  {
    memoryLog(F("=> Successfully initialized interfaces"));
    debugPrint(' ');
    iuBluetooth->exposeInfo();
  }
  
  // Configurators
  if (setupDebugMode) { debugPrint(F("\nInitializing configurators...")); }
  if (!initConfigurators())
  {
    if (setupDebugMode) { debugPrint(F("=> Failed to initialize configurators\n")); }
    while(1);
  }
  if (setupDebugMode)
  {
    memoryLog(F("=> Successfully initialized configurators"));
  }

  // Sensors
  if (setupDebugMode) { debugPrint(F("\nInitializing sensors...")); }
  if (!initSensors())
  {
    if (setupDebugMode) { debugPrint(F("Failed to initialize sensors\n")); }
    while(1);
  }
  if (setupDebugMode)
  {
      memoryLog(F("=> Successfully initialized sensors"));
  }

  // Default feature configuration
  if (setupDebugMode) { debugPrint(F("\nSetting up default feature configuration...")); }
  if (!featureConfigurator.doStandardSetup())
  {
    if (setupDebugMode) { debugPrint(F("Failed to configure features\n")); }
    while(1);                                                // hang
  }
  if (!linkFeaturesToSensors())
  {
    if (setupDebugMode) { debugPrint(F("Failed to link feature sources to sensors\n")); }
    while(1);                                                // hang
  }
  if (setupDebugMode)
  {
    memoryLog(F("Default features succesfully configured"));
  }
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

  if (setupDebugMode) { debugPrint(F("\n***Done setting up components and configurations***\n")); }

  m_callback = callback;
  changeAcquisitionMode(acquisitionMode::FEATURE);
  changeOperationState(operationState::IDLE);
}

void IUConductor::loop()
{
  if (m_usageMode == usageMode::OPERATION)
  {
    if (m_acquisitionMode == acquisitionMode::WIRED)
    {
      return;
    }
    checkAndUpdateAcquisitionMode();
    processInstructionsFromBluetooth();  // Receive instructions via BLE
    processInstructionsFromI2C();        // Receive instructions to enter / exit modes, plus options during data collection
    acquireAndSendData(false);           // Acquire data from asynchronous sensor
    computeFeatures();                   // Feature computation depending on operation mode
    checkAndUpdateOperationState();      // Update the operationState
    streamData(iuBluetooth->port);       // Stream data over BLE
  }
  else if (m_usageMode == usageMode::CALIBRATION)
  {
    processInstructionsFromI2C();        // Receive instructions to enter / exit modes
    computeFeatures();                   // Feature computation depending on operation mode
    streamData(iuI2C->port);             // Stream data over I2C during calibration
  }
  else if (m_usageMode == usageMode::CONFIGURATION)
  {
    processInstructionsFromBluetooth();  // Receive instructions via BLE
    processInstructionsFromI2C();        // Receive instructions to enter / exit modes
  }
}


/* ====================== Diagnostic Functions, only active when setupDebugMode = true ====================== */

